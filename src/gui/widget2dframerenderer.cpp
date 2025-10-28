/*
 * ------------------------------------------------------------------------------------
 *  File: widget2dframerenderer.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Implements helpers that map cached frame data into Qt images honouring VOI window,
 *      palette inversion, flipping, and rotations.
 * ------------------------------------------------------------------------------------
 */

#include "widget2dframerenderer.h"

#include "widget2dframebuilder.h"
#include "widget2dpresentationstate.h"

#include <QTransform>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>

namespace isis::gui
{

namespace
{
        constexpr double kWindowWidthEpsilon = 1e-6;

        double toDouble(const Widget2DImageFrame& frame, std::size_t index)
        {
                switch (frame.ScalarType)
                {
                case Widget2DScalarType::Uint8:
                        return static_cast<double>(reinterpret_cast<const std::uint8_t*>(frame.Data.constData())[index]);
                case Widget2DScalarType::Sint8:
                        return static_cast<double>(reinterpret_cast<const std::int8_t*>(frame.Data.constData())[index]);
                case Widget2DScalarType::Uint16:
                        return static_cast<double>(reinterpret_cast<const std::uint16_t*>(frame.Data.constData())[index]);
                case Widget2DScalarType::Sint16:
                        return static_cast<double>(reinterpret_cast<const std::int16_t*>(frame.Data.constData())[index]);
                case Widget2DScalarType::Uint32:
                        return static_cast<double>(reinterpret_cast<const std::uint32_t*>(frame.Data.constData())[index]);
                case Widget2DScalarType::Sint32:
                        return static_cast<double>(reinterpret_cast<const std::int32_t*>(frame.Data.constData())[index]);
                case Widget2DScalarType::Float32:
                        return static_cast<double>(reinterpret_cast<const float*>(frame.Data.constData())[index]);
                case Widget2DScalarType::Float64:
                        return reinterpret_cast<const double*>(frame.Data.constData())[index];
                default:
                        return 0.0;
                }
        }
}

Widget2DFrameRenderer::Widget2DFrameRenderer() = default;

QImage Widget2DFrameRenderer::renderFrame(Widget2DImageFrame& frame,
        const Widget2dPresentationState& state) const
{
        if (frame.Width <= 0 || frame.Height <= 0 || frame.Data.isEmpty())
        {
                return {};
        }

        const bool monochrome = frame.SamplesPerPixel <= 1;
        QImage image;

        if (monochrome)
        {
                image = QImage(frame.Width, frame.Height, QImage::Format_Indexed8);
                if (image.isNull())
                {
                        return image;
                }
                image.setColorTable(grayscaleColorTable());

                const double defaultWidth = std::max<double>(frame.DefaultWindowWidth, 1.0);
                const double defaultCenter = frame.DefaultWindowCenter;

                double windowWidth = state.WindowWidth > 0.0 ? state.WindowWidth : defaultWidth;
                double windowCenter = state.WindowWidth > 0.0 ? state.WindowCenter : defaultCenter;

                if (windowWidth <= 0.0 || std::abs(windowWidth) < kWindowWidthEpsilon)
                {
                        windowWidth = defaultWidth;
                }

                const double lowerBound = windowCenter - (windowWidth / 2.0);
                const double upperBound = windowCenter + (windowWidth / 2.0);
                const double denominator = std::max<double>(upperBound - lowerBound, kWindowWidthEpsilon);
                const double invDenominator = 1.0 / denominator;
                const double scale = 255.0 * invDenominator;

                const double rescaleSlope = frame.PixelInfo.RescaleSlope;
                const double rescaleIntercept = frame.PixelInfo.RescaleIntercept;
                constexpr double kRescaleEpsilon = 1e-6;
                const bool identityRescale = (std::abs(rescaleSlope - 1.0) <= kRescaleEpsilon)
                        && (std::abs(rescaleIntercept) <= kRescaleEpsilon);

                std::function<unsigned char(double)> mapValue;
                if (identityRescale)
                {
                        mapValue = [scale, lowerBound](const double value)
                        {
                                const double scaled = (value - lowerBound) * scale;
                                const double clamped = std::clamp(scaled, 0.0, 255.0);
                                return static_cast<unsigned char>(std::lround(clamped));
                        };
                }
                else
                {
                        mapValue = [scale, lowerBound, rescaleSlope, rescaleIntercept](const double value)
                        {
                                const double rescaled = value * rescaleSlope + rescaleIntercept;
                                const double scaled = (rescaled - lowerBound) * scale;
                                const double clamped = std::clamp(scaled, 0.0, 255.0);
                                return static_cast<unsigned char>(std::lround(clamped));
                        };
                }

                auto* destination = image.bits();
                const auto bytesPerLine = static_cast<std::size_t>(image.bytesPerLine());
                for (int row = 0; row < frame.Height; ++row)
                {
                        auto* destRow = destination + static_cast<std::size_t>(row) * bytesPerLine;
                        const std::size_t baseIndex = static_cast<std::size_t>(row) * frame.Width;
                        for (int col = 0; col < frame.Width; ++col)
                        {
                                const auto value = toDouble(frame, baseIndex + static_cast<std::size_t>(col));
                                destRow[col] = mapValue(value);
                        }
                }
        }
        else
        {
                image = QImage(frame.Width, frame.Height, QImage::Format_RGB888);
                if (image.isNull())
                {
                        return image;
                }

                const bool planarRgb = (frame.SamplesPerPixel >= 3) && frame.PixelInfo.IsPlanar;
                const std::size_t pixelCount = static_cast<std::size_t>(frame.Width)
                        * static_cast<std::size_t>(frame.Height);
                const double rescaleSlope = frame.PixelInfo.RescaleSlope;
                const double rescaleIntercept = frame.PixelInfo.RescaleIntercept;

                auto toRgbComponent = [rescaleSlope, rescaleIntercept](double value) -> unsigned char
                {
                        const double rescaled = value * rescaleSlope + rescaleIntercept;
                        const double clamped = std::clamp(rescaled, 0.0, 255.0);
                        return static_cast<unsigned char>(std::lround(clamped));
                };

                if (planarRgb)
                {
                        const std::size_t planeSize = pixelCount;
                        for (int row = 0; row < frame.Height; ++row)
                        {
                                auto* dst = image.scanLine(row);
                                for (int col = 0; col < frame.Width; ++col)
                                {
                                        const std::size_t pixelIndex = static_cast<std::size_t>(row)
                                                * static_cast<std::size_t>(frame.Width)
                                                + static_cast<std::size_t>(col);
                                        const auto r = toRgbComponent(toDouble(frame, pixelIndex));
                                        const auto g = toRgbComponent(toDouble(frame, planeSize + pixelIndex));
                                        const auto b = toRgbComponent(toDouble(frame, 2 * planeSize + pixelIndex));
                                        dst[col * 3 + 0] = r;
                                        dst[col * 3 + 1] = g;
                                        dst[col * 3 + 2] = b;
                                }
                        }
                }
                else
                {
                        const int components = std::max(frame.SamplesPerPixel, 1);
                        for (int row = 0; row < frame.Height; ++row)
                        {
                                auto* dst = image.scanLine(row);
                                for (int col = 0; col < frame.Width; ++col)
                                {
                                        const std::size_t pixelIndex = static_cast<std::size_t>(row)
                                                * static_cast<std::size_t>(frame.Width)
                                                + static_cast<std::size_t>(col);
                                        const std::size_t base = pixelIndex * static_cast<std::size_t>(components);
                                        const auto r = toRgbComponent(toDouble(frame, base + 0));
                                        const auto g = (components > 1)
                                                ? toRgbComponent(toDouble(frame, base + 1))
                                                : r;
                                        const auto b = (components > 2)
                                                ? toRgbComponent(toDouble(frame, base + 2))
                                                : g;
                                        dst[col * 3 + 0] = r;
                                        dst[col * 3 + 1] = g;
                                        dst[col * 3 + 2] = b;
                                }
                        }
                }
        }

        if (state.InvertColors)
        {
                image.invertPixels(QImage::InvertRgb);
        }

        if (state.FlipHorizontal || state.FlipVertical)
        {
                image = image.mirrored(state.FlipHorizontal, state.FlipVertical);
        }

        const int rotationSteps = ((state.RotationSteps % 4) + 4) % 4;
        if (rotationSteps != 0)
        {
                QTransform transform;
                transform.rotate(90.0 * rotationSteps);
                image = image.transformed(transform, Qt::FastTransformation);
        }

        return image;
}

const QVector<QRgb>& Widget2DFrameRenderer::grayscaleColorTable()
{
        static QVector<QRgb> colorTable;
        if (colorTable.isEmpty())
        {
                colorTable.reserve(256);
                for (int index = 0; index < 256; ++index)
                {
                        colorTable.append(qRgb(index, index, index));
                }
        }
        return colorTable;
}

}
