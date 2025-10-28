/*
 * ------------------------------------------------------------------------------------
 *  File: widget2dimageframe.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Declares the light-weight data structure that caches pixel data and default
 *      windowing parameters for 2D DICOM frames.
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <QByteArray>
#include <string>

#include "dicomvolume.h"

namespace isis::gui
{
        enum class Widget2DScalarType
        {
                Uint8,
                Sint8,
                Uint16,
                Sint16,
                Uint32,
                Sint32,
                Float32,
                Float64
        };

	struct Widget2DImageFrame
	{
		QByteArray Data = {};
		isis::core::DicomPixelInfo PixelInfo = {};
		int Width = 0;
		int Height = 0;
		int SamplesPerPixel = 1;
		Widget2DScalarType ScalarType = Widget2DScalarType::Uint8;
		double MinValue = 0.0;
		double MaxValue = 0.0;
		double DefaultWindowCenter = 0.0;
		double DefaultWindowWidth = 0.0;
                int FrameIndex = 0;
                bool Cached = false;
                bool Decoding = false;
                std::string SourcePath = {};
        };
}
