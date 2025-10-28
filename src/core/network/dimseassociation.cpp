/*
 * ------------------------------------------------------------------------------------
 *  File: dimseassociation.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of DCMTK association wrapper.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "dimseassociation.h"
#include <dcmtk/dcmnet/diutil.h>
#include <dcmtk/dcmnet/cond.h>
#include <dcmtk/dcmdata/dcuid.h>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcDimse)

namespace isis::core::network
{
    DimseStatus conditionToStatus(const OFCondition& cond)
    {
        if (cond.good())
            return DimseStatus::Success;

        const unsigned short code = cond.code();

        // Network-related errors
        if (code == DULC_TCPINITERROR || code == DULC_TCPIOERROR ||
            code == DULC_UNKNOWNHOST || code == DULC_REQUESTASSOCIATIONFAILED)
            return DimseStatus::ConnectionFailed;

        // Association errors
        if (cond == DUL_ASSOCIATIONREJECTED || cond == DUL_PEERABORTEDASSOCIATION)
            return DimseStatus::AssociationRejected;

        // Timeout errors
        if (cond == DUL_NETWORKCLOSED || cond == DUL_READTIMEOUT || cond == DIMSE_NODATAAVAILABLE)
            return DimseStatus::Timeout;

        // Cancelled operations
        if (cond == DUL_PEERREQUESTEDRELEASE)
            return DimseStatus::Cancelled;

        return DimseStatus::Failure;
    }

    std::string statusToString(DimseStatus status)
    {
        switch (status)
        {
        case DimseStatus::Success: return "Success";
        case DimseStatus::Pending: return "Pending";
        case DimseStatus::Failure: return "Operation failed";
        case DimseStatus::Timeout: return "Connection timeout";
        case DimseStatus::ConnectionFailed: return "Connection failed";
        case DimseStatus::AssociationRejected: return "Association rejected by peer";
        case DimseStatus::NetworkError: return "Network error";
        case DimseStatus::InvalidParameters: return "Invalid parameters";
        case DimseStatus::Cancelled: return "Operation cancelled";
        default: return "Unknown error";
        }
    }

    DimseAssociation::DimseAssociation()
    {
    }

    DimseAssociation::~DimseAssociation()
    {
        disconnect();
    }

    DimseStatus DimseAssociation::connect(const DicomPeer& peer,
                                         const LocalAEConfig& localAE,
                                         const std::vector<std::string>& presentationContexts)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (isConnected())
        {
            qCWarning(lcDimse) << "Already connected to" << m_peer.aeTitle.c_str();
            return DimseStatus::Failure;
        }

        m_peer = peer;
        m_localAE = localAE;

        // Initialize network
        DimseStatus status = initializeNetwork();
        if (status != DimseStatus::Success)
        {
            cleanup();
            return status;
        }

        // Create association parameters
        status = createAssociationParameters(presentationContexts);
        if (status != DimseStatus::Success)
        {
            cleanup();
            return status;
        }

        // Request association
        status = requestAssociation();
        if (status != DimseStatus::Success)
        {
            cleanup();
            return status;
        }

        m_connectionTime = std::chrono::system_clock::now();
        qCInfo(lcDimse) << "Connected to" << peer.aeTitle.c_str()
                        << "at" << peer.hostname.c_str() << ":" << peer.port;
        return DimseStatus::Success;
    }

    void DimseAssociation::disconnect()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_assoc != nullptr)
        {
            ASC_releaseAssociation(m_assoc);
            ASC_destroyAssociation(&m_assoc);
            m_assoc = nullptr;
            qCInfo(lcDimse) << "Disconnected from" << m_peer.aeTitle.c_str();
        }

        cleanup();
    }

    DimseStatus DimseAssociation::sendEcho(int timeout)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!isConnected())
        {
            m_lastError = "Not connected";
            return DimseStatus::ConnectionFailed;
        }

        DIC_US msgId = m_assoc->nextMsgID++;
        DIC_US status;

        OFCondition cond = DIMSE_echoUser(m_assoc, msgId, DIMSE_BLOCKING, timeout, &status, nullptr);

        if (cond.bad())
        {
            m_lastError = cond.text();
            qCWarning(lcDimse) << "C-ECHO failed:" << m_lastError.c_str();
            return conditionToStatus(cond);
        }

        if (status != STATUS_Success)
        {
            m_lastError = "C-ECHO returned non-success status: " + std::to_string(status);
            qCWarning(lcDimse) << m_lastError.c_str();
            return DimseStatus::Failure;
        }

        qCInfo(lcDimse) << "C-ECHO successful to" << m_peer.aeTitle.c_str();
        return DimseStatus::Success;
    }

    bool DimseAssociation::hasTimedOut(int timeoutSeconds) const
    {
        if (!isConnected())
            return true;

        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_connectionTime);
        return elapsed.count() > timeoutSeconds;
    }

    DimseStatus DimseAssociation::initializeNetwork()
    {
        OFCondition cond = ASC_initializeNetwork(NET_REQUESTOR, 0, m_peer.timeout, &m_network);
        if (cond.bad())
        {
            m_lastError = "Failed to initialize network: " + std::string(cond.text());
            qCWarning(lcDimse) << m_lastError.c_str();
            return conditionToStatus(cond);
        }
        return DimseStatus::Success;
    }

    DimseStatus DimseAssociation::createAssociationParameters(const std::vector<std::string>& presentationContexts)
    {
        OFCondition cond = ASC_createAssociationParameters(&m_params, m_peer.maxPduSize);
        if (cond.bad())
        {
            m_lastError = "Failed to create association parameters: " + std::string(cond.text());
            qCWarning(lcDimse) << m_lastError.c_str();
            return conditionToStatus(cond);
        }

        // Set calling AE Title (local)
        ASC_setAPTitles(m_params, m_localAE.aeTitle.c_str(), m_peer.aeTitle.c_str(), nullptr);

        // Set peer hostname and port
        std::string peerAddress = m_peer.hostname + ":" + std::to_string(m_peer.port);
        ASC_setPresentationAddresses(m_params, "localhost", peerAddress.c_str());

        // Add presentation contexts
        std::vector<std::string> contexts = presentationContexts.empty() ?
            getDefaultPresentationContexts() : presentationContexts;

        const char* transferSyntaxes[] = {
            UID_LittleEndianImplicitTransferSyntax,
            UID_LittleEndianExplicitTransferSyntax,
            UID_BigEndianExplicitTransferSyntax
        };
        int numTransferSyntaxes = 3;

        int presentationContextID = 1;
        for (const std::string& abstractSyntax : contexts)
        {
            cond = ASC_addPresentationContext(m_params, presentationContextID,
                                             abstractSyntax.c_str(),
                                             transferSyntaxes, numTransferSyntaxes);
            if (cond.bad())
            {
                qCWarning(lcDimse) << "Failed to add presentation context:" << abstractSyntax.c_str();
            }
            presentationContextID += 2; // Must be odd numbers
        }

        return DimseStatus::Success;
    }

    DimseStatus DimseAssociation::requestAssociation()
    {
        OFCondition cond = ASC_requestAssociation(m_network, m_params, &m_assoc);
        if (cond.bad())
        {
            if (cond == DUL_ASSOCIATIONREJECTED)
            {
                m_lastError = "Association rejected by peer";
                qCWarning(lcDimse) << m_lastError.c_str();
                return DimseStatus::AssociationRejected;
            }
            else
            {
                m_lastError = "Failed to request association: " + std::string(cond.text());
                qCWarning(lcDimse) << m_lastError.c_str();
                return conditionToStatus(cond);
            }
        }

        // Check if association was accepted
        if (ASC_countAcceptedPresentationContexts(m_params) == 0)
        {
            m_lastError = "No presentation contexts accepted by peer";
            qCWarning(lcDimse) << m_lastError.c_str();
            return DimseStatus::AssociationRejected;
        }

        return DimseStatus::Success;
    }

    void DimseAssociation::cleanup()
    {
        if (m_network != nullptr)
        {
            ASC_dropNetwork(&m_network);
            m_network = nullptr;
        }
        m_params = nullptr; // Destroyed with association
    }

    std::vector<std::string> DimseAssociation::getDefaultPresentationContexts()
    {
        return {
            UID_VerificationSOPClass,  // C-ECHO
            UID_FINDStudyRootQueryRetrieveInformationModel,  // C-FIND Study Root
            UID_MOVEStudyRootQueryRetrieveInformationModel,  // C-MOVE Study Root
            UID_GETStudyRootQueryRetrieveInformationModel,   // C-GET Study Root
            // Common storage SOP classes
            UID_CTImageStorage,
            UID_MRImageStorage,
            UID_UltrasoundImageStorage,
            UID_SecondaryCaptureImageStorage,
            UID_XRayAngiographicImageStorage,
            UID_ComputedRadiographyImageStorage
        };
    }

    // DimseConnectionPool implementation

    DimseConnectionPool::DimseConnectionPool(size_t maxPoolSize)
        : m_maxPoolSize(maxPoolSize)
    {
    }

    DimseConnectionPool::~DimseConnectionPool()
    {
        clear();
    }

    std::shared_ptr<DimseAssociation> DimseConnectionPool::acquire(const DicomPeer& peer,
                                                                    const LocalAEConfig& localAE)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::string key = getPeerKey(peer);

        // Clean up stale connections periodically
        cleanupStaleConnections();

        // Check if we have an available connection in the pool
        if (m_pool.find(key) != m_pool.end() && !m_pool[key].empty())
        {
            auto& queue = m_pool[key];
            while (!queue.empty())
            {
                PoolEntry entry = queue.front();
                queue.pop();

                if (entry.assoc->isConnected() && !entry.assoc->hasTimedOut(300))
                {
                    entry.inUse = true;
                    entry.lastUsed = std::chrono::system_clock::now();
                    m_activeConnections++;
                    qCInfo(lcDimse) << "Reusing pooled connection to" << peer.aeTitle.c_str();
                    return entry.assoc;
                }
            }
        }

        // Create new connection
        auto assoc = std::make_shared<DimseAssociation>();
        DimseStatus status = assoc->connect(peer, localAE);

        if (status != DimseStatus::Success)
        {
            qCWarning(lcDimse) << "Failed to create new connection:" << statusToString(status).c_str();
            return nullptr;
        }

        m_activeConnections++;
        return assoc;
    }

    void DimseConnectionPool::release(std::shared_ptr<DimseAssociation> assoc)
    {
        if (!assoc)
            return;

        std::lock_guard<std::mutex> lock(m_mutex);

        std::string key = getPeerKey(assoc->getPeer());

        // For now always drop the association to avoid reusing stale DIMSE connections
        assoc->disconnect();
        qCInfo(lcDimse) << "Released association (connection closed)";

        if (m_activeConnections > 0)
            m_activeConnections--;
    }

    void DimseConnectionPool::clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (auto& pair : m_pool)
        {
            while (!pair.second.empty())
            {
                auto entry = pair.second.front();
                pair.second.pop();
                if (entry.assoc)
                {
                    entry.assoc->disconnect();
                }
            }
        }

        m_pool.clear();
        m_activeConnections = 0;
        qCInfo(lcDimse) << "Connection pool cleared";
    }

    size_t DimseConnectionPool::getPoolSize() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t total = 0;
        for (const auto& pair : m_pool)
        {
            total += pair.second.size();
        }
        return total;
    }

    size_t DimseConnectionPool::getActiveConnections() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_activeConnections;
    }

    std::string DimseConnectionPool::getPeerKey(const DicomPeer& peer) const
    {
        return peer.aeTitle + "@" + peer.hostname + ":" + std::to_string(peer.port);
    }

    void DimseConnectionPool::cleanupStaleConnections()
    {
        auto now = std::chrono::system_clock::now();

        for (auto& pair : m_pool)
        {
            std::queue<PoolEntry> newQueue;

            while (!pair.second.empty())
            {
                PoolEntry entry = pair.second.front();
                pair.second.pop();

                // Keep connections that are still valid
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - entry.lastUsed);
                if (elapsed.count() < 300 && entry.assoc->isConnected())
                {
                    newQueue.push(entry);
                }
                else
                {
                    entry.assoc->disconnect();
                    qCInfo(lcDimse) << "Cleaned up stale connection";
                }
            }

            pair.second = newQueue;
        }
    }

} // namespace isis::core::network
