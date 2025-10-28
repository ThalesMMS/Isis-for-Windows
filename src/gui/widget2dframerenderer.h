/*
 * ------------------------------------------------------------------------------------
 *  File: widget2dframerenderer.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Declares helpers that map cached frame data into Qt images honouring VOI window,
 *      palette inversion, flipping, and rotations.
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <QImage>
#include <QVector>

#include "widget2dimageframe.h"

namespace isis::gui
{
	struct Widget2dPresentationState;

	class Widget2DFrameRenderer
	{
	public:
		Widget2DFrameRenderer();

		QImage renderFrame(Widget2DImageFrame& frame,
			const Widget2dPresentationState& state) const;

	private:
		[[nodiscard]] static const QVector<QRgb>& grayscaleColorTable();
	};
}
