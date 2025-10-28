/*
 * ------------------------------------------------------------------------------------
 *  File: regiongrowingsegmentation.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Region growing segmentation algorithm for semi-automatic segmentation
 *      Seeds-based approach with adjustable parameters
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkImageConnectivityFilter.h>
#include <array>
#include <vector>

namespace isis::core::segmentation
{
    /**
     * @brief Region growing segmentation algorithm
     *
     * Implements semi-automatic segmentation based on seed points
     * and connectivity criteria (intensity similarity, neighborhood).
     */
    class RegionGrowingSegmentation
    {
    public:
        RegionGrowingSegmentation();
        ~RegionGrowingSegmentation() = default;

        /**
         * @brief Set input image for segmentation
         * @param input VTK image data
         */
        void setInputImage(vtkImageData* input);

        /**
         * @brief Add a seed point for region growing
         * @param x X coordinate
         * @param y Y coordinate
         * @param z Z coordinate
         */
        void addSeedPoint(int x, int y, int z);

        /**
         * @brief Clear all seed points
         */
        void clearSeeds();

        /**
         * @brief Set intensity threshold for region growing
         * @param lower Lower intensity threshold
         * @param upper Upper intensity threshold
         */
        void setIntensityRange(double lower, double upper);

        /**
         * @brief Set neighborhood size (connectivity)
         * @param size Neighborhood size (6, 18, or 26 connectivity)
         */
        void setNeighborhoodSize(int size);

        /**
         * @brief Execute region growing segmentation
         * @return Segmented binary mask
         */
        vtkSmartPointer<vtkImageData> execute();

        /**
         * @brief Get the output mask
         * @return Segmentation mask
         */
        [[nodiscard]] vtkImageData* getOutput() const;

    private:
        vtkSmartPointer<vtkImageConnectivityFilter> m_connectivityFilter;
        vtkSmartPointer<vtkImageData> m_inputImage;
        vtkSmartPointer<vtkImageData> m_outputMask;
        std::vector<std::array<int, 3>> m_seedPoints;
        double m_lowerThreshold = 0.0;
        double m_upperThreshold = 255.0;
        int m_neighborhoodSize = 6; // 6-connectivity by default
    };

} // namespace isis::core::segmentation
