/*
 * ------------------------------------------------------------------------------------
 *  File: widget2d_rescale_sync_test.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Regression test for Widget2D rescale parameter synchronization.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "gui/widget2dframecache.h"
#include "gui/widget2dframerenderer.h"
#include "gui/widget2dimageframe.h"
#include "gui/widget2dpresentationstate.h"

#include "core/dicomvolume.h"

#include <QCoreApplication>
#include <QImage>
#include <QLoggingCategory>
#include <QMutex>

#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>

Q_LOGGING_CATEGORY(lcWidget2D, "isis.gui.widget2d")

namespace
{

        std::shared_ptr<isis::core::DicomVolume> createRescaledVolume()
        {
                auto volume = std::make_shared<isis::core::DicomVolume>();
                auto imageData = vtkSmartPointer<vtkImageData>::New();
                imageData->SetDimensions(2, 2, 1);
                imageData->AllocateScalars(VTK_FLOAT, 1);

                auto* scalarPointer = static_cast<float*>(imageData->GetScalarPointer());
                if (!scalarPointer)
                {
                        throw std::runtime_error("Failed to allocate VTK scalars.");
                }

                const float values[4] = {-10.0F, 0.0F, 10.0F, 20.0F};
                for (int index = 0; index < 4; ++index)
                {
                        scalarPointer[index] = values[index];
                }

                volume->ImageData = imageData;
                volume->PixelInfo.SamplesPerPixel = 1;
                volume->PixelInfo.RescaleSlope = 1.0;
                volume->PixelInfo.RescaleIntercept = 0.0;
                volume->PixelInfo.WindowCenter = 5.0;
                volume->PixelInfo.WindowWidth = 20.0;
                volume->NumberOfFrames = 1;
                return volume;
        }

        std::shared_ptr<isis::core::DicomVolume> createWideRangeVolume()
        {
                auto volume = std::make_shared<isis::core::DicomVolume>();
                auto imageData = vtkSmartPointer<vtkImageData>::New();
                imageData->SetDimensions(2, 2, 1);
                imageData->AllocateScalars(VTK_FLOAT, 1);

                auto* scalarPointer = static_cast<float*>(imageData->GetScalarPointer());
                if (!scalarPointer)
                {
                        throw std::runtime_error("Failed to allocate VTK scalars for wide range test.");
                }

                const float values[4] = {-8208.0F, 12000.0F, 20000.0F, 30000.0F};
                for (int index = 0; index < 4; ++index)
                {
                        scalarPointer[index] = values[index];
                }

                volume->ImageData = imageData;
                volume->PixelInfo.SamplesPerPixel = 1;
                volume->PixelInfo.RescaleSlope = 1.0;
                volume->PixelInfo.RescaleIntercept = -9215.0;
                volume->PixelInfo.WindowCenter = 40.0;
                volume->PixelInfo.WindowWidth = 400.0;
                volume->NumberOfFrames = 1;
                return volume;
        }

        bool checkApproximate(int value, int expected, int tolerance)
        {
                return std::abs(value - expected) <= tolerance;
        }
}

int main()
{
        try
        {
                int argc = 1;
                char appName[] = "widget2d_rescale_sync_test";
                char* argv[] = {appName, nullptr};
                QCoreApplication app(argc, argv);

                auto volume = createRescaledVolume();

                isis::gui::Widget2DImageFrame frame;
                frame.FrameIndex = 0;
                frame.Width = 2;
                frame.Height = 2;
                frame.SamplesPerPixel = 1;
                frame.ScalarType = isis::gui::Widget2DScalarType::Float32;
                frame.PixelInfo.SamplesPerPixel = 1;
                frame.PixelInfo.RescaleSlope = 2.0;
                frame.PixelInfo.RescaleIntercept = -10.0;

                QMutex cacheMutex;
                std::size_t totalBytes = 0;
                qint64 decodingDurationMs = 0;
                isis::gui::Widget2DFrameCache cache(cacheMutex, totalBytes, decodingDurationMs);
                cache.setVolume(volume);

                cache.ensureFrameCached(frame);

                const double slopeDelta =
                        std::abs(frame.PixelInfo.RescaleSlope - volume->PixelInfo.RescaleSlope);
                const double interceptDelta =
                        std::abs(frame.PixelInfo.RescaleIntercept - volume->PixelInfo.RescaleIntercept);
                if (slopeDelta > 1e-6 || interceptDelta > 1e-3)
                {
                        throw std::runtime_error("Frame pixel info did not synchronize with volume data.");
                }

                if (frame.Data.size() != static_cast<int>(4 * sizeof(float)))
                {
                        throw std::runtime_error("Unexpected frame buffer size after caching.");
                }

                isis::gui::Widget2dPresentationState state;
                state.WindowCenter = 5.0;
                state.WindowWidth = 20.0;

                isis::gui::Widget2DFrameRenderer renderer;
                const QImage rendered = renderer.renderFrame(frame, state);
                if (rendered.isNull())
                {
                        throw std::runtime_error("Renderer returned a null image.");
                }
                if (rendered.format() != QImage::Format_Indexed8)
                {
                        throw std::runtime_error("Renderer produced an unexpected pixel format.");
                }

                const int stride = rendered.bytesPerLine();
                const uchar* data = rendered.constBits();
                const uchar topLeft = data[0];
                const uchar topRight = data[1];
                const uchar bottomLeft = data[stride];
                const uchar bottomRight = data[stride + 1];

                if (!checkApproximate(topLeft, 0, 1))
                {
                        throw std::runtime_error("Top-left pixel expected to map near 0.");
                }
                if (!checkApproximate(topRight, 64, 8))
                {
                        throw std::runtime_error("Top-right pixel did not map near expected intensity.");
                }
                if (!checkApproximate(bottomLeft, 191, 8))
                {
                        throw std::runtime_error("Bottom-left pixel did not map near expected intensity.");
                }
                if (!checkApproximate(bottomRight, 255, 0))
                {
                        throw std::runtime_error("Bottom-right pixel expected to saturate at 255.");
                }

                auto wideVolume = createWideRangeVolume();
                isis::gui::Widget2DImageFrame wideFrame;
                wideFrame.FrameIndex = 0;
                wideFrame.Width = 2;
                wideFrame.Height = 2;
                wideFrame.SamplesPerPixel = 1;
                wideFrame.ScalarType = isis::gui::Widget2DScalarType::Float32;
                wideFrame.PixelInfo.SamplesPerPixel = 1;
                wideFrame.PixelInfo.RescaleSlope = 1.0;
                wideFrame.PixelInfo.RescaleIntercept = -9215.0;
                wideFrame.PixelInfo.WindowCenter = 40.0;
                wideFrame.PixelInfo.WindowWidth = 400.0;

                cache.setVolume(wideVolume);
                cache.ensureFrameCached(wideFrame);

                const double expectedRange = wideFrame.MaxValue - wideFrame.MinValue;
                const double rangeDelta = std::abs(wideFrame.DefaultWindowWidth - expectedRange);
                if (expectedRange <= 0.0 || rangeDelta > expectedRange * 0.05)
                {
                        throw std::runtime_error("Fallback VOI window width mismatch for wide range data.");
                }

                const double expectedCenter = (wideFrame.MaxValue + wideFrame.MinValue) * 0.5;
                const double centerDelta = std::abs(wideFrame.DefaultWindowCenter - expectedCenter);
                if (centerDelta > std::max(1.0, std::abs(expectedCenter) * 0.01))
                {
                        throw std::runtime_error("Fallback VOI window center mismatch for wide range data.");
                }

                std::cout << "widget2d_rescale_sync_test passed" << std::endl;
                return EXIT_SUCCESS;
        }
        catch (const std::exception& ex)
        {
                std::cerr << "widget2d_rescale_sync_test failed: " << ex.what() << std::endl;
        }
        catch (...)
        {
                std::cerr << "widget2d_rescale_sync_test failed: unknown error" << std::endl;
        }
        return EXIT_FAILURE;
}
