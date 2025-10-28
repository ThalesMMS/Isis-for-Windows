/*
 * ------------------------------------------------------------------------------------
 *  File: affineregistration.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of affine registration
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "affineregistration.h"
#include <vtkImageReslice.h>
#include <fstream>

namespace isis::core::registration
{
    AffineRegistration::AffineRegistration()
    {
        m_transform = vtkSmartPointer<vtkTransform>::New();
        m_matrix = vtkSmartPointer<vtkMatrix4x4>::New();
    }

    void AffineRegistration::setFixedImage(vtkImageData* fixedImage)
    {
        if (fixedImage)
        {
            m_fixedImage = fixedImage;
        }
    }

    void AffineRegistration::setMovingImage(vtkImageData* movingImage)
    {
        if (movingImage)
        {
            m_movingImage = movingImage;
        }
    }

    void AffineRegistration::setMaxIterations(int iterations)
    {
        m_maxIterations = iterations;
    }

    void AffineRegistration::initializeTransform()
    {
        m_transform->Identity();
        m_matrix->Identity();
    }

    void AffineRegistration::optimizeAffineParameters()
    {
        // Placeholder for affine optimization
        // Would implement full 12-parameter optimization here
    }

    vtkSmartPointer<vtkImageData> AffineRegistration::execute()
    {
        if (!m_fixedImage || !m_movingImage)
        {
            return nullptr;
        }

        initializeTransform();
        optimizeAffineParameters();

        // Apply affine transform
        auto reslice = vtkSmartPointer<vtkImageReslice>::New();
        reslice->SetInputData(m_movingImage);
        reslice->SetResliceTransform(m_transform);
        reslice->SetInterpolationModeToLinear();
        reslice->Update();

        m_registeredImage = reslice->GetOutput();

        // Store the transformation matrix
        m_transform->GetMatrix(m_matrix);

        return m_registeredImage;
    }

    vtkMatrix4x4* AffineRegistration::getTransformationMatrix() const
    {
        return m_matrix;
    }

    bool AffineRegistration::exportTransformMatrix(const char* filename) const
    {
        if (!m_matrix || !filename)
        {
            return false;
        }

        std::ofstream file(filename);
        if (!file.is_open())
        {
            return false;
        }

        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                file << m_matrix->GetElement(i, j);
                if (j < 3) file << " ";
            }
            file << "\n";
        }

        file.close();
        return true;
    }

} // namespace isis::core::registration
