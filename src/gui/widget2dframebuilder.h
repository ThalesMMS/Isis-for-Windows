/*
 * ------------------------------------------------------------------------------------
 *  File: widget2dframebuilder.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Declares helpers that construct cached frame descriptors from GDCM images,
 *      computing default VOI windows and initial presentation state defaults.
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>

#include "widget2dimageframe.h"

namespace isis::core
{
	struct DicomPixelInfo;
}

namespace isis::gui
{
	struct Widget2dPresentationState;

	class Widget2DFrameBuilder
	{
	public:
		explicit Widget2DFrameBuilder(Widget2dPresentationState& initialState);

		Widget2DImageFrame createFrame(const core::DicomPixelInfo& pixelInfo,
		        int width,
		        int height,
		        int samplesPerPixel,
		        Widget2DScalarType scalarType,
		        int frameIndex,
		        bool shouldUpdateInitialState);

		static std::size_t bytesPerSample(Widget2DScalarType scalarType);
		static void calculateDefaultVOIWindow(Widget2DImageFrame& frame);

	private:
		Widget2dPresentationState& m_initialState;
	};
}
