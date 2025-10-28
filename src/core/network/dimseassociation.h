/*
 * ------------------------------------------------------------------------------------
 *  File: dimseassociation.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      DCMTK association wrapper with connection pooling and lifecycle management.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <dcmtk/dcmnet/assoc.h>
#include <dcmtk/dcmnet/dimse.h>
#include <dcmtk/ofstd/ofcond.h>
#include <string>
#include <memory>
#include <mutex>
#include <queue>
#include <map>
#include <chrono>
#include "../utils.h"
#include "dimseconfig.h"

namespace isis::core::network
{
    /**
     * @brief DIMSE operation status codes
     */
    enum class DimseStatus
    {
        Success,
        Pending,
        Failure,
        Timeout,
        ConnectionFailed,
        AssociationRejected,
        NetworkError,
        InvalidParameters,
        Cancelled,
        Unknown
    };

    /**
     * @brief Convert OFCondition to DimseStatus
     */
    DimseStatus conditionToStatus(const OFCondition& cond);

    /**
     * @brief Get human-readable message from DimseStatus
     */
    std::string statusToString(DimseStatus status);

    /**
     * @brief Wrapper around T_ASC_Association for DICOM network operations
     */
    class export DimseAssociation
    {
    public:
        DimseAssociation();
        ~DimseAssociation();

        // Prevent copying
        DimseAssociation(const DimseAssociation&) = delete;
        DimseAssociation& operator=(const DimseAssociation&) = delete;

        /**
         * @brief Connect to a DICOM peer
         * @param peer Peer configuration
         * @param localAE Local AE configuration
         * @param presentationContexts List of abstract syntaxes to propose
         * @return Status of connection attempt
         */
        DimseStatus connect(const DicomPeer& peer,
                           const LocalAEConfig& localAE,
                           const std::vector<std::string>& presentationContexts = {});

        /**
         * @brief Disconnect and release association
         */
        void disconnect();

        /**
         * @brief Check if association is currently connected
         */
        bool isConnected() const { return m_assoc != nullptr; }

        /**
         * @brief Get underlying DCMTK association
         */
        T_ASC_Association* getAssociation() { return m_assoc; }
        const T_ASC_Association* getAssociation() const { return m_assoc; }

        /**
         * @brief Get associated peer information
         */
        const DicomPeer& getPeer() const { return m_peer; }

        /**
         * @brief Send C-ECHO request
         */
        DimseStatus sendEcho(int timeout = 30);

        /**
         * @brief Get last error message
         */
        const std::string& getLastError() const { return m_lastError; }

        /**
         * @brief Get connection timestamp
         */
        std::chrono::system_clock::time_point getConnectionTime() const { return m_connectionTime; }

        /**
         * @brief Check if connection has timed out
         */
        bool hasTimedOut(int timeoutSeconds) const;

    private:
        T_ASC_Association* m_assoc = nullptr;
        T_ASC_Network* m_network = nullptr;
        T_ASC_Parameters* m_params = nullptr;
        DicomPeer m_peer;
        LocalAEConfig m_localAE;
        std::string m_lastError;
        std::chrono::system_clock::time_point m_connectionTime;
        mutable std::mutex m_mutex;

        // Helper methods
        DimseStatus initializeNetwork();
        DimseStatus createAssociationParameters(const std::vector<std::string>& presentationContexts);
        DimseStatus requestAssociation();
        void cleanup();
        std::vector<std::string> getDefaultPresentationContexts();
    };

    /**
     * @brief Connection pool for managing multiple DIMSE associations
     */
    class export DimseConnectionPool
    {
    public:
        DimseConnectionPool(size_t maxPoolSize = 5);
        ~DimseConnectionPool();

        /**
         * @brief Acquire an association from the pool or create new one
         * @param peer Peer to connect to
         * @param localAE Local AE configuration
         * @return Shared pointer to association (nullptr on failure)
         */
        std::shared_ptr<DimseAssociation> acquire(const DicomPeer& peer,
                                                   const LocalAEConfig& localAE);

        /**
         * @brief Release association back to pool
         * @param assoc Association to release
         */
        void release(std::shared_ptr<DimseAssociation> assoc);

        /**
         * @brief Clear all pooled connections
         */
        void clear();

        /**
         * @brief Get pool statistics
         */
        size_t getPoolSize() const;
        size_t getActiveConnections() const;

    private:
        struct PoolEntry
        {
            std::shared_ptr<DimseAssociation> assoc;
            std::chrono::system_clock::time_point lastUsed;
            bool inUse = false;
        };

        std::map<std::string, std::queue<PoolEntry>> m_pool;
        size_t m_maxPoolSize;
        size_t m_activeConnections = 0;
        mutable std::mutex m_mutex;

        // Helper methods
        std::string getPeerKey(const DicomPeer& peer) const;
        void cleanupStaleConnections();
    };

} // namespace isis::core::network
