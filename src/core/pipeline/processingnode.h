/*
 * ------------------------------------------------------------------------------------
 *  File: processingnode.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Base processing node for configurable pipeline
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <string>
#include <memory>
#include <vector>
#include <map>

namespace isis::core::pipeline
{
    /**
     * @brief Processing node status
     */
    enum class NodeStatus
    {
        Ready,
        Processing,
        Completed,
        Failed
    };

    /**
     * @brief Base class for processing pipeline nodes
     */
    class ProcessingNode
    {
    public:
        explicit ProcessingNode(const std::string& name);
        virtual ~ProcessingNode() = default;

        /**
         * @brief Execute the processing operation
         * @param input Input image
         * @return Output image
         */
        virtual vtkSmartPointer<vtkImageData> execute(vtkImageData* input) = 0;

        /**
         * @brief Get node name
         */
        [[nodiscard]] std::string getName() const { return m_name; }

        /**
         * @brief Set node name
         */
        void setName(const std::string& name) { m_name = name; }

        /**
         * @brief Get node status
         */
        [[nodiscard]] NodeStatus getStatus() const { return m_status; }

        /**
         * @brief Get last error message
         */
        [[nodiscard]] std::string getErrorMessage() const { return m_errorMessage; }

        /**
         * @brief Set a parameter value
         */
        void setParameter(const std::string& key, double value);

        /**
         * @brief Get a parameter value
         */
        [[nodiscard]] double getParameter(const std::string& key, double defaultValue = 0.0) const;

        /**
         * @brief Check if node has parameter
         */
        [[nodiscard]] bool hasParameter(const std::string& key) const;

        /**
         * @brief Get all parameter names
         */
        [[nodiscard]] std::vector<std::string> getParameterNames() const;

        /**
         * @brief Enable or disable node
         */
        void setEnabled(bool enabled) { m_enabled = enabled; }

        /**
         * @brief Check if node is enabled
         */
        [[nodiscard]] bool isEnabled() const { return m_enabled; }

    protected:
        void setStatus(NodeStatus status) { m_status = status; }
        void setError(const std::string& message);

    private:
        std::string m_name;
        NodeStatus m_status = NodeStatus::Ready;
        std::string m_errorMessage;
        std::map<std::string, double> m_parameters;
        bool m_enabled = true;
    };

    /**
     * @brief Convenience type alias
     */
    using ProcessingNodePtr = std::shared_ptr<ProcessingNode>;

} // namespace isis::core::pipeline
