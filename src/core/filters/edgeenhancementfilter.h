/*
 * ------------------------------------------------------------------------------------
 *  File: edgeenhancementfilter.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Edge enhancement and detection filters (Sobel, Laplacian, Canny-like)
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
     * @brief Edge detection method
     */
    enum class EdgeDetectionMethod
    {
        Sobel,
        Laplacian,
        Gradient,
        LaplacianOfGaussian
    };

    /**
     * @brief Edge enhancement filter for medical images
     */
    class EdgeEnhancementFilter
    {
    public:
        EdgeEnhancementFilter() = default;
        ~EdgeEnhancementFilter() = default;

        /**
         * @brief Set input image
         */
        void setInputImage(vtkImageData* inputImage);

        /**
         * @brief Set edge detection method
         */
        void setMethod(EdgeDetectionMethod method);

        /**
         * @brief Set Gaussian smoothing sigma (for LOG method)
         * @param sigma Standard deviation
         */
        void setSigma(double sigma);

        /**
         * @brief Set enhancement strength
         * @param strength Enhancement factor (0.0 to 1.0)
         */
        void setStrength(double strength);

        /**
         * @brief Execute edge enhancement
         * @return Enhanced image
         */
        vtkSmartPointer<vtkImageData> execute();

        /**
         * @brief Get edge magnitude image only
         * @return Edge magnitude
         */
        vtkSmartPointer<vtkImageData> getEdgeMagnitude();

    private:
        vtkSmartPointer<vtkImageData> applySobel();
        vtkSmartPointer<vtkImageData> applyLaplacian();
        vtkSmartPointer<vtkImageData> applyGradient();
        vtkSmartPointer<vtkImageData> applyLaplacianOfGaussian();

        vtkSmartPointer<vtkImageData> m_inputImage;
        EdgeDetectionMethod m_method = EdgeDetectionMethod::Sobel;
        double m_sigma = 1.0;
        double m_strength = 0.5;
    };

} // namespace isis::core::filters
