/*
 * ------------------------------------------------------------------------------------
 *  File: thresholdsegmentation.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of threshold-based segmentation
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "thresholdsegmentation.h"

namespace isis::core::segmentation
{
    ThresholdSegmentation::ThresholdSegmentation()
    {
        m_thresholdFilter = vtkSmartPointer<vtkImageThreshold>::New();
        m_thresholdFilter->SetInValue(255);  // Inside threshold = white
        m_thresholdFilter->SetOutValue(0);    // Outside threshold = black
    }

    void ThresholdSegmentation::setInputImage(vtkImageData* input)
    {
        if (input)
        {
            m_inputImage = input;
            m_thresholdFilter->SetInputData(input);
        }
    }

    void ThresholdSegmentation::setLowerThreshold(double threshold)
    {
        m_lowerThreshold = threshold;
        m_thresholdFilter->ThresholdBetween(m_lowerThreshold, m_upperThreshold);
    }

    void ThresholdSegmentation::setUpperThreshold(double threshold)
    {
        m_upperThreshold = threshold;
        m_thresholdFilter->ThresholdBetween(m_lowerThreshold, m_upperThreshold);
    }

    void ThresholdSegmentation::setThresholds(double lower, double upper)
    {
        m_lowerThreshold = lower;
        m_upperThreshold = upper;
        m_thresholdFilter->ThresholdBetween(m_lowerThreshold, m_upperThreshold);
    }

    vtkSmartPointer<vtkImageData> ThresholdSegmentation::execute()
    {
        if (!m_inputImage)
        {
            return nullptr;
        }

        m_thresholdFilter->Update();
        m_outputMask = m_thresholdFilter->GetOutput();

        return m_outputMask;
    }

    vtkImageData* ThresholdSegmentation::getOutput() const
    {
        return m_outputMask;
    }

} // namespace isis::core::segmentation
