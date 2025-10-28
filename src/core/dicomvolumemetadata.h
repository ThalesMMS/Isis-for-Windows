/*
 * ------------------------------------------------------------------------------------
 *  File: dicomvolumemetadata.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Declares helpers for extracting metadata, pixel information, and geometry from
 *      DICOM datasets in support of volume loading.
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include "dicomvolume.h"

#include <gdcmFile.h>

namespace isis::core::metadata
{
        struct DicomVolumeMetadata
        {
                DicomMetadata Metadata;
                DicomGeometry Geometry;
                DicomPixelInfo PixelInfo;
        };

        DicomVolumeMetadata loadVolumeMetadata(const gdcm::File& file);
        void populateMetadata(const gdcm::File& file, DicomMetadata& metadata);
        void populatePixelInfo(const gdcm::File& file, DicomPixelInfo& info);
        void populateGeometry(const gdcm::File& file, DicomGeometry& geometry);
        void normalizeDirections(DicomGeometry& geometry);
}
