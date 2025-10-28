/*
 * ------------------------------------------------------------------------------------
 *  File: edgeenhancementfilter.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of edge enhancement filters
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "edgeenhancementfilter.h"
#include <algorithm>
#include <vtkImageGradient.h>
#include <vtkImageMagnitude.h>
#include <vtkImageLaplacian.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkImageMathematics.h>
#include <vtkImageSobel3D.h>

namespace isis::core::filters
{
    void EdgeEnhancementFilter::setInputImage(vtkImageData* inputImage)
    {
        m_inputImage = inputImage;
    }

    void EdgeEnhancementFilter::setMethod(EdgeDetectionMethod method)
    {
        m_method = method;
    }

    void EdgeEnhancementFilter::setSigma(double sigma)
    {
        m_sigma = sigma;
    }

    void EdgeEnhancementFilter::setStrength(double strength)
    {
        m_strength = std::clamp(strength, 0.0, 1.0);
    }

    vtkSmartPointer<vtkImageData> EdgeEnhancementFilter::execute()
    {
        if (!m_inputImage)
        {
            return nullptr;
        }

        vtkSmartPointer<vtkImageData> edgeImage;

        switch (m_method)
        {
        case EdgeDetectionMethod::Sobel:
            edgeImage = applySobel();
            break;
        case EdgeDetectionMethod::Laplacian:
            edgeImage = applyLaplacian();
            break;
        case EdgeDetectionMethod::Gradient:
            edgeImage = applyGradient();
            break;
        case EdgeDetectionMethod::LaplacianOfGaussian:
            edgeImage = applyLaplacianOfGaussian();
            break;
        default:
            return nullptr;
        }

        if (!edgeImage)
        {
            return nullptr;
        }

        // Scale edge contribution before blending so strength actually controls enhancement
        auto edgeScaler = vtkSmartPointer<vtkImageMathematics>::New();
        edgeScaler->SetOperationToMultiplyByK();
        edgeScaler->SetInput1Data(edgeImage);
        edgeScaler->SetConstantK(m_strength);
        edgeScaler->Update();

        auto blend = vtkSmartPointer<vtkImageMathematics>::New();
        blend->SetOperationToAdd();
        blend->SetInput1Data(m_inputImage);
        blend->SetInput2Data(edgeScaler->GetOutput());
        blend->Update();

        return blend->GetOutput();
    }

    vtkSmartPointer<vtkImageData> EdgeEnhancementFilter::getEdgeMagnitude()
    {
        if (!m_inputImage)
        {
            return nullptr;
        }

        switch (m_method)
        {
        case EdgeDetectionMethod::Sobel:
            return applySobel();
        case EdgeDetectionMethod::Laplacian:
            return applyLaplacian();
        case EdgeDetectionMethod::Gradient:
            return applyGradient();
        case EdgeDetectionMethod::LaplacianOfGaussian:
            return applyLaplacianOfGaussian();
        default:
            return nullptr;
        }
    }

    vtkSmartPointer<vtkImageData> EdgeEnhancementFilter::applySobel()
    {
        auto sobel = vtkSmartPointer<vtkImageSobel3D>::New();
        sobel->SetInputData(m_inputImage);
        sobel->Update();

        return sobel->GetOutput();
    }

    vtkSmartPointer<vtkImageData> EdgeEnhancementFilter::applyLaplacian()
    {
        auto laplacian = vtkSmartPointer<vtkImageLaplacian>::New();
        laplacian->SetInputData(m_inputImage);
        laplacian->SetDimensionality(3);
        laplacian->Update();

        return laplacian->GetOutput();
    }

    vtkSmartPointer<vtkImageData> EdgeEnhancementFilter::applyGradient()
    {
        auto gradient = vtkSmartPointer<vtkImageGradient>::New();
        gradient->SetInputData(m_inputImage);
        gradient->SetDimensionality(3);
        gradient->Update();

        auto magnitude = vtkSmartPointer<vtkImageMagnitude>::New();
        magnitude->SetInputData(gradient->GetOutput());
        magnitude->Update();

        return magnitude->GetOutput();
    }

    vtkSmartPointer<vtkImageData> EdgeEnhancementFilter::applyLaplacianOfGaussian()
    {
        // First apply Gaussian smoothing
        auto gaussian = vtkSmartPointer<vtkImageGaussianSmooth>::New();
        gaussian->SetInputData(m_inputImage);
        gaussian->SetStandardDeviation(m_sigma);
        gaussian->SetRadiusFactor(2.0);
        gaussian->Update();

        // Then apply Laplacian
        auto laplacian = vtkSmartPointer<vtkImageLaplacian>::New();
        laplacian->SetInputData(gaussian->GetOutput());
        laplacian->SetDimensionality(3);
        laplacian->Update();

        return laplacian->GetOutput();
    }

} // namespace isis::core::filters
