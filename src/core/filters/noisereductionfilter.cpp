/*
 * ------------------------------------------------------------------------------------
 *  File: noisereductionfilter.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of noise reduction filters
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "noisereductionfilter.h"
#include <algorithm>
#include <vtkImageGaussianSmooth.h>
#include <vtkImageMedian3D.h>
#include <vtkImageAnisotropicDiffusion3D.h>

namespace isis::core::filters
{
    void NoiseReductionFilter::setInputImage(vtkImageData* inputImage)
    {
        m_inputImage = inputImage;
    }

    void NoiseReductionFilter::setMethod(NoiseReductionMethod method)
    {
        m_method = method;
    }

    void NoiseReductionFilter::setRadius(double radius)
    {
        m_radius = radius;
    }

    void NoiseReductionFilter::setSigma(double sigma)
    {
        m_sigma = sigma;
    }

    void NoiseReductionFilter::setIterations(int iterations)
    {
        m_iterations = iterations;
    }

    void NoiseReductionFilter::setTimeStep(double timeStep)
    {
        m_timeStep = std::clamp(timeStep, 0.0, 0.125);
    }

    void NoiseReductionFilter::setEdgeThreshold(double threshold)
    {
        m_edgeThreshold = threshold;
    }

    vtkSmartPointer<vtkImageData> NoiseReductionFilter::execute()
    {
        if (!m_inputImage)
        {
            return nullptr;
        }

        switch (m_method)
        {
        case NoiseReductionMethod::Gaussian:
            return applyGaussian();
        case NoiseReductionMethod::Median:
            return applyMedian();
        case NoiseReductionMethod::AnisotropicDiffusion:
            return applyAnisotropicDiffusion();
        case NoiseReductionMethod::BilateralLike:
            return applyBilateralLike();
        default:
            return nullptr;
        }
    }

    vtkSmartPointer<vtkImageData> NoiseReductionFilter::applyGaussian()
    {
        auto gaussian = vtkSmartPointer<vtkImageGaussianSmooth>::New();
        gaussian->SetInputData(m_inputImage);
        gaussian->SetStandardDeviation(m_sigma);
        gaussian->SetRadiusFactor(2.0);
        gaussian->Update();

        return gaussian->GetOutput();
    }

    vtkSmartPointer<vtkImageData> NoiseReductionFilter::applyMedian()
    {
        auto median = vtkSmartPointer<vtkImageMedian3D>::New();
        median->SetInputData(m_inputImage);

        // Convert radius to kernel size (must be odd)
        int kernelSize = static_cast<int>(m_radius * 2) + 1;
        median->SetKernelSize(kernelSize, kernelSize, kernelSize);

        median->Update();

        return median->GetOutput();
    }

    vtkSmartPointer<vtkImageData> NoiseReductionFilter::applyAnisotropicDiffusion()
    {
        auto diffusion = vtkSmartPointer<vtkImageAnisotropicDiffusion3D>::New();
        diffusion->SetInputData(m_inputImage);
        diffusion->SetNumberOfIterations(m_iterations);
        diffusion->SetDiffusionThreshold(m_edgeThreshold);
        diffusion->SetDiffusionFactor(m_timeStep);
        diffusion->Update();

        return diffusion->GetOutput();
    }

    vtkSmartPointer<vtkImageData> NoiseReductionFilter::applyBilateralLike()
    {
        // Bilateral-like filter using multiple Gaussian passes
        // This is an approximation; true bilateral would require custom implementation

        auto result = vtkSmartPointer<vtkImageData>::New();
        result->DeepCopy(m_inputImage);

        for (int i = 0; i < m_iterations; ++i)
        {
            auto gaussian = vtkSmartPointer<vtkImageGaussianSmooth>::New();
            gaussian->SetInputData(result);
            gaussian->SetStandardDeviation(m_sigma);
            gaussian->SetRadiusFactor(1.5);
            gaussian->Update();

            result = gaussian->GetOutput();
        }

        return result;
    }

} // namespace isis::core::filters
