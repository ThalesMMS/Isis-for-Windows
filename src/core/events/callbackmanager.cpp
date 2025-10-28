/*
 * ------------------------------------------------------------------------------------
 *  File: callbackmanager.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of callback manager
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "callbackmanager.h"
#include <algorithm>

namespace isis::core::events
{
    int CallbackManager::registerCallback(std::shared_ptr<IProcessingCallback> callback)
    {
        if (!callback)
        {
            return -1;
        }

        std::lock_guard<std::mutex> lock(m_mutex);

        int id = m_nextCallbackId++;
        m_callbacks.push_back({id, std::move(callback)});

        return id;
    }

    int CallbackManager::registerFunction(std::function<void(const ProcessingEventData&)> func,
                                         const std::string& name)
    {
        auto callback = std::make_shared<FunctionCallback>(std::move(func), name);
        return registerCallback(callback);
    }

    int CallbackManager::registerFilteredCallback(
        std::function<void(const ProcessingEventData&)> func,
        const std::vector<ProcessingEventType>& eventTypes,
        const std::string& name)
    {
        auto callback = std::make_shared<FilteredCallback>(std::move(func), eventTypes, name);
        return registerCallback(callback);
    }

    int CallbackManager::registerProgressCallback(
        std::function<void(double, const std::string&)> func,
        const std::string& name)
    {
        auto callback = std::make_shared<ProgressCallback>(std::move(func), name);
        return registerCallback(callback);
    }

    void CallbackManager::unregisterCallback(int callbackId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_callbacks.erase(
            std::remove_if(m_callbacks.begin(), m_callbacks.end(),
                          [callbackId](const CallbackEntry& entry) {
                              return entry.id == callbackId;
                          }),
            m_callbacks.end());
    }

    void CallbackManager::clearCallbacks()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callbacks.clear();
    }

    void CallbackManager::dispatchEvent(const ProcessingEventData& eventData)
    {
        std::vector<CallbackEntry> callbacksToInvoke;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_enabled)
            {
                return;
            }

            // Update statistics while holding the lock
            m_eventStatistics[eventData.type]++;
            callbacksToInvoke = m_callbacks;
        }

        // Execute callbacks outside the lock to avoid deadlocks and re-entrancy issues
        for (const auto& entry : callbacksToInvoke)
        {
            if (entry.callback && entry.callback->shouldExecute(eventData.type))
            {
                try
                {
                    entry.callback->execute(eventData);
                }
                catch (...)
                {
                    // Catch exceptions to prevent one callback from breaking others
                }
            }
        }
    }

    void CallbackManager::dispatchEvent(ProcessingEventType type)
    {
        ProcessingEventData eventData(type);
        dispatchEvent(eventData);
    }

    void CallbackManager::dispatchEvent(ProcessingEventType type, const std::string& message)
    {
        ProcessingEventData eventData(type, message);
        dispatchEvent(eventData);
    }

    void CallbackManager::dispatchProgress(ProcessingEventType type, double progress,
                                          const std::string& message)
    {
        ProcessingEventData eventData(type);
        eventData.progress = progress;
        eventData.message = message;
        dispatchEvent(eventData);
    }

    size_t CallbackManager::getCallbackCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_callbacks.size();
    }

    void CallbackManager::setEnabled(bool enabled)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_enabled = enabled;
    }

    bool CallbackManager::isEnabled() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_enabled;
    }

    std::map<ProcessingEventType, size_t> CallbackManager::getEventStatistics() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_eventStatistics;
    }

    void CallbackManager::resetStatistics()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_eventStatistics.clear();
    }

} // namespace isis::core::events
