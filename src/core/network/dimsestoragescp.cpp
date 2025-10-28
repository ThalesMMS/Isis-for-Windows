/*
 * ------------------------------------------------------------------------------------
 *  File: dimsestoragescp.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of Storage SCP for receiving DICOM images.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "dimsestoragescp.h"
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmnet/cond.h>
#include <QDir>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcDimse)

namespace isis::core::network
{
    DimseStorageSCP::DimseStorageSCP(const LocalAEConfig& localAE,
                                    std::shared_ptr<events::CallbackManager> eventManager)
        : m_localConfig(localAE)
        , m_eventManager(eventManager)
        , m_storageDirectory(localAE.tempStoragePath)
    {
        // Ensure storage directory exists
        QDir dir(QString::fromStdString(m_storageDirectory));
        if (!dir.exists())
        {
            dir.mkpath(".");
        }
    }

    DimseStorageSCP::~DimseStorageSCP()
    {
        stop();
    }

    bool DimseStorageSCP::start()
    {
        if (m_running)
        {
            qCWarning(lcDimse) << "Storage SCP already running";
            return false;
        }

        if (!m_localConfig.enableStorage)
        {
            qCInfo(lcDimse) << "Storage SCP is disabled in configuration";
            return false;
        }

        if (!initializeNetwork())
        {
            qCWarning(lcDimse) << "Failed to initialize Storage SCP network";
            return false;
        }

        m_stopRequested = false;
        m_running = true;
        m_serverThread = std::make_unique<std::thread>(&DimseStorageSCP::serverLoop, this);

        qCInfo(lcDimse) << "Storage SCP started on port" << m_localConfig.storagePort
                        << "AE Title:" << m_localConfig.aeTitle.c_str();

        if (m_eventManager)
        {
            m_eventManager->dispatchEvent(events::ProcessingEventType::DimseConnectionEstablished,
                                         "Storage SCP started on port " + std::to_string(m_localConfig.storagePort));
        }

        return true;
    }

    void DimseStorageSCP::stop()
    {
        if (!m_running)
            return;

        qCInfo(lcDimse) << "Stopping Storage SCP...";

        m_stopRequested = true;
        m_running = false;

        if (m_serverThread && m_serverThread->joinable())
        {
            m_serverThread->join();
        }

        cleanupNetwork();

        qCInfo(lcDimse) << "Storage SCP stopped";

        if (m_eventManager)
        {
            m_eventManager->dispatchEvent(events::ProcessingEventType::CustomEvent,
                                         "Storage SCP stopped");
        }
    }

    void DimseStorageSCP::setStorageReceivedCallback(StorageReceivedCallback callback)
    {
        m_storageCallback = callback;
    }

    void DimseStorageSCP::setStorageDirectory(const std::string& dir)
    {
        m_storageDirectory = dir;
        QDir qdir(QString::fromStdString(dir));
        if (!qdir.exists())
        {
            qdir.mkpath(".");
        }
    }

    bool DimseStorageSCP::initializeNetwork()
    {
        OFCondition cond = ASC_initializeNetwork(NET_ACCEPTOR,
                                                m_localConfig.storagePort,
                                                30, // timeout
                                                &m_network);
        if (cond.bad())
        {
            qCWarning(lcDimse) << "Failed to initialize network:" << cond.text();
            return false;
        }
        return true;
    }

    void DimseStorageSCP::cleanupNetwork()
    {
        if (m_network != nullptr)
        {
            ASC_dropNetwork(&m_network);
            m_network = nullptr;
        }
    }

    void DimseStorageSCP::configureAcceptedContexts(T_ASC_Association* assoc)
    {
        if (!assoc || !assoc->params)
        {
            return;
        }

        const char* transferSyntaxes[] = {
            UID_LittleEndianExplicitTransferSyntax,
            UID_BigEndianExplicitTransferSyntax,
            UID_LittleEndianImplicitTransferSyntax
        };
        const int transferSyntaxCount = static_cast<int>(sizeof(transferSyntaxes) / sizeof(const char*));

        ASC_acceptContextsWithPreferredTransferSyntaxes(
            assoc->params,
            dcmAllStorageSOPClassUIDs,
            numberOfDcmAllStorageSOPClassUIDs,
            transferSyntaxes,
            transferSyntaxCount);

        const char* verificationClass[] = { UID_VerificationSOPClass };
        ASC_acceptContextsWithPreferredTransferSyntaxes(
            assoc->params,
            verificationClass,
            1,
            transferSyntaxes,
            transferSyntaxCount);
    }

    void DimseStorageSCP::serverLoop()
    {
        qCInfo(lcDimse) << "Storage SCP server loop started";

        while (!m_stopRequested)
        {
            T_ASC_Association* assoc = nullptr;

            // Wait for incoming association (with timeout to allow checking stop flag)
            OFCondition cond = ASC_receiveAssociation(m_network, &assoc, m_localConfig.maxPduSize,
                                                     nullptr, nullptr, OFFalse,
                                                     DUL_NOBLOCK, 1); // 1 second timeout

            if (cond.bad())
            {
                if (cond == DUL_NOASSOCIATIONREQUEST)
                {
                    // Timeout - no association request, continue loop
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }

                qCWarning(lcDimse) << "Failed to receive association:" << cond.text();
                if (assoc)
                {
                    ASC_dropAssociation(assoc);
                    ASC_destroyAssociation(&assoc);
                }
                continue;
            }

            if (assoc)
            {
                qCInfo(lcDimse) << "Received association request from"
                               << assoc->params->DULparams.callingAPTitle;

                configureAcceptedContexts(assoc);

                // Acknowledge association
                cond = ASC_acknowledgeAssociation(assoc);
                if (cond.bad())
                {
                    qCWarning(lcDimse) << "Failed to acknowledge association:" << cond.text();
                    ASC_dropAssociation(assoc);
                    ASC_destroyAssociation(&assoc);
                    continue;
                }

                // Handle the association
                AssociationResult associationResult = handleAssociation(assoc);

                switch (associationResult)
                {
                case AssociationResult::ReleaseByServer:
                    ASC_releaseAssociation(assoc);
                    break;
                case AssociationResult::ReleaseByPeer:
                    qCInfo(lcDimse) << "Peer requested association release; acknowledged.";
                    break;
                case AssociationResult::AbortOrError:
                    ASC_abortAssociation(assoc);
                    break;
                }

                ASC_dropAssociation(assoc);
                ASC_destroyAssociation(&assoc);
            }
        }

        qCInfo(lcDimse) << "Storage SCP server loop ended";
    }

    DimseStorageSCP::AssociationResult DimseStorageSCP::handleAssociation(T_ASC_Association* assoc)
    {
        if (!assoc)
            return AssociationResult::AbortOrError;

        // Process incoming DIMSE commands
        while (!m_stopRequested)
        {
            T_DIMSE_Message msg;
            T_ASC_PresentationContextID presID;

            // Receive DIMSE command (non-blocking with timeout)
            OFCondition cond = DIMSE_receiveCommand(assoc, DIMSE_NONBLOCKING, 1,
                                                   &presID, &msg, nullptr);

            if (cond == DIMSE_NODATAAVAILABLE || cond == DUL_READTIMEOUT)
            {
                // No data available, check if association is still active
                if (!ASC_dataWaiting(assoc, 0))
                {
                    break; // No more data, exit loop
                }
                continue;
            }

            if (cond == DUL_PEERREQUESTEDRELEASE)
            {
                qCInfo(lcDimse) << "Peer requested association release";
                ASC_acknowledgeRelease(assoc);
                return AssociationResult::ReleaseByPeer;
            }

            if (cond == DUL_PEERABORTEDASSOCIATION)
            {
                qCWarning(lcDimse) << "Peer aborted association";
                return AssociationResult::AbortOrError;
            }

            if (cond.bad())
            {
                qCWarning(lcDimse) << "Failed to receive DIMSE command:" << cond.text();
                return AssociationResult::AbortOrError;
            }

            // Handle different DIMSE commands
            switch (msg.CommandField)
            {
            case DIMSE_C_STORE_RQ:
                handleStoreRequest(assoc, &msg.msg.CStoreRQ, presID);
                break;

            case DIMSE_C_ECHO_RQ:
                {
                    // Send C-ECHO response
                    T_DIMSE_C_EchoRSP response;
                    memset(&response, 0, sizeof(response));
                    response.MessageIDBeingRespondedTo = msg.msg.CEchoRQ.MessageID;
                    response.DimseStatus = STATUS_Success;
                    response.DataSetType = DIMSE_DATASET_NULL;

                    DIMSE_sendEchoResponse(assoc, presID, &msg.msg.CEchoRQ, STATUS_Success, nullptr);
                    qCInfo(lcDimse) << "Responded to C-ECHO";
                }
                break;

            default:
                qCWarning(lcDimse) << "Unsupported DIMSE command:" << msg.CommandField;
                break;
            }
        }

        return AssociationResult::ReleaseByServer;
    }

    OFCondition DimseStorageSCP::handleStoreRequest(T_ASC_Association* assoc,
                                                    T_DIMSE_C_StoreRQ* request,
                                                    T_ASC_PresentationContextID presID)
    {
        if (!assoc || !request)
            return EC_IllegalParameter;

        qCInfo(lcDimse) << "Receiving C-STORE for SOP Instance:"
                       << request->AffectedSOPInstanceUID;

        // Receive dataset
        DcmDataset* dataset = nullptr;
        OFCondition cond = DIMSE_receiveDataSetInMemory(assoc, DIMSE_BLOCKING, 0,
                                                        &presID, &dataset, nullptr, nullptr);

        DIC_US status = STATUS_Success;
        if (cond.bad() || !dataset)
        {
            qCWarning(lcDimse) << "Failed to receive dataset:" << cond.text();
            status = STATUS_STORE_Error_CannotUnderstand;
        }
        else
        {
            // Save the dataset
            std::string filepath = saveReceivedObject(dataset, request->AffectedSOPInstanceUID);

            if (filepath.empty())
            {
                qCWarning(lcDimse) << "Failed to save received object";
                status = STATUS_STORE_Refused_OutOfResources;
            }
            else
            {
                qCInfo(lcDimse) << "Saved received object to:" << filepath.c_str();
                m_receivedFiles++;

                if (m_eventManager)
                {
                    m_eventManager->dispatchEvent(events::ProcessingEventType::DimseStorageReceived,
                                                 "Received file: " + filepath);
                }

                if (m_storageCallback)
                {
                    m_storageCallback(filepath);
                }
            }

            delete dataset;
        }

        // Send C-STORE response
        T_DIMSE_C_StoreRSP response;
        memset(&response, 0, sizeof(response));
        response.MessageIDBeingRespondedTo = request->MessageID;
        response.DimseStatus = status;
        response.DataSetType = DIMSE_DATASET_NULL;
        strcpy(response.AffectedSOPClassUID, request->AffectedSOPClassUID);
        strcpy(response.AffectedSOPInstanceUID, request->AffectedSOPInstanceUID);

        cond = DIMSE_sendStoreResponse(assoc, presID, request, &response, nullptr);

        if (cond.bad())
        {
            qCWarning(lcDimse) << "Failed to send C-STORE response:" << cond.text();
        }

        return cond;
    }

    std::string DimseStorageSCP::saveReceivedObject(DcmDataset* dataset,
                                                    const std::string& sopInstanceUID)
    {
        if (!dataset)
            return "";

        // Create filename from SOP Instance UID
        QString filename = QString::fromStdString(sopInstanceUID) + ".dcm";
        QDir storageDir(QString::fromStdString(m_storageDirectory));
        QString filepath = storageDir.filePath(filename);

        // Create file format object
        DcmFileFormat fileFormat(dataset);

        // Save to file
        OFCondition cond = fileFormat.saveFile(filepath.toStdString().c_str(),
                                              EXS_LittleEndianExplicit);

        if (cond.bad())
        {
            qCWarning(lcDimse) << "Failed to save file:" << cond.text();
            return "";
        }

        return filepath.toStdString();
    }

} // namespace isis::core::network
