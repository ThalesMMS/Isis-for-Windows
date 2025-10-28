/*
 * ------------------------------------------------------------------------------------
 *  File: processingpipeline.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Configurable processing pipeline for chaining image operations
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include "processingnode.h"
#include "../events/callbackmanager.h"
#include <vector>
#include <memory>

namespace isis::core::pipeline
{
    /**
     * @brief Configurable image processing pipeline
     *
     * Allows chaining multiple processing operations together.
     * Supports callback notifications for progress tracking.
     */
    class ProcessingPipeline
    {
    public:
        ProcessingPipeline();
        ~ProcessingPipeline() = default;

        /**
         * @brief Add a processing node to the pipeline
         * @param node Node to add
         */
        void addNode(ProcessingNodePtr node);

        /**
         * @brief Insert a node at specific position
         * @param index Position to insert
         * @param node Node to insert
         */
        void insertNode(size_t index, ProcessingNodePtr node);

        /**
         * @brief Remove a node by index
         * @param index Node index
         */
        void removeNode(size_t index);

        /**
         * @brief Remove a node by name
         * @param name Node name
         */
        void removeNode(const std::string& name);

        /**
         * @brief Clear all nodes
         */
        void clear();

        /**
         * @brief Get number of nodes
         */
        [[nodiscard]] size_t getNodeCount() const;

        /**
         * @brief Get node by index
         */
        [[nodiscard]] ProcessingNodePtr getNode(size_t index) const;

        /**
         * @brief Get node by name
         */
        [[nodiscard]] ProcessingNodePtr getNode(const std::string& name) const;

        /**
         * @brief Execute the pipeline
         * @param input Input image
         * @return Final output image
         */
        vtkSmartPointer<vtkImageData> execute(vtkImageData* input);

        /**
         * @brief Get intermediate result from specific node
         * @param index Node index
         * @return Intermediate result
         */
        [[nodiscard]] vtkSmartPointer<vtkImageData> getIntermediateResult(size_t index) const;

        /**
         * @brief Set callback manager for progress notifications
         */
        void setCallbackManager(std::shared_ptr<events::CallbackManager> callbackManager);

        /**
         * @brief Get pipeline execution time in milliseconds
         */
        [[nodiscard]] double getExecutionTime() const;

        /**
         * @brief Check if pipeline is currently executing
         */
        [[nodiscard]] bool isExecuting() const;

        /**
         * @brief Get last error message
         */
        [[nodiscard]] std::string getLastError() const;

    private:
        void notifyProgress(double progress, const std::string& message);
        void notifyNodeStarted(const std::string& nodeName);
        void notifyNodeCompleted(const std::string& nodeName);

        std::vector<ProcessingNodePtr> m_nodes;
        std::vector<vtkSmartPointer<vtkImageData>> m_intermediateResults;
        std::shared_ptr<events::CallbackManager> m_callbackManager;
        double m_executionTime = 0.0;
        bool m_executing = false;
        std::string m_lastError;
    };

} // namespace isis::core::pipeline
