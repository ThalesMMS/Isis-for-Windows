/*
 * ------------------------------------------------------------------------------------
 *  File: watershedsegmentation.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of watershed segmentation
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "watershedsegmentation.h"
#include <vtkImageGaussianSmooth.h>

namespace isis::core::segmentation
{
    WatershedSegmentation::WatershedSegmentation()
    {
        m_gradientFilter = vtkSmartPointer<vtkImageGradientMagnitude>::New();
    }

    void WatershedSegmentation::setInputImage(vtkImageData* input)
    {
        if (input)
        {
            m_inputImage = input;
        }
    }

    void WatershedSegmentation::setMarkers(vtkImageData* markers)
    {
        if (markers)
        {
            m_markersImage = markers;
        }
    }

    void WatershedSegmentation::setWatershedLevel(double level)
    {
        m_watershedLevel = level;
    }

    void WatershedSegmentation::setGradientPreprocessing(bool enable)
    {
        m_useGradientPreprocessing = enable;
    }

    vtkSmartPointer<vtkImageData> WatershedSegmentation::preprocessGradient()
    {
        if (!m_inputImage)
        {
            return nullptr;
        }

        // Smooth the image first
        auto smoothFilter = vtkSmartPointer<vtkImageGaussianSmooth>::New();
        smoothFilter->SetInputData(m_inputImage);
        smoothFilter->SetStandardDeviation(1.0);
        smoothFilter->Update();

        // Compute gradient magnitude
        m_gradientFilter->SetInputData(smoothFilter->GetOutput());
        m_gradientFilter->Update();

        return m_gradientFilter->GetOutput();
    }

    vtkSmartPointer<vtkImageData> WatershedSegmentation::execute()
    {
        if (!m_inputImage)
        {
            return nullptr;
        }

        vtkImageData* processedImage = m_inputImage;

        // Apply gradient preprocessing if enabled
        if (m_useGradientPreprocessing)
        {
            auto gradientImage = preprocessGradient();
            if (gradientImage)
            {
                processedImage = gradientImage;
            }
        }

        // Note: VTK doesn't have a built-in watershed filter
        // This is a placeholder that would need ITK/SimpleITK integration
        // For now, we return the gradient-processed image
        // In a full implementation, this would use ITK's WatershedImageFilter

        m_outputSegmentation = processedImage;
        return m_outputSegmentation;
    }

    vtkImageData* WatershedSegmentation::getOutput() const
    {
        return m_outputSegmentation;
    }

} // namespace isis::core::segmentation
