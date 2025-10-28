/*
 * ------------------------------------------------------------------------------------
 *  File: processingevent.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Processing event types and event data structures for real-time
 *      callback system
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <string>
#include <any>

namespace isis::core::events
{
    /**
     * @brief Event types for processing callbacks
     */
    enum class ProcessingEventType
    {
        None,
        ImageLoaded,
        ImageProcessingStarted,
        ImageProcessingProgress,
        ImageProcessingCompleted,
        SegmentationStarted,
        SegmentationProgress,
        SegmentationCompleted,
        RegistrationStarted,
        RegistrationProgress,
        RegistrationCompleted,
        MeasurementCreated,
        MeasurementModified,
        MeasurementDeleted,
        FilterApplied,
        RenderingUpdate,
        ValidationError,
        CustomEvent,
        // DIMSE Network Events
        DimseConnectionStarted,
        DimseConnectionEstablished,
        DimseConnectionFailed,
        DimseEchoStarted,
        DimseEchoCompleted,
        DimseQueryStarted,
        DimseQueryProgress,
        DimseQueryCompleted,
        DimseRetrieveStarted,
        DimseRetrieveProgress,
        DimseRetrieveCompleted,
        DimseStorageReceived,
        DimseError
    };

    /**
     * @brief Event data structure
     */
    struct ProcessingEventData
    {
        ProcessingEventType type = ProcessingEventType::None;
        std::string message;
        double progress = 0.0;  // 0.0 to 1.0
        vtkSmartPointer<vtkImageData> imageData = nullptr;
        std::any customData;  // For type-specific data
        bool success = true;

        ProcessingEventData() = default;

        explicit ProcessingEventData(ProcessingEventType eventType)
            : type(eventType) {}

        ProcessingEventData(ProcessingEventType eventType, const std::string& msg)
            : type(eventType), message(msg) {}

        ProcessingEventData(ProcessingEventType eventType, double prog)
            : type(eventType), progress(prog) {}

        ProcessingEventData(ProcessingEventType eventType, vtkImageData* data)
            : type(eventType), imageData(data) {}
    };

    /**
     * @brief Get string representation of event type
     */
    inline std::string eventTypeToString(ProcessingEventType type)
    {
        switch (type)
        {
        case ProcessingEventType::None: return "None";
        case ProcessingEventType::ImageLoaded: return "ImageLoaded";
        case ProcessingEventType::ImageProcessingStarted: return "ImageProcessingStarted";
        case ProcessingEventType::ImageProcessingProgress: return "ImageProcessingProgress";
        case ProcessingEventType::ImageProcessingCompleted: return "ImageProcessingCompleted";
        case ProcessingEventType::SegmentationStarted: return "SegmentationStarted";
        case ProcessingEventType::SegmentationProgress: return "SegmentationProgress";
        case ProcessingEventType::SegmentationCompleted: return "SegmentationCompleted";
        case ProcessingEventType::RegistrationStarted: return "RegistrationStarted";
        case ProcessingEventType::RegistrationProgress: return "RegistrationProgress";
        case ProcessingEventType::RegistrationCompleted: return "RegistrationCompleted";
        case ProcessingEventType::MeasurementCreated: return "MeasurementCreated";
        case ProcessingEventType::MeasurementModified: return "MeasurementModified";
        case ProcessingEventType::MeasurementDeleted: return "MeasurementDeleted";
        case ProcessingEventType::FilterApplied: return "FilterApplied";
        case ProcessingEventType::RenderingUpdate: return "RenderingUpdate";
        case ProcessingEventType::ValidationError: return "ValidationError";
        case ProcessingEventType::CustomEvent: return "CustomEvent";
        case ProcessingEventType::DimseConnectionStarted: return "DimseConnectionStarted";
        case ProcessingEventType::DimseConnectionEstablished: return "DimseConnectionEstablished";
        case ProcessingEventType::DimseConnectionFailed: return "DimseConnectionFailed";
        case ProcessingEventType::DimseEchoStarted: return "DimseEchoStarted";
        case ProcessingEventType::DimseEchoCompleted: return "DimseEchoCompleted";
        case ProcessingEventType::DimseQueryStarted: return "DimseQueryStarted";
        case ProcessingEventType::DimseQueryProgress: return "DimseQueryProgress";
        case ProcessingEventType::DimseQueryCompleted: return "DimseQueryCompleted";
        case ProcessingEventType::DimseRetrieveStarted: return "DimseRetrieveStarted";
        case ProcessingEventType::DimseRetrieveProgress: return "DimseRetrieveProgress";
        case ProcessingEventType::DimseRetrieveCompleted: return "DimseRetrieveCompleted";
        case ProcessingEventType::DimseStorageReceived: return "DimseStorageReceived";
        case ProcessingEventType::DimseError: return "DimseError";
        default: return "Unknown";
        }
    }

} // namespace isis::core::events
