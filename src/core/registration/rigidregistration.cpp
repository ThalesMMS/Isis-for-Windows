/*
 * ------------------------------------------------------------------------------------
 *  File: rigidregistration.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of rigid registration
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "rigidregistration.h"
#include <vtkImageReslice.h>
#include <algorithm>

namespace isis::core::registration
{
    RigidRegistration::RigidRegistration()
    {
        m_transform = vtkSmartPointer<vtkTransform>::New();
    }

    void RigidRegistration::setFixedImage(vtkImageData* fixedImage)
    {
        if (fixedImage)
        {
            m_fixedImage = fixedImage;
        }
    }

    void RigidRegistration::setMovingImage(vtkImageData* movingImage)
    {
        if (movingImage)
        {
            m_movingImage = movingImage;
        }
    }

    void RigidRegistration::setSimilarityMetric(SimilarityMetric metric)
    {
        m_metric = metric;
    }

    void RigidRegistration::setMaxIterations(int iterations)
    {
        m_maxIterations = iterations;
    }

    void RigidRegistration::setLearningRate(double rate)
    {
        // Prevent degenerate optimizer step sizes
        m_learningRate = std::max(rate, 1e-6);
    }

    void RigidRegistration::initializeTransform()
    {
        // Initialize with identity transform
        m_transform->Identity();

        // Center the transform at the image center
        if (m_fixedImage)
        {
            double center[3];
            double* bounds = m_fixedImage->GetBounds();
            center[0] = (bounds[0] + bounds[1]) / 2.0;
            center[1] = (bounds[2] + bounds[3]) / 2.0;
            center[2] = (bounds[4] + bounds[5]) / 2.0;

            m_transform->PostMultiply();
            m_transform->Translate(-center[0], -center[1], -center[2]);
        }
    }

    void RigidRegistration::optimizeTransform()
    {
        // Simplified optimization placeholder
        // In a full implementation, this would use gradient descent or other optimization
        // algorithms to minimize the similarity metric

        // For now, just compute initial similarity
        m_qualityMetric = computeSimilarity();
    }

    double RigidRegistration::computeSimilarity()
    {
        // Placeholder for similarity computation
        // Would implement actual metrics (MSE, NCC, MI) here
        return 0.0;
    }

    vtkSmartPointer<vtkImageData> RigidRegistration::execute()
    {
        if (!m_fixedImage || !m_movingImage)
        {
            return nullptr;
        }

        // Initialize transform
        initializeTransform();

        // Optimize transform parameters
        optimizeTransform();

        // Apply transform to moving image
        auto reslice = vtkSmartPointer<vtkImageReslice>::New();
        reslice->SetInputData(m_movingImage);
        reslice->SetResliceTransform(m_transform);
        reslice->SetInterpolationModeToLinear();
        reslice->Update();

        m_registeredImage = reslice->GetOutput();
        return m_registeredImage;
    }

    vtkTransform* RigidRegistration::getTransform() const
    {
        return m_transform;
    }

    double RigidRegistration::getQualityMetric() const
    {
        return m_qualityMetric;
    }

    double RigidRegistration::getFinalMetricValue() const
    {
        return m_qualityMetric;
    }

} // namespace isis::core::registration
