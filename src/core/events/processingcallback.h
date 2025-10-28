/*
 * ------------------------------------------------------------------------------------
 *  File: processingcallback.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Base callback interface for processing events
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include "processingevent.h"
#include <functional>
#include <memory>

namespace isis::core::events
{
    /**
     * @brief Base callback interface
     */
    class IProcessingCallback
    {
    public:
        virtual ~IProcessingCallback() = default;

        /**
         * @brief Execute callback with event data
         * @param eventData Event data
         */
        virtual void execute(const ProcessingEventData& eventData) = 0;

        /**
         * @brief Get callback name for debugging
         */
        [[nodiscard]] virtual std::string getName() const = 0;

        /**
         * @brief Check if callback should be executed for event type
         * @param type Event type
         */
        [[nodiscard]] virtual bool shouldExecute(ProcessingEventType type) const = 0;
    };

    /**
     * @brief Function-based callback implementation
     */
    class FunctionCallback : public IProcessingCallback
    {
    public:
        using CallbackFunction = std::function<void(const ProcessingEventData&)>;

        explicit FunctionCallback(CallbackFunction func, const std::string& name = "FunctionCallback")
            : m_function(std::move(func)), m_name(name) {}

        void execute(const ProcessingEventData& eventData) override
        {
            if (m_function)
            {
                m_function(eventData);
            }
        }

        [[nodiscard]] std::string getName() const override
        {
            return m_name;
        }

        [[nodiscard]] bool shouldExecute(ProcessingEventType /*type*/) const override
        {
            return true;  // Execute for all events by default
        }

    private:
        CallbackFunction m_function;
        std::string m_name;
    };

    /**
     * @brief Filtered callback that only executes for specific event types
     */
    class FilteredCallback : public IProcessingCallback
    {
    public:
        using CallbackFunction = std::function<void(const ProcessingEventData&)>;

        FilteredCallback(CallbackFunction func,
                        std::vector<ProcessingEventType> eventTypes,
                        const std::string& name = "FilteredCallback")
            : m_function(std::move(func))
            , m_eventTypes(std::move(eventTypes))
            , m_name(name)
        {}

        void execute(const ProcessingEventData& eventData) override
        {
            if (m_function && shouldExecute(eventData.type))
            {
                m_function(eventData);
            }
        }

        [[nodiscard]] std::string getName() const override
        {
            return m_name;
        }

        [[nodiscard]] bool shouldExecute(ProcessingEventType type) const override
        {
            return std::find(m_eventTypes.begin(), m_eventTypes.end(), type) != m_eventTypes.end();
        }

    private:
        CallbackFunction m_function;
        std::vector<ProcessingEventType> m_eventTypes;
        std::string m_name;
    };

    /**
     * @brief Progress tracking callback
     */
    class ProgressCallback : public IProcessingCallback
    {
    public:
        using ProgressFunction = std::function<void(double progress, const std::string& message)>;

        explicit ProgressCallback(ProgressFunction func, const std::string& name = "ProgressCallback")
            : m_function(std::move(func)), m_name(name) {}

        void execute(const ProcessingEventData& eventData) override
        {
            if (m_function && isProgressEvent(eventData.type))
            {
                m_function(eventData.progress, eventData.message);
            }
        }

        [[nodiscard]] std::string getName() const override
        {
            return m_name;
        }

        [[nodiscard]] bool shouldExecute(ProcessingEventType type) const override
        {
            return isProgressEvent(type);
        }

    private:
        static bool isProgressEvent(ProcessingEventType type)
        {
            return type == ProcessingEventType::ImageProcessingProgress ||
                   type == ProcessingEventType::SegmentationProgress ||
                   type == ProcessingEventType::RegistrationProgress;
        }

        ProgressFunction m_function;
        std::string m_name;
    };

    /**
     * @brief Validation callback for error checking
     */
    class ValidationCallback : public IProcessingCallback
    {
    public:
        using ValidationFunction = std::function<bool(const ProcessingEventData&)>;

        explicit ValidationCallback(ValidationFunction func, const std::string& name = "ValidationCallback")
            : m_function(std::move(func)), m_name(name) {}

        void execute(const ProcessingEventData& eventData) override
        {
            if (m_function)
            {
                bool valid = m_function(eventData);
                if (!valid)
                {
                    // Could trigger validation error event
                }
            }
        }

        [[nodiscard]] std::string getName() const override
        {
            return m_name;
        }

        [[nodiscard]] bool shouldExecute(ProcessingEventType /*type*/) const override
        {
            return true;
        }

    private:
        ValidationFunction m_function;
        std::string m_name;
    };

} // namespace isis::core::events
