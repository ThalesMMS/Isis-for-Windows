/*
 * ------------------------------------------------------------------------------------
 *  File: dicomseriesloader_slope_tolerance_test.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Regression test for DicomSeriesLoader rescale slope/intercept tolerance.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "src/core/dicomvolume.h"
#include "src/gui/vtkdicomvolumeloader.h"
#include "src/gui/vtkwidget3d.h"
#include "src/gui/vtkwidgetmpr.h"

#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmdata/dcuid.h>

#include <QCoreApplication>

#include <vtkDataArray.h>

#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
        void writeSyntheticSlice(const std::filesystem::path& path,
                                const std::string& studyUid,
                                const std::string& seriesUid,
                                int index,
                                double rescaleSlope,
                                double rescaleIntercept)
        {
                DcmFileFormat file;
                auto* dataset = file.getDataset();

                const std::string instanceUid = seriesUid + "." + std::to_string(index + 1);

                dataset->putAndInsertString(DCM_SOPClassUID, UID_CTImageStorage);
                dataset->putAndInsertString(DCM_SOPInstanceUID, instanceUid.c_str());
                dataset->putAndInsertString(DCM_Modality, "CT");
                dataset->putAndInsertString(DCM_ImageType, "ORIGINAL\\PRIMARY");
                dataset->putAndInsertString(DCM_PatientName, "Slope^Tolerance");
                dataset->putAndInsertString(DCM_PatientID, "SLOPETEST");
                dataset->putAndInsertString(DCM_StudyInstanceUID, studyUid.c_str());
                dataset->putAndInsertString(DCM_SeriesInstanceUID, seriesUid.c_str());
                dataset->putAndInsertUint16(DCM_SeriesNumber, 1);
                dataset->putAndInsertUint16(DCM_InstanceNumber, static_cast<Uint16>(index + 1));

                dataset->putAndInsertUint16(DCM_SamplesPerPixel, 1);
                dataset->putAndInsertString(DCM_PhotometricInterpretation, "MONOCHROME2");
                dataset->putAndInsertUint16(DCM_Rows, 8);
                dataset->putAndInsertUint16(DCM_Columns, 8);
                dataset->putAndInsertString(DCM_PixelSpacing, "1.0\\1.0");
                dataset->putAndInsertString(DCM_ImageOrientationPatient, "1\\0\\0\\0\\1\\0");

                const double slicePosition = static_cast<double>(index);
                dataset->putAndInsertString(DCM_ImagePositionPatient,
                                            ("0\\0\\" + std::to_string(slicePosition)).c_str());

                dataset->putAndInsertFloat64(DCM_SliceThickness, 1.0);
                dataset->putAndInsertFloat64(DCM_SpacingBetweenSlices, 1.0);

                dataset->putAndInsertUint16(DCM_BitsAllocated, 16);
                dataset->putAndInsertUint16(DCM_BitsStored, 16);
                dataset->putAndInsertUint16(DCM_HighBit, 15);
                dataset->putAndInsertUint16(DCM_PixelRepresentation, 0);

                dataset->putAndInsertFloat64(DCM_RescaleIntercept, rescaleIntercept);
                dataset->putAndInsertFloat64(DCM_RescaleSlope, rescaleSlope);
                dataset->putAndInsertFloat64(DCM_WindowCenter, 40.0);
                dataset->putAndInsertFloat64(DCM_WindowWidth, 350.0);

                std::array<Uint16, 64> pixels{};
                for (size_t i = 0; i < pixels.size(); ++i)
                {
                        pixels[i] = static_cast<Uint16>(index * 100 + i);
                }

                dataset->putAndInsertUint16Array(DCM_PixelData, pixels.data(), pixels.size());

                const OFCondition status = file.saveFile(path.string().c_str(), EXS_LittleEndianExplicit);
                if (status.bad())
                {
                        throw std::runtime_error("Failed to write synthetic slice: " +
                                                 std::string(status.text()));
                }
        }
}

int main()
{
        try
        {
                int argc = 1;
                char appName[] = "dicomseriesloader_slope_tolerance_test";
                char* argv[] = {appName, nullptr};
                QCoreApplication app(argc, argv);

                const std::string studyUid = "1.2.826.0.1.3680043.2.1125.9999.200";
                const std::string seriesUid = studyUid + ".1";

                const auto tempRoot = std::filesystem::temp_directory_path() /
                        "isis_dicom_slope_tolerance";
                std::filesystem::remove_all(tempRoot);
                std::filesystem::create_directories(tempRoot);

                std::vector<std::string> slicePaths;
                for (int index = 0; index < 3; ++index)
                {
                        const double rescaleSlope = 1.0 + static_cast<double>(index) * 5e-4;
                        const double rescaleIntercept = -9215.0 + static_cast<double>(index) * 16.0;
                        const auto slicePath = tempRoot / ("slice_" + std::to_string(index) + ".dcm");
                        writeSyntheticSlice(slicePath, studyUid, seriesUid, index, rescaleSlope, rescaleIntercept);
                        slicePaths.emplace_back(slicePath.string());
                }

                auto volume = isis::core::DicomVolumeLoader::loadSeries(studyUid, seriesUid, slicePaths);
                if (!volume)
                {
                        throw std::runtime_error("DicomVolumeLoader::loadSeries returned nullptr.");
                }
                if (!volume->ImageData)
                {
                        throw std::runtime_error("DicomVolumeLoader produced volume without image data.");
                }

                vtkDataArray* const scalars = volume->ImageData->GetPointData()
                        ? volume->ImageData->GetPointData()->GetScalars()
                        : nullptr;
                if (!scalars)
                {
                        throw std::runtime_error("DicomVolumeLoader volume is missing scalar data.");
                }

                double range[2] = {0.0, 0.0};
                scalars->GetRange(range);
                if (range[0] >= 0.0)
                {
                        throw std::runtime_error("Rescaled scalar range did not include negative values.");
                }

                if (std::abs(volume->PixelInfo.RescaleSlope - 1.0) > 1e-6)
                {
                        throw std::runtime_error("Rescale slope was not normalized after loading.");
                }
                if (std::abs(volume->PixelInfo.RescaleIntercept) > 1e-6)
                {
                        throw std::runtime_error("Rescale intercept was not cleared after loading.");
                }
                if (!volume->PixelInfo.IsSigned)
                {
                        throw std::runtime_error("Volume metadata did not mark the scalar buffer as signed.");
                }

                int extent[6] = {0, 0, 0, 0, 0, 0};
                volume->ImageData->GetExtent(extent);
                const int slicesAlongZ = extent[5] - extent[4] + 1;
                if (slicesAlongZ < 2)
                {
                        throw std::runtime_error("Loaded volume does not contain multiple slices.");
                }

                auto vtkVolume = std::make_shared<isis::gui::VtkDicomVolume>();
                vtkVolume->ImageData = volume->ImageData;
                vtkVolume->Direction = volume->Direction;
                vtkVolume->NumberOfFrames = volume->NumberOfFrames;
                vtkVolume->WindowCenter = volume->PixelInfo.WindowCenter;
                vtkVolume->WindowWidth = volume->PixelInfo.WindowWidth;

                isis::gui::vtkWidgetMPR mprWidget;
                mprWidget.setVolumeForTesting(vtkVolume);
                if (!mprWidget.hasValidVolume())
                {
                        throw std::runtime_error("vtkWidgetMPR rejected volume with tolerated slope variations.");
                }

                isis::gui::vtkWidget3D widget3D;
                if (!widget3D.composeAndRenderVolume(vtkVolume))
                {
                        throw std::runtime_error(
                                std::string("vtkWidget3D failed to compose volume: ") +
                                widget3D.lastVolumeError().toStdString());
                }

                std::filesystem::remove_all(tempRoot);
        }
        catch (const std::exception& ex)
        {
                std::cerr << "dicomseriesloader_slope_tolerance_test failed: " << ex.what() << '\n';
                return EXIT_FAILURE;
        }

        std::cout << "dicomseriesloader_slope_tolerance_test passed" << std::endl;
        return EXIT_SUCCESS;
}

