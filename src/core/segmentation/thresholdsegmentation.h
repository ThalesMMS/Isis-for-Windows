/*
 * ------------------------------------------------------------------------------------
 *  File: thresholdsegmentation.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Threshold-based segmentation for medical images
 *      Supports global and adaptive thresholding with binary mask generation
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkImageThreshold.h>

namespace isis::core::segmentation
{
    /**
     * @brief Threshold segmentation algorithm
     *
     * Provides basic intensity-based segmentation using threshold values.
     * Generates binary masks for regions of interest.
     */
    class ThresholdSegmentation
    {
    public:
        ThresholdSegmentation();
        ~ThresholdSegmentation() = default;

        /**
         * @brief Set input image for segmentation
         * @param input VTK image data
         */
        void setInputImage(vtkImageData* input);

        /**
         * @brief Set lower threshold value
         * @param threshold Lower bound for segmentation
         */
        void setLowerThreshold(double threshold);

        /**
         * @brief Set upper threshold value
         * @param threshold Upper bound for segmentation
         */
        void setUpperThreshold(double threshold);

        /**
         * @brief Set both threshold values
         * @param lower Lower bound
         * @param upper Upper bound
         */
        void setThresholds(double lower, double upper);

        /**
         * @brief Execute segmentation
         * @return Binary mask as vtkImageData
         */
        vtkSmartPointer<vtkImageData> execute();

        /**
         * @brief Get the output mask
         * @return Binary segmentation mask
         */
        [[nodiscard]] vtkImageData* getOutput() const;

    private:
        vtkSmartPointer<vtkImageThreshold> m_thresholdFilter;
        vtkSmartPointer<vtkImageData> m_inputImage;
        vtkSmartPointer<vtkImageData> m_outputMask;
        double m_lowerThreshold = 0.0;
        double m_upperThreshold = 255.0;
    };

} // namespace isis::core::segmentation
