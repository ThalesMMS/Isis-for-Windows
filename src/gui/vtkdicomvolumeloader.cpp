/*
 * ------------------------------------------------------------------------------------
 *  File: vtkdicomvolumeloader.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Helper for constructing GUI-ready VTK volumes from core DicomVolume data.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "vtkdicomvolumeloader.h"

#include <QLoggingCategory>

#include <cmath>

#include "image.h"
#include "series.h"

namespace
{
    Q_LOGGING_CATEGORY(lcVtkDicomLoader, "isis.gui.gdcm.loader")
}

using namespace isis;

std::shared_ptr<gui::VtkDicomVolume> gui::VtkDicomVolumeLoader::loadFromSeries(
    core::Series& series,
    QString* failureReason) const
{
    auto volume = series.getVolumeForSingleFrameSeries();
    if (!volume || !volume->ImageData)
    {
        if (failureReason)
        {
            *failureReason = QStringLiteral("Series volume is unavailable or invalid.");
        }
        return nullptr;
    }

    const auto sourceFiles = series.snapshotSingleFramePaths();
    return buildVolume(volume, sourceFiles, failureReason);
}

std::shared_ptr<gui::VtkDicomVolume> gui::VtkDicomVolumeLoader::loadFromImage(
    const core::Image& image,
    QString* failureReason) const
{
    auto volume = image.getDicomVolume(failureReason);
    if (!volume || !volume->ImageData)
    {
        if (failureReason && failureReason->isEmpty())
        {
            *failureReason = QStringLiteral("Image volume is unavailable or invalid.");
        }
        return nullptr;
    }

    return buildVolume(volume, {image.getImagePath()}, failureReason);
}

std::shared_ptr<gui::VtkDicomVolume> gui::VtkDicomVolumeLoader::buildVolume(
    const std::shared_ptr<core::DicomVolume>& source,
    const std::vector<std::string>& sourceFiles,
    QString* failureReason) const
{
    if (!source || !source->ImageData)
    {
        if (failureReason)
        {
            *failureReason = QStringLiteral("Source DICOM volume is invalid.");
        }
        return nullptr;
    }

    auto vtkVolume = std::make_shared<VtkDicomVolume>();
    vtkVolume->ImageData = source->ImageData;
    vtkVolume->Direction = source->Direction;
    vtkVolume->Geometry = source->Geometry;
    vtkVolume->OverlayMetadata = source->Metadata;
    vtkVolume->WindowCenter = source->PixelInfo.WindowCenter;
    vtkVolume->WindowWidth = source->PixelInfo.WindowWidth;
    vtkVolume->NumberOfFrames = source->NumberOfFrames;
    vtkVolume->SourceFiles = sourceFiles;

    if (vtkVolume->Direction == nullptr)
    {
        vtkVolume->Direction = vtkSmartPointer<vtkMatrix4x4>::New();
        vtkVolume->Direction->Identity();
        for (int axis = 0; axis < 3; ++axis)
        {
            vtkVolume->Direction->SetElement(axis, 0, vtkVolume->Geometry.RowDirection[axis]);
            vtkVolume->Direction->SetElement(axis, 1, vtkVolume->Geometry.ColumnDirection[axis]);
            vtkVolume->Direction->SetElement(axis, 2, vtkVolume->Geometry.NormalDirection[axis]);
            vtkVolume->Direction->SetElement(axis, 3, vtkVolume->Geometry.Origin[axis]);
        }
    }

    if (std::isnan(vtkVolume->WindowCenter) || std::isnan(vtkVolume->WindowWidth))
    {
        int dimensions[3] = {0, 0, 0};
        vtkVolume->ImageData->GetDimensions(dimensions);
        double range[2] = {0.0, 0.0};
        vtkVolume->ImageData->GetScalarRange(range);
        vtkVolume->WindowCenter = (range[0] + range[1]) / 2.0;
        vtkVolume->WindowWidth = range[1] - range[0];
    }

    if (failureReason)
    {
        failureReason->clear();
    }

    qCInfo(lcVtkDicomLoader)
        << "Prepared GDCM-backed volume"
        << "frames:" << vtkVolume->NumberOfFrames
        << "spacing:" << vtkVolume->Geometry.Spacing[0]
        << vtkVolume->Geometry.Spacing[1]
        << vtkVolume->Geometry.Spacing[2];

    return vtkVolume;
}
