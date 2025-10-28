/*
 * ------------------------------------------------------------------------------------
 *  File: callbackmanager.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Callback registration and event dispatching manager
 *      for real-time processing events and analysis
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include "../utils.h"
#include "processingcallback.h"
#include <vector>
#include <memory>
#include <mutex>
#include <map>

namespace isis::core::events
{
    /**
     * @brief Manager for callback registration and event dispatching
     */
    class export CallbackManager
    {
    public:
        CallbackManager() = default;
        ~CallbackManager() = default;

        // Prevent copying
        CallbackManager(const CallbackManager&) = delete;
        CallbackManager& operator=(const CallbackManager&) = delete;

        /**
         * @brief Register a callback
         * @param callback Callback to register
         * @return Callback ID for later removal
         */
        int registerCallback(std::shared_ptr<IProcessingCallback> callback);

        /**
         * @brief Register a function callback
         * @param func Function to call
         * @param name Callback name
         * @return Callback ID
         */
        int registerFunction(std::function<void(const ProcessingEventData&)> func,
                            const std::string& name = "");

        /**
         * @brief Register a filtered callback for specific event types
         * @param func Function to call
         * @param eventTypes Event types to listen for
         * @param name Callback name
         * @return Callback ID
         */
        int registerFilteredCallback(std::function<void(const ProcessingEventData&)> func,
                                     const std::vector<ProcessingEventType>& eventTypes,
                                     const std::string& name = "");

        /**
         * @brief Register a progress callback
         * @param func Progress function
         * @param name Callback name
         * @return Callback ID
         */
        int registerProgressCallback(std::function<void(double, const std::string&)> func,
                                     const std::string& name = "");

        /**
         * @brief Unregister a callback
         * @param callbackId Callback ID
         */
        void unregisterCallback(int callbackId);

        /**
         * @brief Clear all callbacks
         */
        void clearCallbacks();

        /**
         * @brief Dispatch an event to all registered callbacks
         * @param eventData Event data
         */
        void dispatchEvent(const ProcessingEventData& eventData);

        /**
         * @brief Dispatch a simple event by type
         * @param type Event type
         */
        void dispatchEvent(ProcessingEventType type);

        /**
         * @brief Dispatch an event with message
         * @param type Event type
         * @param message Message
         */
        void dispatchEvent(ProcessingEventType type, const std::string& message);

        /**
         * @brief Dispatch a progress event
         * @param type Event type
         * @param progress Progress value (0.0 to 1.0)
         * @param message Optional message
         */
        void dispatchProgress(ProcessingEventType type, double progress,
                            const std::string& message = "");

        /**
         * @brief Get number of registered callbacks
         */
        [[nodiscard]] size_t getCallbackCount() const;

        /**
         * @brief Enable or disable event dispatching
         */
        void setEnabled(bool enabled);

        /**
         * @brief Check if event dispatching is enabled
         */
        [[nodiscard]] bool isEnabled() const;

        /**
         * @brief Get statistics about dispatched events
         */
        [[nodiscard]] std::map<ProcessingEventType, size_t> getEventStatistics() const;

        /**
         * @brief Reset event statistics
         */
        void resetStatistics();

    private:
        struct CallbackEntry
        {
            int id;
            std::shared_ptr<IProcessingCallback> callback;
        };

        std::vector<CallbackEntry> m_callbacks;
        mutable std::mutex m_mutex;
        int m_nextCallbackId = 1;
        bool m_enabled = true;
        std::map<ProcessingEventType, size_t> m_eventStatistics;
    };

} // namespace isis::core::events
