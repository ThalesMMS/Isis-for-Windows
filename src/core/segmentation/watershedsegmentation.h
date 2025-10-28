/*
 * ------------------------------------------------------------------------------------
 *  File: watershedsegmentation.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Watershed segmentation algorithm for advanced image segmentation
 *      with marker-based control and gradient preprocessing
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkImageGradient.h>
#include <vtkImageGradientMagnitude.h>

namespace isis::core::segmentation
{
    /**
     * @brief Watershed segmentation algorithm
     *
     * Implements marker-based watershed segmentation with
     * automatic gradient preprocessing and post-processing.
     */
    class WatershedSegmentation
    {
    public:
        WatershedSegmentation();
        ~WatershedSegmentation() = default;

        /**
         * @brief Set input image for segmentation
         * @param input VTK image data
         */
        void setInputImage(vtkImageData* input);

        /**
         * @brief Set markers image for watershed
         * @param markers Binary image with markers
         */
        void setMarkers(vtkImageData* markers);

        /**
         * @brief Set level for watershed flooding
         * @param level Flood level
         */
        void setWatershedLevel(double level);

        /**
         * @brief Enable/disable gradient preprocessing
         * @param enable true to enable, false to disable
         */
        void setGradientPreprocessing(bool enable);

        /**
         * @brief Execute watershed segmentation
         * @return Segmented labeled image
         */
        vtkSmartPointer<vtkImageData> execute();

        /**
         * @brief Get the output segmentation
         * @return Labeled segmentation image
         */
        [[nodiscard]] vtkImageData* getOutput() const;

        /**
         * @brief Preprocess image with gradient filter
         * @return Gradient magnitude image
         */
        vtkSmartPointer<vtkImageData> preprocessGradient();

    private:
        vtkSmartPointer<vtkImageGradientMagnitude> m_gradientFilter;
        vtkSmartPointer<vtkImageData> m_inputImage;
        vtkSmartPointer<vtkImageData> m_markersImage;
        vtkSmartPointer<vtkImageData> m_outputSegmentation;
        double m_watershedLevel = 0.5;
        bool m_useGradientPreprocessing = true;
    };

} // namespace isis::core::segmentation
