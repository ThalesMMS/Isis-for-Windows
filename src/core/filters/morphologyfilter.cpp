/*
 * ------------------------------------------------------------------------------------
 *  File: morphologyfilter.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of morphological operations
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "morphologyfilter.h"
#include <vtkImageDilateErode3D.h>
#include <vtkImageMathematics.h>

namespace isis::core::filters
{
    void MorphologyFilter::setInputImage(vtkImageData* inputImage)
    {
        m_inputImage = inputImage;
    }

    void MorphologyFilter::setOperation(MorphologyOperation operation)
    {
        m_operation = operation;
    }

    void MorphologyFilter::setStructuringElement(StructuringElement element)
    {
        m_structuringElement = element;
    }

    void MorphologyFilter::setKernelRadius(double radius)
    {
        m_kernelRadius = radius;
    }

    vtkSmartPointer<vtkImageData> MorphologyFilter::execute()
    {
        if (!m_inputImage)
        {
            return nullptr;
        }

        switch (m_operation)
        {
        case MorphologyOperation::Erode:
            return applyErosion();
        case MorphologyOperation::Dilate:
            return applyDilation();
        case MorphologyOperation::Open:
            return applyOpening();
        case MorphologyOperation::Close:
            return applyClosing();
        case MorphologyOperation::Gradient:
            return applyGradient();
        case MorphologyOperation::TopHat:
            return applyTopHat();
        case MorphologyOperation::BlackHat:
            return applyBlackHat();
        default:
            return nullptr;
        }
    }

    vtkSmartPointer<vtkImageData> MorphologyFilter::applyErosion()
    {
        auto erode = vtkSmartPointer<vtkImageDilateErode3D>::New();
        erode->SetInputData(m_inputImage);
        erode->SetKernelSize(getKernelSize(0), getKernelSize(1), getKernelSize(2));
        erode->SetErodeValue(1);
        erode->SetDilateValue(0);
        erode->Update();

        return erode->GetOutput();
    }

    vtkSmartPointer<vtkImageData> MorphologyFilter::applyDilation()
    {
        auto dilate = vtkSmartPointer<vtkImageDilateErode3D>::New();
        dilate->SetInputData(m_inputImage);
        dilate->SetKernelSize(getKernelSize(0), getKernelSize(1), getKernelSize(2));
        dilate->SetDilateValue(1);
        dilate->SetErodeValue(0);
        dilate->Update();

        return dilate->GetOutput();
    }

    vtkSmartPointer<vtkImageData> MorphologyFilter::applyOpening()
    {
        // Opening = Erosion followed by Dilation
        auto eroded = applyErosion();
        if (!eroded)
        {
            return nullptr;
        }

        auto dilate = vtkSmartPointer<vtkImageDilateErode3D>::New();
        dilate->SetInputData(eroded);
        dilate->SetKernelSize(getKernelSize(0), getKernelSize(1), getKernelSize(2));
        dilate->SetDilateValue(1);
        dilate->SetErodeValue(0);
        dilate->Update();

        return dilate->GetOutput();
    }

    vtkSmartPointer<vtkImageData> MorphologyFilter::applyClosing()
    {
        // Closing = Dilation followed by Erosion
        auto dilated = applyDilation();
        if (!dilated)
        {
            return nullptr;
        }

        auto erode = vtkSmartPointer<vtkImageDilateErode3D>::New();
        erode->SetInputData(dilated);
        erode->SetKernelSize(getKernelSize(0), getKernelSize(1), getKernelSize(2));
        erode->SetErodeValue(1);
        erode->SetDilateValue(0);
        erode->Update();

        return erode->GetOutput();
    }

    vtkSmartPointer<vtkImageData> MorphologyFilter::applyGradient()
    {
        // Morphological Gradient = Dilation - Erosion
        auto dilated = applyDilation();
        auto eroded = applyErosion();

        if (!dilated || !eroded)
        {
            return nullptr;
        }

        auto subtract = vtkSmartPointer<vtkImageMathematics>::New();
        subtract->SetOperationToSubtract();
        subtract->SetInput1Data(dilated);
        subtract->SetInput2Data(eroded);
        subtract->Update();

        return subtract->GetOutput();
    }

    vtkSmartPointer<vtkImageData> MorphologyFilter::applyTopHat()
    {
        // Top Hat = Original - Opening
        auto opened = applyOpening();
        if (!opened)
        {
            return nullptr;
        }

        auto subtract = vtkSmartPointer<vtkImageMathematics>::New();
        subtract->SetOperationToSubtract();
        subtract->SetInput1Data(m_inputImage);
        subtract->SetInput2Data(opened);
        subtract->Update();

        return subtract->GetOutput();
    }

    vtkSmartPointer<vtkImageData> MorphologyFilter::applyBlackHat()
    {
        // Black Hat = Closing - Original
        auto closed = applyClosing();
        if (!closed)
        {
            return nullptr;
        }

        auto subtract = vtkSmartPointer<vtkImageMathematics>::New();
        subtract->SetOperationToSubtract();
        subtract->SetInput1Data(closed);
        subtract->SetInput2Data(m_inputImage);
        subtract->Update();

        return subtract->GetOutput();
    }

    int MorphologyFilter::getKernelSize(int axis) const
    {
        int kernelSize = static_cast<int>(m_kernelRadius * 2) + 1;

        // Adjust based on structuring element
        switch (m_structuringElement)
        {
        case StructuringElement::Cross:
            // Cross uses thinner kernel
            return std::max(3, kernelSize - 2);
        case StructuringElement::Box:
        case StructuringElement::Sphere:
        default:
            return kernelSize;
        }
    }

} // namespace isis::core::filters
