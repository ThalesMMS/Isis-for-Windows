/*
 * ------------------------------------------------------------------------------------
 *  File: morphologyfilter.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Morphological operations (erosion, dilation, opening, closing)
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
     * @brief Morphological operation type
     */
    enum class MorphologyOperation
    {
        Erode,
        Dilate,
        Open,      // Erosion followed by dilation
        Close,     // Dilation followed by erosion
        Gradient,  // Dilation - Erosion
        TopHat,    // Original - Opening
        BlackHat   // Closing - Original
    };

    /**
     * @brief Structuring element shape
     */
    enum class StructuringElement
    {
        Sphere,
        Box,
        Cross
    };

    /**
     * @brief Morphological filter for binary and grayscale operations
     */
    class MorphologyFilter
    {
    public:
        MorphologyFilter() = default;
        ~MorphologyFilter() = default;

        /**
         * @brief Set input image
         */
        void setInputImage(vtkImageData* inputImage);

        /**
         * @brief Set morphological operation
         */
        void setOperation(MorphologyOperation operation);

        /**
         * @brief Set structuring element
         */
        void setStructuringElement(StructuringElement element);

        /**
         * @brief Set kernel radius
         * @param radius Kernel radius in pixels
         */
        void setKernelRadius(double radius);

        /**
         * @brief Execute morphological operation
         * @return Filtered image
         */
        vtkSmartPointer<vtkImageData> execute();

    private:
        vtkSmartPointer<vtkImageData> applyErosion();
        vtkSmartPointer<vtkImageData> applyDilation();
        vtkSmartPointer<vtkImageData> applyOpening();
        vtkSmartPointer<vtkImageData> applyClosing();
        vtkSmartPointer<vtkImageData> applyGradient();
        vtkSmartPointer<vtkImageData> applyTopHat();
        vtkSmartPointer<vtkImageData> applyBlackHat();

        int getKernelSize(int axis) const;

        vtkSmartPointer<vtkImageData> m_inputImage;
        MorphologyOperation m_operation = MorphologyOperation::Dilate;
        StructuringElement m_structuringElement = StructuringElement::Sphere;
        double m_kernelRadius = 1.0;
    };

} // namespace isis::core::filters
