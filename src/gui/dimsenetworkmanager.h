/*
 * ------------------------------------------------------------------------------------
 *  File: dimsenetworkmanager.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Manager for DIMSE networking services, handling configuration, connections,
 *      and lifecycle of Storage SCP. Integrates DIMSE operations with the GUI.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <QObject>
#include "../core/network/dimseconfig.h"
#include "../core/network/dimseassociation.h"
#include "../core/network/dimseservices.h"
#include "../core/network/dimsestoragescp.h"
#include "../core/events/callbackmanager.h"

namespace isis::gui
{
    class FilesImporter;

    /**
     * @brief Manages DIMSE networking services for the GUI
     */
    class DimseNetworkManager : public QObject
    {
        Q_OBJECT

    public:
        explicit DimseNetworkManager(FilesImporter* filesImporter, QObject* parent = nullptr);
        ~DimseNetworkManager() override;

        /**
         * @brief Initialize DIMSE services and load configuration
         * @return true if successful
         */
        bool initialize();

        /**
         * @brief Shutdown DIMSE services
         */
        void shutdown();

        /**
         * @brief Get DIMSE configuration
         */
        std::shared_ptr<core::network::DimseConfig> getConfig() const { return m_config; }

        /**
         * @brief Get connection pool
         */
        std::shared_ptr<core::network::DimseConnectionPool> getConnectionPool() const { return m_connectionPool; }

        /**
         * @brief Get event manager
         */
        std::shared_ptr<core::events::CallbackManager> getEventManager() const { return m_eventManager; }

        /**
         * @brief Get echo service
         */
        std::shared_ptr<core::network::DimseEchoService> getEchoService() const { return m_echoService; }

        /**
         * @brief Get query service
         */
        std::shared_ptr<core::network::DimseQueryService> getQueryService() const { return m_queryService; }

        /**
         * @brief Get retrieve service
         */
        std::shared_ptr<core::network::DimseRetrieveService> getRetrieveService() const { return m_retrieveService; }

        /**
         * @brief Get store service (C-STORE SCU)
         */
        std::shared_ptr<core::network::DimseStoreService> getStoreService() const { return m_storeService; }

        /**
         * @brief Check if Storage SCP is running
         */
        bool isStorageScpRunning() const;

        /**
         * @brief Start Storage SCP
         * @return true if started successfully
         */
        bool startStorageScp();

        /**
         * @brief Stop Storage SCP
         */
        void stopStorageScp();

    signals:
        /**
         * @brief Emitted when a file is received via Storage SCP
         * @param filepath Path to received DICOM file
         */
        void fileReceived(const QString& filepath);

        /**
         * @brief Emitted when DIMSE operation status changes
         * @param message Status message
         */
        void statusChanged(const QString& message);

    private slots:
        void onFileReceived(const QString& filepath);

    private:
        // Core DIMSE services
        std::shared_ptr<core::network::DimseConfig> m_config;
        std::shared_ptr<core::network::DimseConnectionPool> m_connectionPool;
        std::shared_ptr<core::events::CallbackManager> m_eventManager;
        std::shared_ptr<core::network::DimseEchoService> m_echoService;
        std::shared_ptr<core::network::DimseQueryService> m_queryService;
        std::shared_ptr<core::network::DimseRetrieveService> m_retrieveService;
        std::shared_ptr<core::network::DimseStoreService> m_storeService;
        std::shared_ptr<core::network::DimseStorageSCP> m_storageScp;

        // Integration
        FilesImporter* m_filesImporter = nullptr;
        bool m_initialized = false;

        // Helper methods
        void setupEventCallbacks();
        void onStorageReceived(const std::string& filepath);
    };

} // namespace isis::gui
