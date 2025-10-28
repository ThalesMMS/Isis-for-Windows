/*
 * ------------------------------------------------------------------------------------
 *  File: dicomseriesloader_window_mismatch_test.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Regression test for DicomSeriesLoader window center/width mismatch handling.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "src/core/dicomvolume.h"

#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmdata/dcuid.h>

#include <QCoreApplication>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
        void writeSlice(const std::filesystem::path& path,
                        const std::string& studyUid,
                        const std::string& seriesUid,
                        int index,
                        double windowCenter,
                        double windowWidth)
        {
                DcmFileFormat file;
                auto* dataset = file.getDataset();

                const std::string instanceUid = seriesUid + "." + std::to_string(index + 1);

                dataset->putAndInsertString(DCM_SOPClassUID, UID_CTImageStorage);
                dataset->putAndInsertString(DCM_SOPInstanceUID, instanceUid.c_str());
                dataset->putAndInsertString(DCM_Modality, "CT");
                dataset->putAndInsertString(DCM_ImageType, "ORIGINAL\\PRIMARY");
                dataset->putAndInsertString(DCM_PatientName, "Window^Tolerance");
                dataset->putAndInsertString(DCM_PatientID, "WINDOW");
                dataset->putAndInsertString(DCM_StudyInstanceUID, studyUid.c_str());
                dataset->putAndInsertString(DCM_SeriesInstanceUID, seriesUid.c_str());
                dataset->putAndInsertUint16(DCM_SeriesNumber, 1);
                dataset->putAndInsertUint16(DCM_InstanceNumber, static_cast<Uint16>(index + 1));

                dataset->putAndInsertUint16(DCM_SamplesPerPixel, 1);
                dataset->putAndInsertString(DCM_PhotometricInterpretation, "MONOCHROME2");
                dataset->putAndInsertUint16(DCM_Rows, 4);
                dataset->putAndInsertUint16(DCM_Columns, 4);
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

                dataset->putAndInsertFloat64(DCM_RescaleIntercept, 0.0);
                dataset->putAndInsertFloat64(DCM_RescaleSlope, 1.0);
                dataset->putAndInsertFloat64(DCM_WindowCenter, windowCenter);
                dataset->putAndInsertFloat64(DCM_WindowWidth, windowWidth);

                std::array<Uint16, 16> pixels{};
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
                char appName[] = "dicomseriesloader_window_mismatch_test";
                char* argv[] = {appName, nullptr};
                QCoreApplication app(argc, argv);

                const std::string studyUid = "1.2.826.0.1.3680043.2.1125.9999.100";
                const std::string seriesUid = studyUid + ".1";

                const auto tempRoot = std::filesystem::temp_directory_path() /
                        "isis_dicom_window_mismatch";
                std::filesystem::remove_all(tempRoot);
                std::filesystem::create_directories(tempRoot);

                std::vector<std::string> slicePaths;
                for (int index = 0; index < 3; ++index)
                {
                        const auto slicePath = tempRoot / ("slice_" + std::to_string(index) + ".dcm");
                        writeSlice(slicePath, studyUid, seriesUid, index, 40.0 + index * 5.0, 350.0 + index * 10.0);
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

                std::filesystem::remove_all(tempRoot);
        }
        catch (const std::exception& ex)
        {
                std::cerr << "dicomseriesloader_window_mismatch_test failed: " << ex.what() << '\n';
                return EXIT_FAILURE;
        }

        std::cout << "dicomseriesloader_window_mismatch_test passed" << std::endl;
        return EXIT_SUCCESS;
}
