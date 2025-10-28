/*
 * ------------------------------------------------------------------------------------
 *  File: noisereductionfilter.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Noise reduction filters (Gaussian, Median, Anisotropic Diffusion)
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <vtkSmartPointer.h>
#include <vtkImageData.h>

namespace isis::core::filters
{
    /**
     * @brief Noise reduction method
     */
    enum class NoiseReductionMethod
    {
        Gaussian,
        Median,
        AnisotropicDiffusion,
        BilateralLike  // Approximation using multiple passes
    };

    /**
     * @brief Noise reduction filter for medical images
     *
     * Provides various denoising methods optimized for medical imaging,
     * preserving edges and important structures.
     */
    class NoiseReductionFilter
    {
    public:
        NoiseReductionFilter() = default;
        ~NoiseReductionFilter() = default;

        /**
         * @brief Set input image
         */
        void setInputImage(vtkImageData* inputImage);

        /**
         * @brief Set noise reduction method
         */
        void setMethod(NoiseReductionMethod method);

        /**
         * @brief Set filter strength/radius
         * @param radius Filter kernel radius (pixels)
         */
        void setRadius(double radius);

        /**
         * @brief Set standard deviation for Gaussian
         * @param sigma Standard deviation
         */
        void setSigma(double sigma);

        /**
         * @brief Set number of iterations for iterative methods
         * @param iterations Number of iterations
         */
        void setIterations(int iterations);

        /**
         * @brief Set time step for anisotropic diffusion
         * @param timeStep Time step (0.0 to 0.125)
         */
        void setTimeStep(double timeStep);

        /**
         * @brief Set edge preservation threshold
         * @param threshold Conductance threshold
         */
        void setEdgeThreshold(double threshold);

        /**
         * @brief Execute noise reduction
         * @return Filtered image
         */
        vtkSmartPointer<vtkImageData> execute();

    private:
        vtkSmartPointer<vtkImageData> applyGaussian();
        vtkSmartPointer<vtkImageData> applyMedian();
        vtkSmartPointer<vtkImageData> applyAnisotropicDiffusion();
        vtkSmartPointer<vtkImageData> applyBilateralLike();

        vtkSmartPointer<vtkImageData> m_inputImage;
        NoiseReductionMethod m_method = NoiseReductionMethod::Gaussian;
        double m_radius = 1.0;
        double m_sigma = 1.0;
        int m_iterations = 5;
        double m_timeStep = 0.0625;
        double m_edgeThreshold = 10.0;
    };

} // namespace isis::core::filters
