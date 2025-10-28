/*
 * ------------------------------------------------------------------------------------
 *  File: deformableregistration.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of deformable registration
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "deformableregistration.h"
#include <vtkImageGradient.h>
#include <vtkImageMagnitude.h>

namespace isis::core::registration
{
    DeformableRegistration::DeformableRegistration()
    {
        m_transform = vtkSmartPointer<vtkThinPlateSplineTransform>::New();
    }

    void DeformableRegistration::setFixedImage(vtkImageData* fixedImage)
    {
        if (fixedImage)
        {
            m_fixedImage = fixedImage;
        }
    }

    void DeformableRegistration::setMovingImage(vtkImageData* movingImage)
    {
        if (movingImage)
        {
            m_movingImage = movingImage;
        }
    }

    void DeformableRegistration::addControlPointPair(const double sourcePoint[3], const double targetPoint[3])
    {
        ControlPointPair pair;
        for (int i = 0; i < 3; ++i)
        {
            pair.sourcePoint[i] = sourcePoint[i];
            pair.targetPoint[i] = targetPoint[i];
        }
        m_controlPoints.push_back(pair);
    }

    void DeformableRegistration::clearControlPoints()
    {
        m_controlPoints.clear();
    }

    void DeformableRegistration::setRegularization(double lambda)
    {
        m_regularization = lambda;
    }

    void DeformableRegistration::buildTransform()
    {
        if (m_controlPoints.empty())
        {
            return;
        }

        auto sourcePoints = vtkSmartPointer<vtkPoints>::New();
        auto targetPoints = vtkSmartPointer<vtkPoints>::New();

        for (const auto& pair : m_controlPoints)
        {
            sourcePoints->InsertNextPoint(pair.sourcePoint);
            targetPoints->InsertNextPoint(pair.targetPoint);
        }

        m_transform->SetSourceLandmarks(sourcePoints);
        m_transform->SetTargetLandmarks(targetPoints);
        m_transform->SetBasisToR();  // Radial basis function

        // Apply regularization
        m_transform->SetSigma(m_regularization);

        m_transform->Update();
    }

    vtkSmartPointer<vtkImageData> DeformableRegistration::execute()
    {
        if (!m_fixedImage || !m_movingImage)
        {
            return nullptr;
        }

        buildTransform();

        // Apply deformable transform
        auto reslice = vtkSmartPointer<vtkImageReslice>::New();
        reslice->SetInputData(m_movingImage);
        reslice->SetResliceTransform(m_transform);
        reslice->SetInterpolationModeToCubic(); // Higher quality for deformable
        reslice->Update();

        m_registeredImage = reslice->GetOutput();

        // Compute deformation field
        computeDeformationField();

        return m_registeredImage;
    }

    void DeformableRegistration::computeDeformationField()
    {
        if (!m_fixedImage)
        {
            return;
        }

        // Create deformation field with same dimensions as fixed image
        m_deformationField = vtkSmartPointer<vtkImageData>::New();
        m_deformationField->SetDimensions(m_fixedImage->GetDimensions());
        m_deformationField->SetSpacing(m_fixedImage->GetSpacing());
        m_deformationField->SetOrigin(m_fixedImage->GetOrigin());
        m_deformationField->AllocateScalars(VTK_FLOAT, 3); // 3 components for vector field

        int* dims = m_fixedImage->GetDimensions();
        double* spacing = m_fixedImage->GetSpacing();
        double* origin = m_fixedImage->GetOrigin();

        for (int z = 0; z < dims[2]; ++z)
        {
            for (int y = 0; y < dims[1]; ++y)
            {
                for (int x = 0; x < dims[0]; ++x)
                {
                    double point[3];
                    point[0] = origin[0] + x * spacing[0];
                    point[1] = origin[1] + y * spacing[1];
                    point[2] = origin[2] + z * spacing[2];

                    double transformedPoint[3];
                    m_transform->TransformPoint(point, transformedPoint);

                    // Compute deformation vector
                    float* pixel = static_cast<float*>(
                        m_deformationField->GetScalarPointer(x, y, z));

                    pixel[0] = static_cast<float>(transformedPoint[0] - point[0]);
                    pixel[1] = static_cast<float>(transformedPoint[1] - point[1]);
                    pixel[2] = static_cast<float>(transformedPoint[2] - point[2]);
                }
            }
        }
    }

    vtkSmartPointer<vtkImageData> DeformableRegistration::getDeformationField()
    {
        return m_deformationField;
    }

    vtkThinPlateSplineTransform* DeformableRegistration::getTransform() const
    {
        return m_transform;
    }

    vtkSmartPointer<vtkImageData> DeformableRegistration::visualizeDeformationMagnitude()
    {
        if (!m_deformationField)
        {
            return nullptr;
        }

        auto magnitudeFilter = vtkSmartPointer<vtkImageMagnitude>::New();
        magnitudeFilter->SetInputData(m_deformationField);
        magnitudeFilter->Update();

        return magnitudeFilter->GetOutput();
    }

} // namespace isis::core::registration
