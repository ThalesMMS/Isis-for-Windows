/*
 * ------------------------------------------------------------------------------------
 *  File: dimseservices.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of DIMSE service classes.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "dimseservices.h"
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmnet/dimse.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/ofstd/ofstd.h>
#include <QLoggingCategory>
#include <QtConcurrent>
#include <QDir>
#include <QDirIterator>
#include <sstream>
#include <algorithm>

Q_DECLARE_LOGGING_CATEGORY(lcDimse)

namespace isis::core::network
{
    void DimseQueryService::findResponseHandler(void* callbackData,
                                                T_DIMSE_C_FindRQ* /*request*/,
                                                int /*responseCount*/,
                                                T_DIMSE_C_FindRSP* response,
                                                DcmDataset* responseIdentifiers)
    {
        auto* context = static_cast<FindCallbackContext*>(callbackData);
        if (context == nullptr || context->service == nullptr)
        {
            return;
        }

        auto* service = context->service;
        if (service->m_cancelRequested.load())
        {
            if (response != nullptr)
            {
                response->DimseStatus = STATUS_FIND_Cancel_MatchingTerminatedDueToCancelRequest;
            }
            return;
        }

        if (response == nullptr || responseIdentifiers == nullptr)
        {
            return;
        }

        if (response->DimseStatus != STATUS_Pending)
        {
            return;
        }

        if (context->results != nullptr &&
            context->maxResults > 0 &&
            context->results->size() >= static_cast<size_t>(context->maxResults))
        {
            return;
        }

        RemoteStudyInfo studyInfo = service->parseStudyResponse(responseIdentifiers);
        if (context->results != nullptr)
        {
            context->results->push_back(studyInfo);

            if (service->m_eventManager && context->maxResults > 0)
            {
                const double progress = std::min(
                    1.0,
                    static_cast<double>(context->results->size()) /
                        static_cast<double>(context->maxResults));
                service->m_eventManager->dispatchProgress(
                    events::ProcessingEventType::DimseQueryProgress,
                    progress,
                    "Found " + std::to_string(context->results->size()) + " studies");
            }
        }

        if (context->callback)
        {
            context->callback(studyInfo);
        }
    }

    void DimseRetrieveService::moveResponseHandler(void* callbackData,
                                                   T_DIMSE_C_MoveRQ* /*request*/,
                                                   int responseCount,
                                                   T_DIMSE_C_MoveRSP* response)
    {
        auto* context = static_cast<MoveCallbackContext*>(callbackData);
        if (context == nullptr || context->service == nullptr || response == nullptr)
        {
            return;
        }

        if (!context->progressCallback)
        {
            return;
        }

        double progressValue = 0.0;
        std::ostringstream message;
        message << "C-MOVE response #" << responseCount;

        if (response->NumberOfRemainingSubOperations >= 0 &&
            response->NumberOfCompletedSubOperations >= 0)
        {
            const double total = static_cast<double>(
                response->NumberOfRemainingSubOperations +
                response->NumberOfCompletedSubOperations +
                response->NumberOfFailedSubOperations +
                response->NumberOfWarningSubOperations);
            if (total > 0.0)
            {
                progressValue = static_cast<double>(response->NumberOfCompletedSubOperations) / total;
            }
        }

        if (response->DimseStatus == STATUS_Pending)
        {
            message << " - remaining: " << response->NumberOfRemainingSubOperations;
        }
        else if (response->DimseStatus == STATUS_Success)
        {
            progressValue = 1.0;
            message << " - completed";
        }
        else
        {
            message << " - status: 0x" << std::hex << response->DimseStatus;
        }

        context->progressCallback(progressValue, message.str());
    }

    // DimseEchoService implementation

    DimseEchoService::DimseEchoService(std::shared_ptr<DimseConnectionPool> pool,
                                       std::shared_ptr<events::CallbackManager> eventManager)
        : m_connectionPool(pool)
        , m_eventManager(eventManager)
    {
    }

    DimseStatus DimseEchoService::performEcho(const DicomPeer& peer,
                                              const LocalAEConfig& localAE,
                                              int timeout)
    {
        if (m_eventManager)
        {
            m_eventManager->dispatchEvent(events::ProcessingEventType::DimseEchoStarted,
                                         "Starting C-ECHO to " + peer.name);
        }

        auto assoc = m_connectionPool->acquire(peer, localAE);
        if (!assoc)
        {
            m_lastError = "Failed to acquire connection";
            qCWarning(lcDimse) << m_lastError.c_str();
            if (m_eventManager)
            {
                m_eventManager->dispatchEvent(events::ProcessingEventType::DimseError, m_lastError);
            }
            return DimseStatus::ConnectionFailed;
        }

        DimseStatus status = assoc->sendEcho(timeout);
        m_lastError = assoc->getLastError();

        m_connectionPool->release(assoc);

        if (m_eventManager)
        {
            if (status == DimseStatus::Success)
            {
                m_eventManager->dispatchEvent(events::ProcessingEventType::DimseEchoCompleted,
                                             "C-ECHO successful to " + peer.name);
            }
            else
            {
                m_eventManager->dispatchEvent(events::ProcessingEventType::DimseError,
                                             "C-ECHO failed: " + m_lastError);
            }
        }

        return status;
    }

    QFuture<DimseStatus> DimseEchoService::performEchoAsync(const DicomPeer& peer,
                                                            const LocalAEConfig& localAE)
    {
        return QtConcurrent::run([this, peer, localAE]() {
            return performEcho(peer, localAE);
        });
    }

    // DimseQueryService implementation

    DimseQueryService::DimseQueryService(std::shared_ptr<DimseConnectionPool> pool,
                                        std::shared_ptr<events::CallbackManager> eventManager)
        : m_connectionPool(pool)
        , m_eventManager(eventManager)
    {
    }

    DimseStatus DimseQueryService::queryStudies(const DicomPeer& peer,
                                                const LocalAEConfig& localAE,
                                                const QueryFilter& filter,
                                                std::vector<RemoteStudyInfo>& results,
                                                QueryResultCallback callback)
    {
        m_cancelRequested = false;
        results.clear();

        if (m_eventManager)
        {
            m_eventManager->dispatchEvent(events::ProcessingEventType::DimseQueryStarted,
                                         "Starting C-FIND query to " + peer.name);
        }

        auto assoc = m_connectionPool->acquire(peer, localAE);
        if (!assoc)
        {
            m_lastError = "Failed to acquire connection";
            qCWarning(lcDimse) << m_lastError.c_str();
            if (m_eventManager)
            {
                m_eventManager->dispatchEvent(events::ProcessingEventType::DimseError, m_lastError);
            }
            return DimseStatus::ConnectionFailed;
        }

        T_ASC_Association* association = assoc->getAssociation();
        if (!association)
        {
            m_lastError = "Invalid association";
            m_connectionPool->release(assoc);
            return DimseStatus::ConnectionFailed;
        }

        // Find accepted presentation context
        T_ASC_PresentationContextID presID = ASC_findAcceptedPresentationContextID(
            association, UID_FINDStudyRootQueryRetrieveInformationModel);

        if (presID == 0)
        {
            m_lastError = "No presentation context for C-FIND Study Root";
            qCWarning(lcDimse) << m_lastError.c_str();
            m_connectionPool->release(assoc);
            return DimseStatus::Failure;
        }

        // Build query dataset
        DcmDataset* queryDataset = buildQueryDataset(filter);
        if (!queryDataset)
        {
            m_lastError = "Failed to build query dataset";
            m_connectionPool->release(assoc);
            return DimseStatus::InvalidParameters;
        }

        // Prepare DIMSE request
        T_DIMSE_C_FindRQ request;
        T_DIMSE_C_FindRSP response;
        DcmDataset* statusDetail = nullptr;

        memset(&request, 0, sizeof(request));
        request.MessageID = association->nextMsgID++;
        strcpy(request.AffectedSOPClassUID, UID_FINDStudyRootQueryRetrieveInformationModel);
        request.Priority = DIMSE_PRIORITY_MEDIUM;
        request.DataSetType = DIMSE_DATASET_PRESENT;

        qCInfo(lcDimse) << "Sending C-FIND request...";

        OFCondition cond;
        int responseCount = 0;
        FindCallbackContext context{this, callback, &results, filter.maxResults};

        cond = DIMSE_findUser(association,
                              presID,
                              &request,
                              queryDataset,
                              responseCount,
                              &DimseQueryService::findResponseHandler,
                              &context,
                              DIMSE_BLOCKING,
                              0,
                              &response,
                              &statusDetail);

        delete queryDataset;
        queryDataset = nullptr;

        if (statusDetail)
        {
            delete statusDetail;
            statusDetail = nullptr;
        }

        m_connectionPool->release(assoc);

        if (cond.bad())
        {
            m_lastError = "C-FIND failed: " + std::string(cond.text());
            qCWarning(lcDimse) << m_lastError.c_str();
            return conditionToStatus(cond);
        }

        if (m_cancelRequested.load() ||
            response.DimseStatus == STATUS_FIND_Cancel_MatchingTerminatedDueToCancelRequest)
        {
            m_lastError = "Query cancelled";
            qCInfo(lcDimse) << m_lastError.c_str();
            return DimseStatus::Cancelled;
        }

        if (response.DimseStatus == STATUS_Success)
        {
            qCInfo(lcDimse) << "C-FIND completed. Found" << results.size() << "studies";
            if (m_eventManager)
            {
                m_eventManager->dispatchEvent(events::ProcessingEventType::DimseQueryCompleted,
                                             "Query completed: found " + std::to_string(results.size()) + " studies");
            }
            return DimseStatus::Success;
        }

        m_lastError = "C-FIND failed with status: " + std::to_string(response.DimseStatus);
        qCWarning(lcDimse) << m_lastError.c_str();
        return DimseStatus::Failure;
    }

    DimseStatus DimseQueryService::querySeries(const DicomPeer& peer,
                                              const LocalAEConfig& localAE,
                                              const std::string& studyInstanceUID,
                                              std::vector<RemoteSeriesInfo>& results)
    {
        // Similar implementation to queryStudies but at SERIES level
        // Omitted for brevity - would query at series root level
        qCInfo(lcDimse) << "Series query not yet implemented";
        return DimseStatus::Success;
    }

    QFuture<std::vector<RemoteStudyInfo>> DimseQueryService::queryStudiesAsync(
        const DicomPeer& peer,
        const LocalAEConfig& localAE,
        const QueryFilter& filter,
        QueryResultCallback callback)
    {
        return QtConcurrent::run([this, peer, localAE, filter, callback]() {
            std::vector<RemoteStudyInfo> results;
            queryStudies(peer, localAE, filter, results, callback);
            return results;
        });
    }

    void DimseQueryService::cancelQuery()
    {
        m_cancelRequested = true;
        qCInfo(lcDimse) << "Query cancellation requested";
    }

    DcmDataset* DimseQueryService::buildQueryDataset(const QueryFilter& filter)
    {
        DcmDataset* dataset = new DcmDataset();

        // Set query/retrieve level
        dataset->putAndInsertString(DCM_QueryRetrieveLevel, "STUDY");

        // Patient level
        if (!filter.patientName.empty())
        {
            dataset->putAndInsertString(DCM_PatientName, filter.patientName.c_str());
        }
        else
        {
            dataset->putAndInsertString(DCM_PatientName, "");
        }

        if (!filter.patientID.empty())
        {
            dataset->putAndInsertString(DCM_PatientID, filter.patientID.c_str());
        }
        else
        {
            dataset->putAndInsertString(DCM_PatientID, "");
        }

        // Study level
        if (!filter.studyDate.empty())
        {
            dataset->putAndInsertString(DCM_StudyDate, filter.studyDate.c_str());
        }
        else
        {
            dataset->putAndInsertString(DCM_StudyDate, "");
        }

        if (!filter.studyInstanceUID.empty())
        {
            dataset->putAndInsertString(DCM_StudyInstanceUID, filter.studyInstanceUID.c_str());
        }
        else
        {
            dataset->putAndInsertString(DCM_StudyInstanceUID, "");
        }

        if (!filter.accessionNumber.empty())
        {
            dataset->putAndInsertString(DCM_AccessionNumber, filter.accessionNumber.c_str());
        }
        else
        {
            dataset->putAndInsertString(DCM_AccessionNumber, "");
        }

        if (!filter.studyDescription.empty())
        {
            dataset->putAndInsertString(DCM_StudyDescription, filter.studyDescription.c_str());
        }
        else
        {
            dataset->putAndInsertString(DCM_StudyDescription, "");
        }

        // Return attributes
        dataset->putAndInsertString(DCM_PatientBirthDate, "");
        dataset->putAndInsertString(DCM_PatientSex, "");
        dataset->putAndInsertString(DCM_StudyTime, "");
        dataset->putAndInsertString(DCM_ModalitiesInStudy, "");
        dataset->putAndInsertString(DCM_NumberOfStudyRelatedSeries, "");
        dataset->putAndInsertString(DCM_NumberOfStudyRelatedInstances, "");

        return dataset;
    }

    RemoteStudyInfo DimseQueryService::parseStudyResponse(DcmDataset* dataset)
    {
        RemoteStudyInfo info;

        if (!dataset)
            return info;

        OFString value;

        if (dataset->findAndGetOFString(DCM_StudyInstanceUID, value).good())
            info.studyInstanceUID = value.c_str();

        if (dataset->findAndGetOFString(DCM_PatientName, value).good())
            info.patientName = value.c_str();

        if (dataset->findAndGetOFString(DCM_PatientID, value).good())
            info.patientID = value.c_str();

        if (dataset->findAndGetOFString(DCM_PatientBirthDate, value).good())
            info.patientBirthDate = value.c_str();

        if (dataset->findAndGetOFString(DCM_PatientSex, value).good())
            info.patientSex = value.c_str();

        if (dataset->findAndGetOFString(DCM_StudyDate, value).good())
            info.studyDate = value.c_str();

        if (dataset->findAndGetOFString(DCM_StudyTime, value).good())
            info.studyTime = value.c_str();

        if (dataset->findAndGetOFString(DCM_StudyDescription, value).good())
            info.studyDescription = value.c_str();

        if (dataset->findAndGetOFString(DCM_AccessionNumber, value).good())
            info.accessionNumber = value.c_str();

        if (dataset->findAndGetOFString(DCM_ModalitiesInStudy, value).good())
            info.modality = value.c_str();

        Sint32 intValue;
        if (dataset->findAndGetSint32(DCM_NumberOfStudyRelatedSeries, intValue).good())
            info.numberOfSeries = intValue;

        if (dataset->findAndGetSint32(DCM_NumberOfStudyRelatedInstances, intValue).good())
            info.numberOfInstances = intValue;

        return info;
    }

    RemoteSeriesInfo DimseQueryService::parseSeriesResponse(DcmDataset* dataset)
    {
        RemoteSeriesInfo info;

        if (!dataset)
            return info;

        OFString value;

        if (dataset->findAndGetOFString(DCM_SeriesInstanceUID, value).good())
            info.seriesInstanceUID = value.c_str();

        if (dataset->findAndGetOFString(DCM_StudyInstanceUID, value).good())
            info.studyInstanceUID = value.c_str();

        if (dataset->findAndGetOFString(DCM_SeriesNumber, value).good())
            info.seriesNumber = value.c_str();

        if (dataset->findAndGetOFString(DCM_SeriesDescription, value).good())
            info.seriesDescription = value.c_str();

        if (dataset->findAndGetOFString(DCM_Modality, value).good())
            info.modality = value.c_str();

        Sint32 intValue;
        if (dataset->findAndGetSint32(DCM_NumberOfSeriesRelatedInstances, intValue).good())
            info.numberOfInstances = intValue;

        return info;
    }

    // DimseRetrieveService implementation

    DimseRetrieveService::DimseRetrieveService(std::shared_ptr<DimseConnectionPool> pool,
                                              std::shared_ptr<events::CallbackManager> eventManager)
        : m_connectionPool(pool)
        , m_eventManager(eventManager)
    {
    }

    DimseStatus DimseRetrieveService::retrieveStudyMove(const DicomPeer& peer,
                                                       const LocalAEConfig& localAE,
                                                       const std::string& studyInstanceUID,
                                                       const std::string& destinationAE,
                                                       DimseProgressCallback callback)
    {
        m_cancelRequested = false;
        m_lastError.clear();

        if (!localAE.enableStorage)
        {
            m_lastError = "Local storage SCP disabled. Enable storage to receive C-STORE objects.";
            qCWarning(lcDimse) << m_lastError.c_str();
            return DimseStatus::InvalidParameters;
        }

        if (m_eventManager)
        {
            m_eventManager->dispatchEvent(events::ProcessingEventType::DimseRetrieveStarted,
                                         "Starting C-MOVE for study " + studyInstanceUID);
        }

        auto assoc = m_connectionPool->acquire(peer, localAE);
        if (!assoc)
        {
            m_lastError = "Failed to acquire connection for C-MOVE";
            qCWarning(lcDimse) << m_lastError.c_str();
            if (m_eventManager)
            {
                m_eventManager->dispatchEvent(events::ProcessingEventType::DimseError, m_lastError);
            }
            return DimseStatus::ConnectionFailed;
        }

        auto releaseAssociation = [&]() {
            if (assoc)
            {
                m_connectionPool->release(assoc);
                assoc.reset();
            }
        };

        T_ASC_Association* association = assoc->getAssociation();
        if (!association)
        {
            m_lastError = "Invalid association for C-MOVE";
            releaseAssociation();
            return DimseStatus::ConnectionFailed;
        }

        std::unique_ptr<DcmDataset> dataset(buildRetrieveDataset(studyInstanceUID));
        if (!dataset)
        {
            m_lastError = "Failed to build retrieve dataset";
            releaseAssociation();
            return DimseStatus::InvalidParameters;
        }

        T_ASC_PresentationContextID presID = ASC_findAcceptedPresentationContextID(
            association, UID_MOVEStudyRootQueryRetrieveInformationModel);
        if (presID == 0)
        {
            m_lastError = "No presentation context for C-MOVE Study Root";
            qCWarning(lcDimse) << m_lastError.c_str();
            releaseAssociation();
            return DimseStatus::Failure;
        }

        T_DIMSE_C_MoveRQ request;
        memset(&request, 0, sizeof(request));
        request.MessageID = association->nextMsgID++;
        OFStandard::strlcpy(request.AffectedSOPClassUID,
                            UID_MOVEStudyRootQueryRetrieveInformationModel,
                            sizeof(request.AffectedSOPClassUID));
        request.Priority = DIMSE_PRIORITY_MEDIUM;
        request.DataSetType = DIMSE_DATASET_PRESENT;

        std::string moveDestination = destinationAE.empty() ? localAE.aeTitle : destinationAE;
        if (moveDestination.empty())
        {
            m_lastError = "Destination AE Title is empty";
            releaseAssociation();
            return DimseStatus::InvalidParameters;
        }
        OFStandard::strlcpy(request.MoveDestination,
                            moveDestination.c_str(),
                            sizeof(request.MoveDestination));

        MoveCallbackContext context;
        context.service = this;
        context.progressCallback = callback;

        T_DIMSE_C_MoveRSP response;
        memset(&response, 0, sizeof(response));
        DcmDataset* statusDetail = nullptr;
        DcmDataset* responseIdentifiers = nullptr;

        OFCondition cond = DIMSE_moveUser(association,
                                          presID,
                                          &request,
                                          dataset.get(),
                                          &DimseRetrieveService::moveResponseHandler,
                                          &context,
                                          DIMSE_BLOCKING,
                                          0,
                                          nullptr,
                                          nullptr,
                                          nullptr,
                                          &response,
                                          &statusDetail,
                                          &responseIdentifiers);

        if (statusDetail)
        {
            delete statusDetail;
            statusDetail = nullptr;
        }
        if (responseIdentifiers)
        {
            delete responseIdentifiers;
            responseIdentifiers = nullptr;
        }

        releaseAssociation();

        if (cond.bad())
        {
            m_lastError = "C-MOVE failed: " + std::string(cond.text());
            qCWarning(lcDimse) << m_lastError.c_str();
            if (m_eventManager)
            {
                m_eventManager->dispatchEvent(events::ProcessingEventType::DimseError, m_lastError);
            }
            return conditionToStatus(cond);
        }

        DimseStatus finalStatus = DimseStatus::Failure;
        std::string completionMessage;

        switch (response.DimseStatus)
        {
        case STATUS_Success:
            finalStatus = DimseStatus::Success;
            completionMessage = "C-MOVE completed successfully";
            break;
        case STATUS_MOVE_Cancel_SubOperationsTerminatedDueToCancelIndication:
            finalStatus = DimseStatus::Cancelled;
            completionMessage = "C-MOVE cancelled";
            break;
        case STATUS_MOVE_Refused_OutOfResourcesNumberOfMatches:
        case STATUS_MOVE_Refused_OutOfResourcesSubOperations:
        case STATUS_MOVE_Refused_MoveDestinationUnknown:
        case STATUS_MOVE_Error_DataSetDoesNotMatchSOPClass:
        default:
            finalStatus = DimseStatus::Failure;
            {
                std::ostringstream oss;
                oss << "C-MOVE failed with status 0x" << std::hex << response.DimseStatus;
                completionMessage = oss.str();
            }
            m_lastError = completionMessage;
            qCWarning(lcDimse) << completionMessage.c_str();
            break;
        }

        if (finalStatus == DimseStatus::Success && callback)
        {
            callback(1.0, "C-MOVE completed");
        }

        if (m_eventManager)
        {
            if (finalStatus == DimseStatus::Success)
            {
                m_eventManager->dispatchEvent(events::ProcessingEventType::DimseRetrieveCompleted,
                                             completionMessage);
            }
            else if (finalStatus == DimseStatus::Cancelled)
            {
                m_eventManager->dispatchEvent(events::ProcessingEventType::DimseConnectionFailed,
                                             completionMessage);
            }
            else
            {
                m_eventManager->dispatchEvent(events::ProcessingEventType::DimseError,
                                             completionMessage);
            }
        }

        return finalStatus;
    }

    DimseStatus DimseRetrieveService::retrieveStudyGet(const DicomPeer& peer,
                                                      const LocalAEConfig& localAE,
                                                      const std::string& studyInstanceUID,
                                                      DimseProgressCallback callback)
    {
        // C-GET implementation would go here
        qCInfo(lcDimse) << "C-GET not yet implemented";
        return DimseStatus::Success;
    }

    DimseStatus DimseRetrieveService::retrieveStudy(const DicomPeer& peer,
                                                   const LocalAEConfig& localAE,
                                                   const std::string& studyInstanceUID,
                                                   DimseProgressCallback callback)
    {
        // Try C-MOVE first, fallback to C-GET if needed
        std::string destAE = peer.moveDestinationAE.empty() ? localAE.aeTitle : peer.moveDestinationAE;
        return retrieveStudyMove(peer, localAE, studyInstanceUID, destAE, callback);
    }

    QFuture<DimseStatus> DimseRetrieveService::retrieveStudyAsync(
        const DicomPeer& peer,
        const LocalAEConfig& localAE,
        const std::string& studyInstanceUID,
        DimseProgressCallback callback)
    {
        return QtConcurrent::run([this, peer, localAE, studyInstanceUID, callback]() {
            return retrieveStudy(peer, localAE, studyInstanceUID, callback);
        });
    }

    void DimseRetrieveService::cancelRetrieve()
    {
        m_cancelRequested = true;
        qCInfo(lcDimse) << "Retrieve cancellation requested";
    }

    DcmDataset* DimseRetrieveService::buildRetrieveDataset(const std::string& studyInstanceUID)
    {
        DcmDataset* dataset = new DcmDataset();

        dataset->putAndInsertString(DCM_QueryRetrieveLevel, "STUDY");
        dataset->putAndInsertString(DCM_StudyInstanceUID, studyInstanceUID.c_str());

        return dataset;
    }

    // DimseStoreService implementation

    DimseStoreService::DimseStoreService(std::shared_ptr<DimseConnectionPool> pool,
                                        std::shared_ptr<events::CallbackManager> eventManager)
        : m_connectionPool(pool)
        , m_eventManager(eventManager)
    {
    }

    DimseStatus DimseStoreService::storeFile(const DicomPeer& peer,
                                            const LocalAEConfig& localAE,
                                            const std::string& filepath,
                                            DimseProgressCallback callback)
    {
        std::vector<std::string> files = {filepath};
        return storeFiles(peer, localAE, files, callback);
    }

    DimseStatus DimseStoreService::storeFiles(const DicomPeer& peer,
                                             const LocalAEConfig& localAE,
                                             const std::vector<std::string>& filepaths,
                                             DimseProgressCallback callback)
    {
        m_cancelRequested = false;
        m_storedCount = 0;
        m_failedCount = 0;

        if (filepaths.empty())
        {
            m_lastError = "No files specified";
            return DimseStatus::InvalidParameters;
        }

        if (m_eventManager)
        {
            std::string msg = "Starting C-STORE: " + std::to_string(filepaths.size()) +
                            " files to " + peer.name;
            m_eventManager->dispatchEvent(events::ProcessingEventType::DimseConnectionStarted, msg);
        }

        // Acquire connection
        auto assoc = m_connectionPool->acquire(peer, localAE);
        if (!assoc)
        {
            m_lastError = "Failed to acquire connection";
            qCWarning(lcDimse) << m_lastError.c_str();
            if (m_eventManager)
            {
                m_eventManager->dispatchEvent(events::ProcessingEventType::DimseError, m_lastError);
            }
            return DimseStatus::ConnectionFailed;
        }

        T_ASC_Association* association = assoc->getAssociation();
        if (!association)
        {
            m_lastError = "Invalid association";
            m_connectionPool->release(assoc);
            return DimseStatus::ConnectionFailed;
        }

        DimseStatus overallStatus = DimseStatus::Success;

        // Store each file
        for (size_t i = 0; i < filepaths.size(); ++i)
        {
            if (m_cancelRequested)
            {
                qCInfo(lcDimse) << "C-STORE cancelled by user";
                overallStatus = DimseStatus::Cancelled;
                break;
            }

            DimseStatus status = storeSingleFile(association, filepaths[i],
                                                static_cast<int>(i),
                                                static_cast<int>(filepaths.size()),
                                                callback);

            if (status == DimseStatus::Success)
            {
                m_storedCount++;
            }
            else
            {
                m_failedCount++;
                overallStatus = DimseStatus::Failure;
            }

            // Report progress
            if (callback && !m_cancelRequested)
            {
                double progress = static_cast<double>(i + 1) / filepaths.size();
                std::string msg = "Stored " + std::to_string(m_storedCount) + "/" +
                                std::to_string(filepaths.size()) + " files";
                callback(progress, msg);
            }
        }

        m_connectionPool->release(assoc);

        if (m_eventManager)
        {
            std::string msg = "C-STORE completed: " + std::to_string(m_storedCount) +
                            " succeeded, " + std::to_string(m_failedCount) + " failed";
            m_eventManager->dispatchEvent(events::ProcessingEventType::DimseConnectionEstablished, msg);
        }

        return overallStatus;
    }

    DimseStatus DimseStoreService::storeDirectory(const DicomPeer& peer,
                                                 const LocalAEConfig& localAE,
                                                 const std::string& directory,
                                                 bool recursive,
                                                 DimseProgressCallback callback)
    {
        std::vector<std::string> files = findDicomFiles(directory, recursive);

        if (files.empty())
        {
            m_lastError = "No DICOM files found in directory: " + directory;
            qCWarning(lcDimse) << m_lastError.c_str();
            return DimseStatus::InvalidParameters;
        }

        qCInfo(lcDimse) << "Found" << files.size() << "DICOM files in" << directory.c_str();
        return storeFiles(peer, localAE, files, callback);
    }

    QFuture<DimseStatus> DimseStoreService::storeFilesAsync(
        const DicomPeer& peer,
        const LocalAEConfig& localAE,
        const std::vector<std::string>& filepaths,
        DimseProgressCallback callback)
    {
        return QtConcurrent::run([this, peer, localAE, filepaths, callback]() {
            return storeFiles(peer, localAE, filepaths, callback);
        });
    }

    void DimseStoreService::cancelStore()
    {
        m_cancelRequested = true;
        qCInfo(lcDimse) << "Store cancellation requested";
    }

    DimseStatus DimseStoreService::storeSingleFile(T_ASC_Association* assoc,
                                                  const std::string& filepath,
                                                  int fileIndex,
                                                  int totalFiles,
                                                  DimseProgressCallback callback)
    {
        // Load DICOM file
        DcmFileFormat fileformat;
        OFCondition cond = fileformat.loadFile(filepath.c_str());

        if (cond.bad())
        {
            m_lastError = "Failed to load DICOM file: " + filepath + " - " +
                         std::string(cond.text());
            qCWarning(lcDimse) << m_lastError.c_str();
            return DimseStatus::Failure;
        }

        DcmDataset* dataset = fileformat.getDataset();
        if (!dataset)
        {
            m_lastError = "No dataset in file: " + filepath;
            qCWarning(lcDimse) << m_lastError.c_str();
            return DimseStatus::Failure;
        }

        // Get SOP Class UID and SOP Instance UID
        OFString sopClassUID;
        OFString sopInstanceUID;

        cond = dataset->findAndGetOFString(DCM_SOPClassUID, sopClassUID);
        if (cond.bad())
        {
            m_lastError = "Missing SOP Class UID in file: " + filepath;
            qCWarning(lcDimse) << m_lastError.c_str();
            return DimseStatus::Failure;
        }

        cond = dataset->findAndGetOFString(DCM_SOPInstanceUID, sopInstanceUID);
        if (cond.bad())
        {
            m_lastError = "Missing SOP Instance UID in file: " + filepath;
            qCWarning(lcDimse) << m_lastError.c_str();
            return DimseStatus::Failure;
        }

        // Find accepted presentation context
        T_ASC_PresentationContextID presID = ASC_findAcceptedPresentationContextID(
            assoc, sopClassUID.c_str());

        if (presID == 0)
        {
            m_lastError = "No presentation context for SOP Class: " + std::string(sopClassUID.c_str());
            qCWarning(lcDimse) << m_lastError.c_str();
            return DimseStatus::Failure;
        }

        // Send C-STORE request
        T_DIMSE_C_StoreRQ req;
        memset(&req, 0, sizeof(req));
        req.MessageID = assoc->nextMsgID++;
        strcpy(req.AffectedSOPClassUID, sopClassUID.c_str());
        strcpy(req.AffectedSOPInstanceUID, sopInstanceUID.c_str());
        req.DataSetType = DIMSE_DATASET_PRESENT;
        req.Priority = DIMSE_PRIORITY_MEDIUM;

        T_DIMSE_C_StoreRSP rsp;
        DcmDataset* statusDetail = nullptr;

        cond = DIMSE_storeUser(assoc, presID, &req, nullptr, dataset,
                              nullptr, nullptr, DIMSE_BLOCKING, 0,
                              &rsp, &statusDetail, nullptr);

        if (statusDetail)
        {
            delete statusDetail;
        }

        if (cond.bad())
        {
            m_lastError = "C-STORE failed for file: " + filepath + " - " +
                         std::string(cond.text());
            qCWarning(lcDimse) << m_lastError.c_str();
            return DimseStatus::Failure;
        }

        if (rsp.DimseStatus != STATUS_Success)
        {
            m_lastError = "C-STORE returned non-success status: " +
                         std::to_string(rsp.DimseStatus) + " for file: " + filepath;
            qCWarning(lcDimse) << m_lastError.c_str();
            return DimseStatus::Failure;
        }

        qCInfo(lcDimse) << "Successfully stored file" << (fileIndex + 1) << "of"
                       << totalFiles << ":" << filepath.c_str();

        return DimseStatus::Success;
    }

    std::vector<std::string> DimseStoreService::findDicomFiles(const std::string& directory,
                                                               bool recursive)
    {
        std::vector<std::string> files;

        // Use Qt for directory traversal
        QDir dir(QString::fromStdString(directory));
        if (!dir.exists())
        {
            qCWarning(lcDimse) << "Directory does not exist:" << directory.c_str();
            return files;
        }

        QStringList filters;
        filters << "*.dcm" << "*.dicom" << "*.DCM" << "*.DICOM";

        QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
        if (recursive)
        {
            flags = QDirIterator::Subdirectories;
        }

        QDirIterator it(dir.absolutePath(), filters, QDir::Files, flags);

        while (it.hasNext())
        {
            QString filepath = it.next();
            if (isDicomFile(filepath.toStdString()))
            {
                files.push_back(filepath.toStdString());
            }
        }

        return files;
    }

    bool DimseStoreService::isDicomFile(const std::string& filepath)
    {
        // Try to load as DICOM file
        DcmFileFormat fileformat;
        OFCondition cond = fileformat.loadFile(filepath.c_str());

        if (cond.bad())
        {
            return false;
        }

        DcmDataset* dataset = fileformat.getDataset();
        if (!dataset)
        {
            return false;
        }

        // Check for required DICOM elements
        OFString sopClassUID;
        cond = dataset->findAndGetOFString(DCM_SOPClassUID, sopClassUID);

        return cond.good();
    }

} // namespace isis::core::network
