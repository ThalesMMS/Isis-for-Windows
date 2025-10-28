/*
 * ------------------------------------------------------------------------------------
 *  File: widget2dframebuilder.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Implements helpers that construct cached frame descriptors from GDCM images,
 *      computing default VOI windows and initial presentation state defaults.
 * ------------------------------------------------------------------------------------
 */

#include "widget2dframebuilder.h"

#include "widget2dpresentationstate.h"

#include <algorithm>
#include <QLoggingCategory>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

Q_DECLARE_LOGGING_CATEGORY(lcWidget2D)

namespace isis::gui
{

Widget2DFrameBuilder::Widget2DFrameBuilder(Widget2dPresentationState& initialState)
	: m_initialState(initialState)
{
}

namespace
{
        template <typename T>
        void computeRange(const T* data,
                std::size_t count,
                double& minValue,
                double& maxValue)
        {
                if (!data || count == 0)
                {
                        minValue = 0.0;
                        maxValue = 0.0;
                        return;
                }

                auto minVal = static_cast<double>(data[0]);
                auto maxVal = static_cast<double>(data[0]);
                for (std::size_t index = 1; index < count; ++index)
                {
                        const auto value = static_cast<double>(data[index]);
                        minVal = std::min(minVal, value);
                        maxVal = std::max(maxVal, value);
                }
                minValue = minVal;
                maxValue = maxVal;
        }
}

Widget2DImageFrame Widget2DFrameBuilder::createFrame(const core::DicomPixelInfo& pixelInfo,
        const int width,
        const int height,
        const int samplesPerPixel,
        const Widget2DScalarType scalarType,
        const int frameIndex,
        const bool shouldUpdateInitialState)
{
	Widget2DImageFrame frame;
	frame.PixelInfo = pixelInfo;
	frame.FrameIndex = frameIndex;
	frame.Width = std::max(width, 0);
	frame.Height = std::max(height, 0);
	frame.SamplesPerPixel = (samplesPerPixel > 0) ? samplesPerPixel : 1;
	frame.ScalarType = scalarType;
	frame.DefaultWindowCenter = pixelInfo.WindowCenter;
	frame.DefaultWindowWidth = std::max<double>(pixelInfo.WindowWidth, 1.0);

	calculateDefaultVOIWindow(frame);

	if (shouldUpdateInitialState)
	{
		m_initialState.WindowCenter = frame.DefaultWindowCenter;
		m_initialState.WindowWidth = frame.DefaultWindowWidth;
		m_initialState.InvertColors = frame.PixelInfo.InvertMonochrome;
		m_initialState.FlipHorizontal = false;
		m_initialState.FlipVertical = false;
		m_initialState.RotationSteps = 0;
	}

	return frame;
}

std::size_t Widget2DFrameBuilder::bytesPerSample(const Widget2DScalarType scalarType)
{
	switch (scalarType)
	{
	case Widget2DScalarType::Uint8:
	case Widget2DScalarType::Sint8:
		return sizeof(std::uint8_t);
	case Widget2DScalarType::Uint16:
	case Widget2DScalarType::Sint16:
		return sizeof(std::uint16_t);
	case Widget2DScalarType::Uint32:
	case Widget2DScalarType::Sint32:
		return sizeof(std::uint32_t);
	case Widget2DScalarType::Float32:
		return sizeof(float);
	case Widget2DScalarType::Float64:
		return sizeof(double);
	default:
		return 0;
	}
}

void Widget2DFrameBuilder::calculateDefaultVOIWindow(Widget2DImageFrame& frame)
{
	if (frame.SamplesPerPixel > 1)
	{
		frame.MinValue = 0.0;
		frame.MaxValue = 255.0;
		frame.DefaultWindowWidth = 255.0;
		frame.DefaultWindowCenter = 127.0;
		return;
	}

        if (frame.Data.isEmpty())
        {
                frame.MinValue = 0.0;
                frame.MaxValue = 0.0;
                frame.DefaultWindowWidth = std::max<double>(frame.PixelInfo.WindowWidth, 1.0);
                frame.DefaultWindowCenter = frame.PixelInfo.WindowCenter;
                return;
        }

        const int width = std::max(frame.Width, 0);
        const int height = std::max(frame.Height, 0);
        const int components = std::max(frame.SamplesPerPixel, 1);
        const std::size_t sampleCount = static_cast<std::size_t>(width)
                * static_cast<std::size_t>(height)
                * static_cast<std::size_t>(components);

        double minValue = 0.0;
        double maxValue = 0.0;

        switch (frame.ScalarType)
        {
        case Widget2DScalarType::Uint8:
        {
                computeRange(reinterpret_cast<const std::uint8_t*>(frame.Data.constData()),
                        sampleCount,
                        minValue,
                        maxValue);
                break;
        }
        case Widget2DScalarType::Sint8:
        {
                computeRange(reinterpret_cast<const std::int8_t*>(frame.Data.constData()),
                        sampleCount,
                        minValue,
                        maxValue);
                break;
        }
        case Widget2DScalarType::Uint16:
        {
                computeRange(reinterpret_cast<const std::uint16_t*>(frame.Data.constData()),
                        sampleCount,
                        minValue,
                        maxValue);
                break;
        }
        case Widget2DScalarType::Sint16:
        {
                computeRange(reinterpret_cast<const std::int16_t*>(frame.Data.constData()),
                        sampleCount,
                        minValue,
                        maxValue);
                break;
        }
        case Widget2DScalarType::Uint32:
        {
                computeRange(reinterpret_cast<const std::uint32_t*>(frame.Data.constData()),
                        sampleCount,
                        minValue,
                        maxValue);
                break;
        }
        case Widget2DScalarType::Sint32:
        {
                computeRange(reinterpret_cast<const std::int32_t*>(frame.Data.constData()),
                        sampleCount,
                        minValue,
                        maxValue);
                break;
        }
        case Widget2DScalarType::Float32:
        {
                computeRange(reinterpret_cast<const float*>(frame.Data.constData()),
                        sampleCount,
                        minValue,
                        maxValue);
                break;
        }
        case Widget2DScalarType::Float64:
        {
                computeRange(reinterpret_cast<const double*>(frame.Data.constData()),
                        sampleCount,
                        minValue,
                        maxValue);
                break;
        }
        default:
                minValue = 0.0;
                maxValue = 0.0;
                break;
        }

        const double slope = frame.PixelInfo.RescaleSlope;
        const double intercept = frame.PixelInfo.RescaleIntercept;
        const double scaledMin = minValue * slope + intercept;
        const double scaledMax = maxValue * slope + intercept;
        frame.MinValue = scaledMin;
        frame.MaxValue = scaledMax;

        double windowWidth = frame.PixelInfo.WindowWidth;
        double windowCenter = frame.PixelInfo.WindowCenter;
        const double originalWindowWidth = windowWidth;
        const double originalWindowCenter = windowCenter;
        if (scaledMax > scaledMin)
        {
                const double scaledRange = scaledMax - scaledMin;
                const double safeRange = std::max(scaledRange, 1.0);
                const double lowerBound = windowCenter - (windowWidth * 0.5);
                const double upperBound = windowCenter + (windowWidth * 0.5);
                const bool windowFinite = std::isfinite(windowCenter) && std::isfinite(windowWidth);
                const bool windowCoversData = windowFinite
                        && upperBound > scaledMin
                        && lowerBound < scaledMax;

                double coverageRatio = 0.0;
                if (windowCoversData && windowWidth > 0.0)
                {
                        const double intersectionLower = std::max(lowerBound, scaledMin);
                        const double intersectionUpper = std::min(upperBound, scaledMax);
                        const double coverageWidth = std::max(0.0, intersectionUpper - intersectionLower);
                        coverageRatio = coverageWidth / safeRange;
                }

                const bool windowInvalid = (!windowFinite)
                        || (windowWidth <= 0.0)
                        || (upperBound <= scaledMin)
                        || (lowerBound >= scaledMax)
                        || (coverageRatio < 0.02);

                if (windowInvalid)
                {
                        windowWidth = scaledRange;
                        windowCenter = (scaledMax + scaledMin) * 0.5;
                        qCWarning(lcWidget2D)
                                << "[Diagnostics] Replacing DICOM window"
                                << "originalCenter" << originalWindowCenter
                                << "originalWidth" << originalWindowWidth
                                << "scaledMin" << scaledMin
                                << "scaledMax" << scaledMax
                                << "range" << scaledRange;
                }
        }

        frame.DefaultWindowWidth = std::max<double>(windowWidth, 1.0);
        frame.DefaultWindowCenter = windowCenter;
        if (Q_UNLIKELY(lcWidget2D().isDebugEnabled()))
        {
                qCDebug(lcWidget2D)
                        << "[Diagnostics] Frame VOI stats"
                        << "frameIndex" << frame.FrameIndex
                        << "rawMin" << minValue
                        << "rawMax" << maxValue
                        << "scaledMin" << scaledMin
                        << "scaledMax" << scaledMax
                        << "slope" << frame.PixelInfo.RescaleSlope
                        << "intercept" << frame.PixelInfo.RescaleIntercept
                        << "defaultCenter" << frame.DefaultWindowCenter
                        << "defaultWidth" << frame.DefaultWindowWidth;
        }
}

}
