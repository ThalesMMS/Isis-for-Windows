/*
 * ------------------------------------------------------------------------------------
 *  File: processingpipeline.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of processing pipeline
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "processingpipeline.h"
#include <chrono>
#include <algorithm>

namespace isis::core::pipeline
{
    ProcessingPipeline::ProcessingPipeline() = default;

    void ProcessingPipeline::addNode(ProcessingNodePtr node)
    {
        if (node)
        {
            m_nodes.push_back(node);
        }
    }

    void ProcessingPipeline::insertNode(size_t index, ProcessingNodePtr node)
    {
        if (node && index <= m_nodes.size())
        {
            m_nodes.insert(m_nodes.begin() + static_cast<ptrdiff_t>(index), node);
        }
    }

    void ProcessingPipeline::removeNode(size_t index)
    {
        if (index < m_nodes.size())
        {
            m_nodes.erase(m_nodes.begin() + static_cast<ptrdiff_t>(index));
        }
    }

    void ProcessingPipeline::removeNode(const std::string& name)
    {
        m_nodes.erase(
            std::remove_if(m_nodes.begin(), m_nodes.end(),
                          [&name](const ProcessingNodePtr& node) {
                              return node && node->getName() == name;
                          }),
            m_nodes.end());
    }

    void ProcessingPipeline::clear()
    {
        m_nodes.clear();
        m_intermediateResults.clear();
    }

    size_t ProcessingPipeline::getNodeCount() const
    {
        return m_nodes.size();
    }

    ProcessingNodePtr ProcessingPipeline::getNode(size_t index) const
    {
        if (index < m_nodes.size())
        {
            return m_nodes[index];
        }
        return nullptr;
    }

    ProcessingNodePtr ProcessingPipeline::getNode(const std::string& name) const
    {
        auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
                              [&name](const ProcessingNodePtr& node) {
                                  return node && node->getName() == name;
                              });

        if (it != m_nodes.end())
        {
            return *it;
        }
        return nullptr;
    }

    vtkSmartPointer<vtkImageData> ProcessingPipeline::execute(vtkImageData* input)
    {
        if (!input || m_nodes.empty())
        {
            m_lastError = "Invalid input or empty pipeline";
            return nullptr;
        }

        m_executing = true;
        m_lastError.clear();
        m_intermediateResults.clear();
        m_intermediateResults.reserve(m_nodes.size());

        auto startTime = std::chrono::high_resolution_clock::now();

        vtkSmartPointer<vtkImageData> currentImage = input;

        for (size_t i = 0; i < m_nodes.size(); ++i)
        {
            const auto& node = m_nodes[i];

            if (!node || !node->isEnabled())
            {
                m_intermediateResults.push_back(currentImage);
                continue;
            }

            double progress = static_cast<double>(i) / static_cast<double>(m_nodes.size());
            notifyNodeStarted(node->getName());
            notifyProgress(progress, "Processing: " + node->getName());

            try
            {
                auto result = node->execute(currentImage);

                if (!result)
                {
                    m_lastError = "Node '" + node->getName() + "' failed: " + node->getErrorMessage();
                    m_executing = false;
                    return nullptr;
                }

                currentImage = result;
                m_intermediateResults.push_back(currentImage);
                notifyNodeCompleted(node->getName());
            }
            catch (const std::exception& e)
            {
                m_lastError = "Node '" + node->getName() + "' threw exception: " + e.what();
                m_executing = false;
                return nullptr;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        m_executionTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        notifyProgress(1.0, "Pipeline completed");
        m_executing = false;

        return currentImage;
    }

    vtkSmartPointer<vtkImageData> ProcessingPipeline::getIntermediateResult(size_t index) const
    {
        if (index < m_intermediateResults.size())
        {
            return m_intermediateResults[index];
        }
        return nullptr;
    }

    void ProcessingPipeline::setCallbackManager(std::shared_ptr<events::CallbackManager> callbackManager)
    {
        m_callbackManager = callbackManager;
    }

    double ProcessingPipeline::getExecutionTime() const
    {
        return m_executionTime;
    }

    bool ProcessingPipeline::isExecuting() const
    {
        return m_executing;
    }

    std::string ProcessingPipeline::getLastError() const
    {
        return m_lastError;
    }

    void ProcessingPipeline::notifyProgress(double progress, const std::string& message)
    {
        if (m_callbackManager)
        {
            m_callbackManager->dispatchProgress(
                events::ProcessingEventType::ImageProcessingProgress,
                progress, message);
        }
    }

    void ProcessingPipeline::notifyNodeStarted(const std::string& nodeName)
    {
        if (m_callbackManager)
        {
            m_callbackManager->dispatchEvent(
                events::ProcessingEventType::ImageProcessingStarted,
                "Starting: " + nodeName);
        }
    }

    void ProcessingPipeline::notifyNodeCompleted(const std::string& nodeName)
    {
        if (m_callbackManager)
        {
            m_callbackManager->dispatchEvent(
                events::ProcessingEventType::ImageProcessingCompleted,
                "Completed: " + nodeName);
        }
    }

} // namespace isis::core::pipeline
