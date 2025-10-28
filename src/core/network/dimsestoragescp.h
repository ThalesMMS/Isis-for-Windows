/*
 * ------------------------------------------------------------------------------------
 *  File: dimsestoragescp.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Storage SCP for receiving DICOM images via C-STORE (for C-MOVE operations).
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <dcmtk/dcmnet/assoc.h>
#include <dcmtk/dcmnet/dimse.h>
#include "../utils.h"
#include "../events/callbackmanager.h"
#include "dimseconfig.h"

namespace isis::core::network
{
    /**
     * @brief Callback for received DICOM files
     */
    using StorageReceivedCallback = std::function<void(const std::string& filepath)>;

    /**
     * @brief Simple Storage SCP for receiving C-STORE operations
     */
    class export DimseStorageSCP
    {
    public:
        DimseStorageSCP(const LocalAEConfig& localAE,
                       std::shared_ptr<events::CallbackManager> eventManager);
        ~DimseStorageSCP();

        /**
         * @brief Start the Storage SCP server
         * @return true if started successfully
         */
        bool start();

        /**
         * @brief Stop the Storage SCP server
         */
        void stop();

        /**
         * @brief Check if server is running
         */
        bool isRunning() const { return m_running; }

        /**
         * @brief Set callback for received files
         */
        void setStorageReceivedCallback(StorageReceivedCallback callback);

        /**
         * @brief Get storage directory
         */
        const std::string& getStorageDirectory() const { return m_storageDirectory; }

        /**
         * @brief Set storage directory
         */
        void setStorageDirectory(const std::string& dir);

        /**
         * @brief Get number of received files
         */
        size_t getReceivedFileCount() const { return m_receivedFiles; }

        /**
         * @brief Reset received file counter
         */
        void resetFileCount() { m_receivedFiles = 0; }

    private:
        LocalAEConfig m_localConfig;
        std::shared_ptr<events::CallbackManager> m_eventManager;
        std::string m_storageDirectory;
        std::atomic<bool> m_running{false};
        std::atomic<bool> m_stopRequested{false};
        std::atomic<size_t> m_receivedFiles{0};
        std::unique_ptr<std::thread> m_serverThread;
        T_ASC_Network* m_network = nullptr;
        StorageReceivedCallback m_storageCallback;

        enum class AssociationResult
        {
            ReleaseByServer,   // Server still needs to send release
            ReleaseByPeer,     // Peer requested release and we acknowledged
            AbortOrError       // Association terminated due to abort/error
        };

        // Server thread function
        void serverLoop();

        // Handle incoming association
        AssociationResult handleAssociation(T_ASC_Association* assoc);

        // Handle C-STORE request
        OFCondition handleStoreRequest(T_ASC_Association* assoc,
                                       T_DIMSE_C_StoreRQ* request,
                                       T_ASC_PresentationContextID presID);

        // Save received DICOM object
        std::string saveReceivedObject(DcmDataset* dataset,
                                      const std::string& sopInstanceUID);

        // Helper methods
        bool initializeNetwork();
        void cleanupNetwork();
        void configureAcceptedContexts(T_ASC_Association* assoc);
    };

} // namespace isis::core::network
