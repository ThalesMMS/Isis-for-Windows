/*
 * ------------------------------------------------------------------------------------
 *  File: processingnode.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of processing node base class
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "processingnode.h"

namespace isis::core::pipeline
{
    ProcessingNode::ProcessingNode(const std::string& name)
        : m_name(name)
    {}

    void ProcessingNode::setError(const std::string& message)
    {
        m_status = NodeStatus::Failed;
        m_errorMessage = message;
    }

    void ProcessingNode::setParameter(const std::string& key, double value)
    {
        m_parameters[key] = value;
    }

    double ProcessingNode::getParameter(const std::string& key, double defaultValue) const
    {
        auto it = m_parameters.find(key);
        if (it != m_parameters.end())
        {
            return it->second;
        }
        return defaultValue;
    }

    bool ProcessingNode::hasParameter(const std::string& key) const
    {
        return m_parameters.find(key) != m_parameters.end();
    }

    std::vector<std::string> ProcessingNode::getParameterNames() const
    {
        std::vector<std::string> names;
        names.reserve(m_parameters.size());
        for (const auto& [key, value] : m_parameters)
        {
            names.push_back(key);
        }
        return names;
    }

} // namespace isis::core::pipeline
