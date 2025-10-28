/*
 * ------------------------------------------------------------------------------------
 *  File: dicomvolumecache.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Declares the DICOM volume cache responsible for retaining assembled volumes
 *      across study and series loads.
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include "dicomvolume.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace isis::core
{
        class DicomVolumeCache
        {
        public:
                using VolumePtr = std::shared_ptr<DicomVolume>;

                VolumePtr get(const std::string& studyUid,
                        const std::string& seriesUid,
                        const std::vector<std::string>& paths,
                        const std::function<VolumePtr()>& loader);

                void invalidateSeries(const std::string& studyUid,
                        const std::string& seriesUid);

                void invalidateStudy(const std::string& studyUid);
        };

        DicomVolumeCache& volumeCache();
}
