/*
 * ------------------------------------------------------------------------------------
 *  File: dicomseriesloader.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Declares helpers responsible for loading DICOM volumes from single files or
 *      series of slice paths.
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include "dicomvolume.h"

#include <memory>
#include <string>
#include <vector>

namespace isis::core::seriesloader
{
        std::shared_ptr<DicomVolume> loadVolumeFromFile(const std::string& path);
        std::shared_ptr<DicomVolume> loadVolumeFromSeries(const std::vector<std::string>& slicePaths);
}
