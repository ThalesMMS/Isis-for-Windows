/*
 * ------------------------------------------------------------------------------------
 *  File: dimsenetworkmanager.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of DIMSE network manager.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "dimsenetworkmanager.h"
#include "filesimporter.h"
#include <QLoggingCategory>
#include <QFileInfo>

Q_LOGGING_CATEGORY(lcDimseGui, "isis.gui.dimse")

namespace isis::gui
{
    DimseNetworkManager::DimseNetworkManager(FilesImporter* filesImporter, QObject* parent)
        : QObject(parent)
        , m_filesImporter(filesImporter)
    {
    }

    DimseNetworkManager::~DimseNetworkManager()
    {
        shutdown();
    }

    bool DimseNetworkManager::initialize()
    {
        if (m_initialized)
        {
            qCInfo(lcDimseGui) << "DIMSE Network Manager already initialized";
            return true;
        }

        qCInfo(lcDimseGui) << "Initializing DIMSE Network Manager...";

        // Create configuration
        m_config = std::make_shared<core::network::DimseConfig>();
        if (!m_config->loadFromDefault())
        {
            qCWarning(lcDimseGui) << "Failed to load DIMSE configuration, using defaults";
        }

        // Create event manager
        m_eventManager = std::make_shared<core::events::CallbackManager>();
        setupEventCallbacks();

        // Create connection pool
        m_connectionPool = std::make_shared<core::network::DimseConnectionPool>(
            m_config->getLocalAEConfig().maxConnections);

        // Create services
        m_echoService = std::make_shared<core::network::DimseEchoService>(
            m_connectionPool, m_eventManager);

        m_queryService = std::make_shared<core::network::DimseQueryService>(
            m_connectionPool, m_eventManager);

        m_retrieveService = std::make_shared<core::network::DimseRetrieveService>(
            m_connectionPool, m_eventManager);

        m_storeService = std::make_shared<core::network::DimseStoreService>(
            m_connectionPool, m_eventManager);

        // Create Storage SCP
        m_storageScp = std::make_shared<core::network::DimseStorageSCP>(
            m_config->getLocalAEConfig(), m_eventManager);

        // Set up Storage SCP callback
        m_storageScp->setStorageReceivedCallback(
            [this](const std::string& filepath) {
                onStorageReceived(filepath);
            });

        // Auto-start Storage SCP if enabled
        if (m_config->getLocalAEConfig().enableStorage)
        {
            if (startStorageScp())
            {
                qCInfo(lcDimseGui) << "Storage SCP auto-started on port"
                               << m_config->getLocalAEConfig().storagePort;
            }
            else
            {
                qCWarning(lcDimseGui) << "Failed to auto-start Storage SCP";
            }
        }

        m_initialized = true;
        emit statusChanged("DIMSE services initialized");
        qCInfo(lcDimseGui) << "DIMSE Network Manager initialized successfully";

        return true;
    }

    void DimseNetworkManager::shutdown()
    {
        if (!m_initialized)
            return;

        qCInfo(lcDimseGui) << "Shutting down DIMSE Network Manager...";

        // Stop Storage SCP
        stopStorageScp();

        // Clear connection pool
        if (m_connectionPool)
        {
            m_connectionPool->clear();
        }

        // Clear services
        m_echoService.reset();
        m_queryService.reset();
        m_retrieveService.reset();
        m_storageScp.reset();
        m_connectionPool.reset();
        m_eventManager.reset();
        m_config.reset();

        m_initialized = false;
        emit statusChanged("DIMSE services shut down");
        qCInfo(lcDimseGui) << "DIMSE Network Manager shut down";
    }

    bool DimseNetworkManager::isStorageScpRunning() const
    {
        return m_storageScp && m_storageScp->isRunning();
    }

    bool DimseNetworkManager::startStorageScp()
    {
        if (!m_storageScp)
        {
            qCWarning(lcDimseGui) << "Storage SCP not created";
            return false;
        }

        if (m_storageScp->isRunning())
        {
            qCInfo(lcDimseGui) << "Storage SCP already running";
            return true;
        }

        if (m_storageScp->start())
        {
            emit statusChanged(QString("Storage SCP started on port %1")
                .arg(m_config->getLocalAEConfig().storagePort));
            qCInfo(lcDimseGui) << "Storage SCP started successfully";
            return true;
        }
        else
        {
            emit statusChanged("Failed to start Storage SCP");
            qCWarning(lcDimseGui) << "Failed to start Storage SCP";
            return false;
        }
    }

    void DimseNetworkManager::stopStorageScp()
    {
        if (!m_storageScp)
            return;

        if (m_storageScp->isRunning())
        {
            m_storageScp->stop();
            emit statusChanged("Storage SCP stopped");
            qCInfo(lcDimseGui) << "Storage SCP stopped";
        }
    }

    void DimseNetworkManager::setupEventCallbacks()
    {
        if (!m_eventManager)
            return;

        // Register callback for DIMSE events
        m_eventManager->registerFilteredCallback(
            [this](const core::events::ProcessingEventData& event) {
                QString message = QString::fromStdString(event.message);

                // Emit status changes for important events
                switch (event.type)
                {
                case core::events::ProcessingEventType::DimseConnectionEstablished:
                case core::events::ProcessingEventType::DimseEchoCompleted:
                case core::events::ProcessingEventType::DimseQueryCompleted:
                case core::events::ProcessingEventType::DimseRetrieveCompleted:
                    emit statusChanged(message);
                    break;

                case core::events::ProcessingEventType::DimseError:
                case core::events::ProcessingEventType::DimseConnectionFailed:
                    qCWarning(lcDimseGui) << "DIMSE Error:" << message;
                    emit statusChanged("Error: " + message);
                    break;

                case core::events::ProcessingEventType::DimseStorageReceived:
                    // Handled by storage callback
                    break;

                default:
                    // Log but don't emit for progress events
                    if (event.type == core::events::ProcessingEventType::DimseQueryProgress ||
                        event.type == core::events::ProcessingEventType::DimseRetrieveProgress)
                    {
                        qCDebug(lcDimseGui) << message;
                    }
                    break;
                }
            },
            {
                core::events::ProcessingEventType::DimseConnectionStarted,
                core::events::ProcessingEventType::DimseConnectionEstablished,
                core::events::ProcessingEventType::DimseConnectionFailed,
                core::events::ProcessingEventType::DimseEchoStarted,
                core::events::ProcessingEventType::DimseEchoCompleted,
                core::events::ProcessingEventType::DimseQueryStarted,
                core::events::ProcessingEventType::DimseQueryProgress,
                core::events::ProcessingEventType::DimseQueryCompleted,
                core::events::ProcessingEventType::DimseRetrieveStarted,
                core::events::ProcessingEventType::DimseRetrieveProgress,
                core::events::ProcessingEventType::DimseRetrieveCompleted,
                core::events::ProcessingEventType::DimseStorageReceived,
                core::events::ProcessingEventType::DimseError
            },
            "DimseNetworkManager"
        );
    }

    void DimseNetworkManager::onStorageReceived(const std::string& filepath)
    {
        QString qFilepath = QString::fromStdString(filepath);

        qCInfo(lcDimseGui) << "Received DICOM file via C-STORE:" << qFilepath;

        // Verify file exists
        QFileInfo fileInfo(qFilepath);
        if (!fileInfo.exists())
        {
            qCWarning(lcDimseGui) << "Received file does not exist:" << qFilepath;
            return;
        }

        // Emit signal for GUI
        emit fileReceived(qFilepath);

        // Auto-import if FilesImporter is available
        if (m_filesImporter)
        {
            qCInfo(lcDimseGui) << "Auto-importing received file:" << qFilepath;
            QStringList files;
            files << qFilepath;
            m_filesImporter->addFiles(files);
        }
        else
        {
            qCWarning(lcDimseGui) << "FilesImporter not available, file not imported:" << qFilepath;
        }
    }

    void DimseNetworkManager::onFileReceived(const QString& filepath)
    {
        // This slot can be connected to UI elements if needed
        qCInfo(lcDimseGui) << "File received signal:" << filepath;
    }

} // namespace isis::gui
