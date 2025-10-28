/*
 * ------------------------------------------------------------------------------------
 *  File: dimseservices.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      DIMSE service classes for C-ECHO, C-FIND, C-MOVE/C-GET operations.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <future>
#include <atomic>
#include <QFuture>
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmnet/dimse.h>
#include "../utils.h"
#include "../events/callbackmanager.h"
#include "dimseconfig.h"
#include "dimseassociation.h"

namespace isis::core::network
{
    /**
     * @brief Query filter for C-FIND operations
     */
    struct QueryFilter
    {
        std::string patientName;
        std::string patientID;
        std::string studyDate;              // YYYYMMDD or range YYYYMMDD-YYYYMMDD
        std::string studyTime;
        std::string accessionNumber;
        std::string studyInstanceUID;
        std::string seriesInstanceUID;
        std::vector<std::string> modalities;
        std::string studyDescription;
        int maxResults = 100;

        QueryFilter() = default;
    };

    /**
     * @brief Study information from C-FIND response
     */
    struct RemoteStudyInfo
    {
        std::string studyInstanceUID;
        std::string patientName;
        std::string patientID;
        std::string patientBirthDate;
        std::string patientSex;
        std::string studyDate;
        std::string studyTime;
        std::string studyDescription;
        std::string accessionNumber;
        std::string modality;
        int numberOfSeries = 0;
        int numberOfInstances = 0;

        RemoteStudyInfo() = default;
    };

    /**
     * @brief Series information from C-FIND response
     */
    struct RemoteSeriesInfo
    {
        std::string seriesInstanceUID;
        std::string studyInstanceUID;
        std::string seriesNumber;
        std::string seriesDescription;
        std::string modality;
        int numberOfInstances = 0;

        RemoteSeriesInfo() = default;
    };

    /**
     * @brief Progress callback for DIMSE operations
     */
    using DimseProgressCallback = std::function<void(double progress, const std::string& message)>;

    /**
     * @brief Query result callback
     */
    using QueryResultCallback = std::function<void(const RemoteStudyInfo& study)>;

    /**
     * @brief C-ECHO service for connectivity testing
     */
    class export DimseEchoService
    {
    public:
        DimseEchoService(std::shared_ptr<DimseConnectionPool> pool,
                        std::shared_ptr<events::CallbackManager> eventManager);
        ~DimseEchoService() = default;

        /**
         * @brief Perform synchronous C-ECHO
         * @param peer Peer to echo
         * @param localAE Local AE configuration
         * @param timeout Timeout in seconds
         * @return Status of echo operation
         */
        DimseStatus performEcho(const DicomPeer& peer, const LocalAEConfig& localAE, int timeout = 30);

        /**
         * @brief Perform asynchronous C-ECHO
         */
        QFuture<DimseStatus> performEchoAsync(const DicomPeer& peer, const LocalAEConfig& localAE);

        /**
         * @brief Get last error message
         */
        const std::string& getLastError() const { return m_lastError; }

    private:
        std::shared_ptr<DimseConnectionPool> m_connectionPool;
        std::shared_ptr<events::CallbackManager> m_eventManager;
        std::string m_lastError;
    };

    /**
     * @brief C-FIND service for querying PACS
     */
    class export DimseQueryService
    {
    public:
        DimseQueryService(std::shared_ptr<DimseConnectionPool> pool,
                         std::shared_ptr<events::CallbackManager> eventManager);
        ~DimseQueryService() = default;

        /**
         * @brief Query studies from PACS
         * @param peer Peer to query
         * @param localAE Local AE configuration
         * @param filter Query filter
         * @param results Vector to store results
         * @param callback Optional progress callback
         * @return Status of query operation
         */
        DimseStatus queryStudies(const DicomPeer& peer,
                                const LocalAEConfig& localAE,
                                const QueryFilter& filter,
                                std::vector<RemoteStudyInfo>& results,
                                QueryResultCallback callback = nullptr);

        /**
         * @brief Query series for a study
         */
        DimseStatus querySeries(const DicomPeer& peer,
                               const LocalAEConfig& localAE,
                               const std::string& studyInstanceUID,
                               std::vector<RemoteSeriesInfo>& results);

        /**
         * @brief Perform asynchronous query
         */
        QFuture<std::vector<RemoteStudyInfo>> queryStudiesAsync(
            const DicomPeer& peer,
            const LocalAEConfig& localAE,
            const QueryFilter& filter,
            QueryResultCallback callback = nullptr);

        /**
         * @brief Cancel ongoing query
         */
        void cancelQuery();

        /**
         * @brief Get last error message
         */
        const std::string& getLastError() const { return m_lastError; }

    private:
        std::shared_ptr<DimseConnectionPool> m_connectionPool;
        std::shared_ptr<events::CallbackManager> m_eventManager;
        std::string m_lastError;
        std::atomic<bool> m_cancelRequested{false};

        // Helper methods
        DcmDataset* buildQueryDataset(const QueryFilter& filter);
        RemoteStudyInfo parseStudyResponse(DcmDataset* dataset);
        RemoteSeriesInfo parseSeriesResponse(DcmDataset* dataset);

        struct FindCallbackContext
        {
            DimseQueryService* service = nullptr;
            QueryResultCallback callback;
            std::vector<RemoteStudyInfo>* results = nullptr;
            int maxResults = 0;
        };

        static void findResponseHandler(void* callbackData,
                                        T_DIMSE_C_FindRQ* request,
                                        int responseCount,
                                        T_DIMSE_C_FindRSP* response,
                                        DcmDataset* responseIdentifiers);
    };

    /**
     * @brief C-MOVE/C-GET service for retrieving studies
     */
    class export DimseRetrieveService
    {
    public:
        DimseRetrieveService(std::shared_ptr<DimseConnectionPool> pool,
                            std::shared_ptr<events::CallbackManager> eventManager);
        ~DimseRetrieveService() = default;

        /**
         * @brief Retrieve study using C-MOVE
         * @param peer Peer to retrieve from
         * @param localAE Local AE configuration
         * @param studyInstanceUID Study to retrieve
         * @param destinationAE Destination AE for C-MOVE
         * @param callback Progress callback
         * @return Status of retrieve operation
         */
        DimseStatus retrieveStudyMove(const DicomPeer& peer,
                                     const LocalAEConfig& localAE,
                                     const std::string& studyInstanceUID,
                                     const std::string& destinationAE,
                                     DimseProgressCallback callback = nullptr);

        /**
         * @brief Retrieve study using C-GET
         */
        DimseStatus retrieveStudyGet(const DicomPeer& peer,
                                    const LocalAEConfig& localAE,
                                    const std::string& studyInstanceUID,
                                    DimseProgressCallback callback = nullptr);

        /**
         * @brief Retrieve study (auto-select C-MOVE or C-GET)
         */
        DimseStatus retrieveStudy(const DicomPeer& peer,
                                 const LocalAEConfig& localAE,
                                 const std::string& studyInstanceUID,
                                 DimseProgressCallback callback = nullptr);

        /**
         * @brief Perform asynchronous retrieve
         */
        QFuture<DimseStatus> retrieveStudyAsync(const DicomPeer& peer,
                                                    const LocalAEConfig& localAE,
                                                    const std::string& studyInstanceUID,
                                                    DimseProgressCallback callback = nullptr);

        /**
         * @brief Cancel ongoing retrieve
         */
        void cancelRetrieve();

        /**
         * @brief Get last error message
         */
        const std::string& getLastError() const { return m_lastError; }

    private:
        std::shared_ptr<DimseConnectionPool> m_connectionPool;
        std::shared_ptr<events::CallbackManager> m_eventManager;
        std::string m_lastError;
        std::atomic<bool> m_cancelRequested{false};

        // Helper methods
        DcmDataset* buildRetrieveDataset(const std::string& studyInstanceUID);
        struct MoveCallbackContext
        {
            DimseRetrieveService* service = nullptr;
            DimseProgressCallback progressCallback;
        };
        static void moveResponseHandler(void* callbackData,
                                        T_DIMSE_C_MoveRQ* request,
                                        int responseCount,
                                        T_DIMSE_C_MoveRSP* response);
    };

    /**
     * @brief C-STORE SCU service for sending DICOM files to PACS
     */
    class export DimseStoreService
    {
    public:
        DimseStoreService(std::shared_ptr<DimseConnectionPool> pool,
                         std::shared_ptr<events::CallbackManager> eventManager);
        ~DimseStoreService() = default;

        /**
         * @brief Store single DICOM file to PACS
         * @param peer Peer to send to
         * @param localAE Local AE configuration
         * @param filepath Path to DICOM file
         * @param callback Progress callback
         * @return Status of store operation
         */
        DimseStatus storeFile(const DicomPeer& peer,
                             const LocalAEConfig& localAE,
                             const std::string& filepath,
                             DimseProgressCallback callback = nullptr);

        /**
         * @brief Store multiple DICOM files to PACS
         * @param peer Peer to send to
         * @param localAE Local AE configuration
         * @param filepaths Vector of file paths
         * @param callback Progress callback
         * @return Status of store operation
         */
        DimseStatus storeFiles(const DicomPeer& peer,
                              const LocalAEConfig& localAE,
                              const std::vector<std::string>& filepaths,
                              DimseProgressCallback callback = nullptr);

        /**
         * @brief Store all files in a directory to PACS
         * @param peer Peer to send to
         * @param localAE Local AE configuration
         * @param directory Directory path
         * @param recursive Search recursively
         * @param callback Progress callback
         * @return Status of store operation
         */
        DimseStatus storeDirectory(const DicomPeer& peer,
                                  const LocalAEConfig& localAE,
                                  const std::string& directory,
                                  bool recursive = true,
                                  DimseProgressCallback callback = nullptr);

        /**
         * @brief Perform asynchronous store
         */
        QFuture<DimseStatus> storeFilesAsync(const DicomPeer& peer,
                                                const LocalAEConfig& localAE,
                                                const std::vector<std::string>& filepaths,
                                                DimseProgressCallback callback = nullptr);

        /**
         * @brief Cancel ongoing store operation
         */
        void cancelStore();

        /**
         * @brief Get last error message
         */
        const std::string& getLastError() const { return m_lastError; }

        /**
         * @brief Get number of successfully stored files
         */
        int getStoredCount() const { return m_storedCount; }

        /**
         * @brief Get number of failed files
         */
        int getFailedCount() const { return m_failedCount; }

    private:
        std::shared_ptr<DimseConnectionPool> m_connectionPool;
        std::shared_ptr<events::CallbackManager> m_eventManager;
        std::string m_lastError;
        std::atomic<bool> m_cancelRequested{false};
        int m_storedCount{0};
        int m_failedCount{0};

        // Helper methods
        DimseStatus storeSingleFile(T_ASC_Association* assoc,
                                   const std::string& filepath,
                                   int fileIndex,
                                   int totalFiles,
                                   DimseProgressCallback callback);
        std::vector<std::string> findDicomFiles(const std::string& directory, bool recursive);
        bool isDicomFile(const std::string& filepath);
    };

} // namespace isis::core::network
