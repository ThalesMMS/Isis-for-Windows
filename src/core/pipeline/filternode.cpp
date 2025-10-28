/*
 * ------------------------------------------------------------------------------------
 *  File: filternode.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of filter processing nodes
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "filternode.h"

namespace isis::core::pipeline
{
    // EdgeEnhancementNode

    EdgeEnhancementNode::EdgeEnhancementNode()
        : ProcessingNode("Edge Enhancement")
    {
        setParameter("sigma", 1.0);
        setParameter("strength", 0.5);
    }

    vtkSmartPointer<vtkImageData> EdgeEnhancementNode::execute(vtkImageData* input)
    {
        if (!input)
        {
            setError("Invalid input image");
            return nullptr;
        }

        setStatus(NodeStatus::Processing);

        m_filter.setInputImage(input);
        m_filter.setSigma(getParameter("sigma", 1.0));
        m_filter.setStrength(getParameter("strength", 0.5));

        auto result = m_filter.execute();

        if (result)
        {
            setStatus(NodeStatus::Completed);
        }
        else
        {
            setError("Edge enhancement failed");
        }

        return result;
    }

    void EdgeEnhancementNode::setMethod(filters::EdgeDetectionMethod method)
    {
        m_filter.setMethod(method);
    }

    void EdgeEnhancementNode::setSigma(double sigma)
    {
        setParameter("sigma", sigma);
        m_filter.setSigma(sigma);
    }

    void EdgeEnhancementNode::setStrength(double strength)
    {
        setParameter("strength", strength);
        m_filter.setStrength(strength);
    }

    // NoiseReductionNode

    NoiseReductionNode::NoiseReductionNode()
        : ProcessingNode("Noise Reduction")
    {
        setParameter("radius", 1.0);
        setParameter("sigma", 1.0);
        setParameter("iterations", 5.0);
    }

    vtkSmartPointer<vtkImageData> NoiseReductionNode::execute(vtkImageData* input)
    {
        if (!input)
        {
            setError("Invalid input image");
            return nullptr;
        }

        setStatus(NodeStatus::Processing);

        m_filter.setInputImage(input);
        m_filter.setRadius(getParameter("radius", 1.0));
        m_filter.setSigma(getParameter("sigma", 1.0));
        m_filter.setIterations(static_cast<int>(getParameter("iterations", 5.0)));

        auto result = m_filter.execute();

        if (result)
        {
            setStatus(NodeStatus::Completed);
        }
        else
        {
            setError("Noise reduction failed");
        }

        return result;
    }

    void NoiseReductionNode::setMethod(filters::NoiseReductionMethod method)
    {
        m_filter.setMethod(method);
    }

    void NoiseReductionNode::setRadius(double radius)
    {
        setParameter("radius", radius);
        m_filter.setRadius(radius);
    }

    void NoiseReductionNode::setSigma(double sigma)
    {
        setParameter("sigma", sigma);
        m_filter.setSigma(sigma);
    }

    void NoiseReductionNode::setIterations(int iterations)
    {
        setParameter("iterations", static_cast<double>(iterations));
        m_filter.setIterations(iterations);
    }

    // MorphologyNode

    MorphologyNode::MorphologyNode()
        : ProcessingNode("Morphology")
    {
        setParameter("kernelRadius", 1.0);
    }

    vtkSmartPointer<vtkImageData> MorphologyNode::execute(vtkImageData* input)
    {
        if (!input)
        {
            setError("Invalid input image");
            return nullptr;
        }

        setStatus(NodeStatus::Processing);

        m_filter.setInputImage(input);
        m_filter.setKernelRadius(getParameter("kernelRadius", 1.0));

        auto result = m_filter.execute();

        if (result)
        {
            setStatus(NodeStatus::Completed);
        }
        else
        {
            setError("Morphology operation failed");
        }

        return result;
    }

    void MorphologyNode::setOperation(filters::MorphologyOperation operation)
    {
        m_filter.setOperation(operation);
    }

    void MorphologyNode::setStructuringElement(filters::StructuringElement element)
    {
        m_filter.setStructuringElement(element);
    }

    void MorphologyNode::setKernelRadius(double radius)
    {
        setParameter("kernelRadius", radius);
        m_filter.setKernelRadius(radius);
    }

} // namespace isis::core::pipeline
