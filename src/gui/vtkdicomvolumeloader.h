/*
 * ------------------------------------------------------------------------------------
 *  File: vtkdicomvolumeloader.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Structures and helpers for building VTK volumes from DICOM series and images.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <QString>
#include <limits>
#include <vtkMatrix4x4.h>
#include <vtkSmartPointer.h>

#include "dicomvolume.h"

namespace isis::core
{
    class Image;
    class Series;
}

namespace isis::gui
{
    struct VtkDicomVolume
    {
        vtkSmartPointer<vtkImageData> ImageData = {};
        vtkSmartPointer<vtkMatrix4x4> Direction = {};
        core::DicomGeometry Geometry = {};
        core::DicomMetadata OverlayMetadata = {};
        double WindowCenter = std::numeric_limits<double>::quiet_NaN();
        double WindowWidth = std::numeric_limits<double>::quiet_NaN();
        int NumberOfFrames = 0;
        std::vector<std::string> SourceFiles = {};

        [[nodiscard]] bool isValid() const
        {
            return ImageData != nullptr;
        }
    };

    class VtkDicomVolumeLoader
    {
    public:
        VtkDicomVolumeLoader() = default;
        ~VtkDicomVolumeLoader() = default;

        [[nodiscard]] std::shared_ptr<VtkDicomVolume> loadFromSeries(
            core::Series& series,
            QString* failureReason = nullptr) const;

        [[nodiscard]] std::shared_ptr<VtkDicomVolume> loadFromImage(
            const core::Image& image,
            QString* failureReason = nullptr) const;

    private:
        [[nodiscard]] std::shared_ptr<VtkDicomVolume> buildVolume(
            const std::shared_ptr<core::DicomVolume>& source,
            const std::vector<std::string>& sourceFiles,
            QString* failureReason) const;
    };
}
