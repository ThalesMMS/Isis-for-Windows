#include "widget2dframecache.h"

#include "widget2dframebuilder.h"

#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QMutexLocker>
#include <QtConcurrent/QtConcurrentMap>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>

Q_DECLARE_LOGGING_CATEGORY(lcWidget2D)

namespace isis::gui
{

Widget2DFrameCache::Widget2DFrameCache(QMutex& cacheMutex,
        std::size_t& totalFrameBytes,
        qint64& decodingDurationMs)
        : m_cacheMutex(cacheMutex)
        , m_totalFrameBytes(totalFrameBytes)
        , m_decodingDurationMs(decodingDurationMs)
{
}

void Widget2DFrameCache::setVolume(const std::shared_ptr<const isis::core::DicomVolume>& volume)
{
        QMutexLocker locker(&m_cacheMutex);
        m_volume = volume;
}

void Widget2DFrameCache::ensureFrameCached(Widget2DImageFrame& frame)
{
        const bool debugLoggingEnabled = Q_UNLIKELY(lcWidget2D().isDebugEnabled());
        if (debugLoggingEnabled)
        {
                qCDebug(lcWidget2D)
                        << "ensureFrameCached called for frame"
                        << frame.FrameIndex
                        << "with cached status"
                        << frame.Cached;
        }

        std::shared_ptr<const isis::core::DicomVolume> volume;
        isis::core::DicomPixelInfo volumePixelInfo = {};
        bool volumePixelInfoValid = false;
        int frameIndex = 0;

        m_cacheMutex.lock();
        while (frame.Decoding)
        {
                m_decodeWait.wait(&m_cacheMutex);
        }
        if (frame.Cached)
        {
                if (debugLoggingEnabled)
                {
                        qCDebug(lcWidget2D)
                                << "[Telemetry] Frame already cached during decode"
                                << "frameIndex"
                                << frame.FrameIndex;
                }
                m_cacheMutex.unlock();
                return;
        }

        frame.Decoding = true;
        frameIndex = frame.FrameIndex;
        volume = m_volume.lock();
        if (volume)
        {
                volumePixelInfo = volume->PixelInfo;
                volumePixelInfoValid = true;
        }
        m_cacheMutex.unlock();

        if (!volume || !volume->ImageData)
        {
                throw std::runtime_error("No DICOM volume available for frame caching.");
        }

        auto* imageData = volume->ImageData.GetPointer();
        auto* pointData = imageData ? imageData->GetPointData() : nullptr;
        auto* scalars = pointData ? pointData->GetScalars() : nullptr;
        if (!scalars)
        {
                throw std::runtime_error("Volume scalar data is unavailable.");
        }

        const int width = frame.Width > 0 ? frame.Width : imageData->GetDimensions()[0];
        const int height = frame.Height > 0 ? frame.Height : imageData->GetDimensions()[1];
        const int components = std::max(frame.SamplesPerPixel, scalars->GetNumberOfComponents());
        const std::size_t bytesPerValue = Widget2DFrameBuilder::bytesPerSample(frame.ScalarType);
        if (bytesPerValue == 0)
        {
                throw std::runtime_error("Unsupported scalar representation encountered during caching.");
        }

        const std::size_t frameSampleCount = static_cast<std::size_t>(width)
                * static_cast<std::size_t>(height)
                * static_cast<std::size_t>(components);
        const std::size_t frameByteCount = frameSampleCount * bytesPerValue;
        if (frameByteCount > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        {
                throw std::runtime_error("Frame data exceeds supported size.");
        }

        const unsigned char* const dataBegin = static_cast<const unsigned char*>(scalars->GetVoidPointer(0));
        if (!dataBegin)
        {
                throw std::runtime_error("Unable to access GDCM pixel buffer.");
        }

        const vtkIdType tupleCount = scalars->GetNumberOfTuples();
        const std::size_t totalByteCount = static_cast<std::size_t>(tupleCount)
                * static_cast<std::size_t>(scalars->GetNumberOfComponents())
                * static_cast<std::size_t>(scalars->GetDataTypeSize());

        const std::size_t offset = static_cast<std::size_t>(frameIndex) * frameByteCount;
        if (offset + frameByteCount > totalByteCount)
        {
                throw std::runtime_error("Requested frame index exceeds available scalar data.");
        }

        QElapsedTimer decodeTimer;
        decodeTimer.start();

        std::size_t frameBytes = 0;
        qint64 decodeDuration = 0;
        int loggedWidth = 0;
        int loggedHeight = 0;
        int loggedSamplesPerPixel = 0;
        unsigned long long accumulatedBytes = 0;

        try
        {
                QByteArray frameBuffer(static_cast<int>(frameByteCount), Qt::Uninitialized);
                std::memcpy(frameBuffer.data(),
                        dataBegin + offset,
                        frameByteCount);
                decodeDuration = decodeTimer.elapsed();
                frameBytes = static_cast<std::size_t>(frameBuffer.size());

                m_cacheMutex.lock();
                try
                {
                        if (volumePixelInfoValid)
                        {
                                const double slopeDelta = std::abs(frame.PixelInfo.RescaleSlope - volumePixelInfo.RescaleSlope);
                                const double interceptDelta = std::abs(frame.PixelInfo.RescaleIntercept - volumePixelInfo.RescaleIntercept);
                                if (Q_UNLIKELY(slopeDelta > 1e-6 || interceptDelta > 1e-3))
                                {
                                        qCWarning(lcWidget2D)
                                                << "Frame pixel rescale parameters differed from volume during cache fill."
                                                << "frameIndex" << frame.FrameIndex
                                                << "frameSlope" << frame.PixelInfo.RescaleSlope
                                                << "frameIntercept" << frame.PixelInfo.RescaleIntercept
                                                << "volumeSlope" << volumePixelInfo.RescaleSlope
                                                << "volumeIntercept" << volumePixelInfo.RescaleIntercept;
                                }
                                frame.PixelInfo = volumePixelInfo;
                        }

                        frame.Width = width;
                        frame.Height = height;
                        frame.SamplesPerPixel = components;
                        frame.PixelInfo.SamplesPerPixel = components;
                        frame.Data = std::move(frameBuffer);
                        Widget2DFrameBuilder::calculateDefaultVOIWindow(frame);
                        if (debugLoggingEnabled)
                        {
                                qCDebug(lcWidget2D)
                                        << "[Diagnostics] Frame default VOI window"
                                        << "frameIndex" << frame.FrameIndex
                                        << "windowCenter" << frame.DefaultWindowCenter
                                        << "windowWidth" << frame.DefaultWindowWidth;
                        }

                        frame.Cached = true;
                        frame.Decoding = false;
                        m_totalFrameBytes += frameBytes;
                        m_decodingDurationMs += decodeDuration;
                        loggedWidth = frame.Width;
                        loggedHeight = frame.Height;
                        loggedSamplesPerPixel = frame.SamplesPerPixel;
                        accumulatedBytes = static_cast<unsigned long long>(m_totalFrameBytes);
                        m_decodeWait.wakeAll();
                        m_cacheMutex.unlock();
                }
                catch (...)
                {
                        frame.Decoding = false;
                        m_decodeWait.wakeAll();
                        m_cacheMutex.unlock();
                        throw;
                }

                if (debugLoggingEnabled)
                {
                        qCDebug(lcWidget2D)
                                << "[Telemetry] Frame cached"
                                << "frameIndex"
                                << frameIndex
                                << "bytes"
                                << static_cast<unsigned long long>(frameBytes)
                                << "accumulatedBytes"
                                << accumulatedBytes
                                << "width"
                                << loggedWidth
                                << "height"
                                << loggedHeight
                                << "samplesPerPixel"
                                << loggedSamplesPerPixel;
                }
        }
        catch (...)
        {
                m_cacheMutex.lock();
                frame.Decoding = false;
                m_decodeWait.wakeAll();
                m_cacheMutex.unlock();
                throw;
        }
}

void Widget2DFrameCache::prefetchAllFrames(QVector<Widget2DImageFrame>& frames)
{
        QVector<Widget2DImageFrame*> framesToDecode;
        framesToDecode.reserve(frames.size());

        m_cacheMutex.lock();
        for (Widget2DImageFrame& frame : frames)
        {
                while (frame.Decoding)
                {
                        m_decodeWait.wait(&m_cacheMutex);
                }

                if (!frame.Cached)
                {
                        framesToDecode.append(&frame);
                }
        }
        m_cacheMutex.unlock();

        if (framesToDecode.isEmpty())
        {
                return;
        }

        auto decodeTask = [this](Widget2DImageFrame*& framePtr)
        {
                if (!framePtr)
                {
                        return;
                }

                try
                {
                        ensureFrameCached(*framePtr);
                }
                catch (const std::exception& ex)
                {
                        qCWarning(lcWidget2D)
                                << "Background prefetch failed:"
                                << ex.what();
                }
                catch (...)
                {
                        qCWarning(lcWidget2D)
                                << "Background prefetch failed due to an unknown error.";
                }
        };

        QtConcurrent::blockingMap(framesToDecode, decodeTask);
}

}
