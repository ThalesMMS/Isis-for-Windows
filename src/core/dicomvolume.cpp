/*
 * ------------------------------------------------------------------------------------
 *  File: dicomvolume.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements the DicomVolumeLoader, including caching, metadata
 *      extraction, and VTK volume construction helpers.
 *
 *  License:
 *      This file is part of a derived work based on the Asclepios DICOM Viewer,
 *      licensed under the MIT License.
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a copy
 *      of this software and associated documentation files (the "Software"), to deal
 *      in the Software without restriction, including without limitation the rights
 *      to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *      copies of the Software, and to permit persons to whom the Software is
 *      furnished to do so, subject to the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included in
 *      all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *      FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *      AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *      LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *      OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *      SOFTWARE.
 * ------------------------------------------------------------------------------------
 */

#include "dicomvolume.h"
#include "dicomvolumecache.h"
#include "dicomseriesloader.h"

#include <cctype>
#include <memory>
#include <stdexcept>
#include <vector>

#include <gdcmAttribute.h>
#include <gdcmFileMetaInformation.h>
#include <gdcmReader.h>
#include <gdcmStringFilter.h>
#include <gdcmTransferSyntax.h>

#include <QLoggingCategory>
#include <QString>

#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkMatrix4x4.h>

using isis::core::DicomVolume;
using isis::core::DicomVolumeLoader;
using isis::core::volumeCache;

namespace seriesloader = isis::core::seriesloader;

Q_LOGGING_CATEGORY(lcDicomVolumeLoader, "isis.core.dicomvolume")

std::shared_ptr<DicomVolume> DicomVolumeLoader::loadSeries(
        const std::string& studyInstanceUid,
        const std::string& seriesInstanceUid,
        const std::vector<std::string>& slicePaths)
{
        auto loader = [&slicePaths]() -> std::shared_ptr<DicomVolume>
        {
                return seriesloader::loadVolumeFromSeries(slicePaths);
        };

        return volumeCache().get(studyInstanceUid, seriesInstanceUid, slicePaths, loader);
}

void DicomVolumeLoader::invalidateSeriesCache(const std::string& studyInstanceUid,
        const std::string& seriesInstanceUid)
{
        volumeCache().invalidateSeries(studyInstanceUid, seriesInstanceUid);
}

void DicomVolumeLoader::invalidateStudyCache(const std::string& studyInstanceUid)
{
        volumeCache().invalidateStudy(studyInstanceUid);
}

std::shared_ptr<DicomVolume> DicomVolumeLoader::loadImage(const std::string& path)
{
        return seriesloader::loadVolumeFromFile(path);
}

bool DicomVolumeLoader::diagnoseStudy(const std::string& path)
{
        const auto qPath = QString::fromStdString(path);
        qCInfo(lcDicomVolumeLoader)
                << "Running diagnostic load for" << qPath;

        gdcm::Reader reader;
        reader.SetFileName(path.c_str());
        if (!reader.Read())
        {
                qCWarning(lcDicomVolumeLoader)
                        << "Diagnostic failed to open" << qPath;
                return false;
        }

        const gdcm::File& file = reader.GetFile();
        const gdcm::FileMetaInformation& meta = file.GetHeader();
        const gdcm::TransferSyntax& transferSyntax = meta.GetDataSetTransferSyntax();

        const char* tsName = gdcm::TransferSyntax::GetTSString(
                static_cast<gdcm::TransferSyntax::TSType>(transferSyntax));
        qCInfo(lcDicomVolumeLoader)
                << "Transfer syntax:"
                << QString::fromLatin1(tsName ? tsName : "Unknown")
                << "encapsulated:"
                << transferSyntax.IsEncapsulated();

        auto trim = [](std::string value) -> std::string {
                while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0)
                {
                        value.pop_back();
                }
                std::size_t start = 0;
                while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0)
                {
                        ++start;
                }
                return value.substr(start);
        };

        gdcm::StringFilter filter;
        filter.SetFile(file);
        const std::string modality = trim(filter.ToString(gdcm::Tag(0x0008, 0x0060)));
        if (!modality.empty())
        {
                qCInfo(lcDicomVolumeLoader)
                        << "Diagnostic modality:" << QString::fromStdString(modality);
        }

        try
        {
                const auto volume = loadImage(path);
                if (!volume || !volume->ImageData)
                {
                        qCWarning(lcDicomVolumeLoader)
                                << "Diagnostic load produced empty image data for" << qPath;
                        return false;
                }

                int dimensions[3] = {0, 0, 0};
                volume->ImageData->GetDimensions(dimensions);
                if (dimensions[0] <= 0 || dimensions[1] <= 0 || dimensions[2] <= 0)
                {
                        qCWarning(lcDicomVolumeLoader)
                                << "Diagnostic load reported invalid dimensions for"
                                << qPath
                                << dimensions[0] << dimensions[1] << dimensions[2];
                        return false;
                }

                qCInfo(lcDicomVolumeLoader)
                        << "Diagnostic load succeeded. Dimensions:"
                        << dimensions[0] << dimensions[1] << dimensions[2];
                auto* scalars = volume->ImageData->GetPointData() ?
                        volume->ImageData->GetPointData()->GetScalars() : nullptr;
                if (scalars)
                {
                        qCInfo(lcDicomVolumeLoader)
                                << "Diagnostic scalar type:"
                                << scalars->GetDataTypeAsString()
                                << "components:"
                                << scalars->GetNumberOfComponents();
                }
                qCInfo(lcDicomVolumeLoader)
                        << "Diagnostic rescale applied:"
                        << volume->PixelInfo.RescaleSlope
                        << volume->PixelInfo.RescaleIntercept;
                return true;
        }
        catch (const std::exception& ex)
        {
                qCCritical(lcDicomVolumeLoader)
                        << "Diagnostic load failed for"
                        << qPath
                        << ":" << ex.what();
        }
        catch (...)
        {
                qCCritical(lcDicomVolumeLoader)
                        << "Diagnostic load failed for"
                        << qPath
                        << ": unknown exception.";
        }

        return false;
}

void isis::core::DicomVolumeLoader::populateDirectionMatrix(DicomVolume& volume)
{
        if (!volume.Direction)
        {
                volume.Direction = vtkSmartPointer<vtkMatrix4x4>::New();
        }

        auto* const matrix = volume.Direction.GetPointer();
        for (int row = 0; row < 3; ++row)
        {
                matrix->SetElement(row, 0, volume.Geometry.RowDirection[row]);
                matrix->SetElement(row, 1, volume.Geometry.ColumnDirection[row]);
                matrix->SetElement(row, 2, volume.Geometry.NormalDirection[row]);
                matrix->SetElement(row, 3, volume.Geometry.Origin[row]);
        }

        matrix->SetElement(3, 0, 0.0);
        matrix->SetElement(3, 1, 0.0);
        matrix->SetElement(3, 2, 0.0);
        matrix->SetElement(3, 3, 1.0);

        if (volume.ImageData)
        {
                double origin[3] = {
                        matrix->GetElement(0, 3),
                        matrix->GetElement(1, 3),
                        matrix->GetElement(2, 3)};
                volume.ImageData->SetOrigin(origin);
                volume.ImageData->Modified();
        }
}

