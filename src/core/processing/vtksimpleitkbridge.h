/*
 * ------------------------------------------------------------------------------------
 *  File: vtksimpleitkbridge.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Bridge between VTK and SimpleITK for advanced image processing
 *      Provides efficient conversion between VTK and SimpleITK image formats
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <vtkSmartPointer.h>
#include <vtkImageData.h>

// Note: SimpleITK headers will be included when SimpleITK is available
// For now, this is a placeholder interface

namespace isis::core::processing
{
    /**
     * @brief Bridge for converting between VTK and SimpleITK image formats
     *
     * This class provides methods to convert vtkImageData to SimpleITK images
     * and vice versa, enabling the use of SimpleITK's advanced processing
     * algorithms with VTK's visualization capabilities.
     *
     * Note: Requires SimpleITK library to be linked.
     */
    class VtkSimpleItkBridge
    {
    public:
        VtkSimpleItkBridge() = default;
        ~VtkSimpleItkBridge() = default;

        /**
         * @brief Convert VTK image to SimpleITK image
         * @param vtkImage Input VTK image
         * @return SimpleITK image (placeholder - requires SimpleITK)
         *
         * Note: Implementation requires SimpleITK library
         */
        //static sitk::Image vtkToSimpleItk(vtkImageData* vtkImage);

        /**
         * @brief Convert SimpleITK image to VTK image
         * @param sitkImage Input SimpleITK image
         * @return VTK image
         *
         * Note: Implementation requires SimpleITK library
         */
        //static vtkSmartPointer<vtkImageData> simpleItkToVtk(const sitk::Image& sitkImage);

        /**
         * @brief Check if the bridge is available (SimpleITK linked)
         * @return true if SimpleITK is available, false otherwise
         */
        [[nodiscard]] static bool isAvailable();

        /**
         * @brief Get version information
         * @return Version string
         */
        [[nodiscard]] static const char* getVersion();

    private:
        // Cache for performance optimization (future implementation)
        // static std::unordered_map<vtkImageData*, sitk::Image> m_vtkToSitkCache;
    };

} // namespace isis::core::processing
