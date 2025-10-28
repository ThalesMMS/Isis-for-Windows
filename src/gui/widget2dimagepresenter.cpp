#include "widget2dimagepresenter.h"

#include <QLoggingCategory>

#include <algorithm>
#include <exception>
#include <utility>
#include <QReadLocker>
#include <QWriteLocker>

#include "image.h"
#include "series.h"
#include "widget2dframebuilder.h"

#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <unordered_map>
#include <vector>

Q_DECLARE_LOGGING_CATEGORY(lcWidget2D)

namespace
{
        using isis::gui::Widget2DScalarType;

        Widget2DScalarType mapVtkScalarTypeToWidgetScalar(const vtkDataArray* scalars)
        {
                if (!scalars)
                {
                        return Widget2DScalarType::Uint8;
                }

                switch (scalars->GetDataType())
                {
                case VTK_CHAR:
                case VTK_SIGNED_CHAR:
                        return Widget2DScalarType::Sint8;
                case VTK_UNSIGNED_CHAR:
                        return Widget2DScalarType::Uint8;
                case VTK_SHORT:
                        return Widget2DScalarType::Sint16;
                case VTK_UNSIGNED_SHORT:
                        return Widget2DScalarType::Uint16;
                case VTK_INT:
                        return Widget2DScalarType::Sint32;
                case VTK_UNSIGNED_INT:
                        return Widget2DScalarType::Uint32;
                case VTK_FLOAT:
                        return Widget2DScalarType::Float32;
                case VTK_DOUBLE:
                        return Widget2DScalarType::Float64;
                default:
                        return Widget2DScalarType::Uint8;
                }
        }
}

namespace isis::gui
{
        Widget2DImagePresenter::Widget2DImagePresenter()
                : m_frameCache(m_cacheMutex, m_totalFrameBytes, m_decodingDurationMs)
        {
        }

        bool Widget2DImagePresenter::isValid() const
        {
        QReadLocker locker(&m_stateLock);
        return !m_frames.isEmpty() && m_volume && m_volume->ImageData;
        }

        int Widget2DImagePresenter::frameCount() const
        {
                QReadLocker locker(&m_stateLock);
                return m_frames.size();
        }

        std::shared_ptr<Widget2DImagePresenter> Widget2DImagePresenter::load(
                core::Series* series,
                core::Image* image)
        {
                if (!image)
                {
                        return {};
                }

                auto presenter = std::make_shared<Widget2DImagePresenter>();
                if (!presenter->loadVolumeForImage(series, image))
                {
                        return presenter;
                }

                presenter->rebuildFramesFromVolume(true);
                return presenter;
        }

        bool Widget2DImagePresenter::loadVolumeForImage(core::Series* series,
                core::Image* image)
        {
        if (!image)
        {
                return false;
        }

        if (!image->getIsMultiFrame())
        {
                return loadVolumeForSeries(series);
        }

        QString failureReason;
        auto volume = image->getDicomVolume(&failureReason);
        if (!volume || !volume->ImageData)
        {
                qCWarning(lcWidget2D)
                        << "Failed to load DICOM volume for image"
                        << QString::fromStdString(image->getImagePath())
                        << "reason:" << failureReason;
                return loadVolumeForSeries(series);
        }

        {
                QWriteLocker locker(&m_stateLock);
                m_volume = std::move(volume);
                m_usesSeriesVolume = false;
                m_frameCache.setVolume(m_volume);
        }
        return true;
}

bool Widget2DImagePresenter::loadVolumeForSeries(core::Series* series)
        {
                if (!series)
                {
                        return false;
        }

        auto volume = series->getVolumeForSingleFrameSeries();
        if (!volume || !volume->ImageData)
        {
                qCWarning(lcWidget2D)
                        << "Failed to load GDCM volume for series"
                        << QString::fromStdString(series->getUID());
                return false;
        }

        {
                QWriteLocker locker(&m_stateLock);
                m_volume = std::move(volume);
                m_usesSeriesVolume = true;
                m_frameCache.setVolume(m_volume);
        }
        return true;
}

bool Widget2DImagePresenter::rebuildFramesFromVolume(const bool resetInitialState)
        {
        QWriteLocker locker(&m_stateLock);
        if (!m_volume || !m_volume->ImageData)
        {
                m_frames.clear();
                m_slicePaths.clear();
                m_totalFrameBytes = 0;
                m_decodingDurationMs = 0;
                return false;
        }

        auto* imageData = m_volume->ImageData.GetPointer();
        auto* pointData = imageData ? imageData->GetPointData() : nullptr;
        auto* scalars = pointData ? pointData->GetScalars() : nullptr;
        if (!scalars)
        {
                m_frames.clear();
                m_slicePaths.clear();
                m_totalFrameBytes = 0;
                m_decodingDurationMs = 0;
                return false;
        }

        int dimensions[3] = {0, 0, 0};
        imageData->GetDimensions(dimensions);
        const int width = dimensions[0];
        const int height = dimensions[1];
        const int depth = std::max(1, dimensions[2]);
        const int samplesPerPixel = (m_volume->PixelInfo.SamplesPerPixel > 0)
                ? m_volume->PixelInfo.SamplesPerPixel
                : scalars->GetNumberOfComponents();
        const int volumeFrames = std::max(m_volume->NumberOfFrames, depth);

        if (resetInitialState)
        {
                m_initialState = {};
        }

        Widget2DFrameBuilder frameBuilder(m_initialState);
        const Widget2DScalarType scalarType = mapVtkScalarTypeToWidgetScalar(scalars);
        QVector<FrameBuffer> frames;
        frames.reserve(volumeFrames);
        m_slicePaths.clear();
        m_slicePaths.reserve(volumeFrames);

        const auto& sourceFiles = m_volume->SourceFiles;

        for (int displayIndex = 0; displayIndex < volumeFrames; ++displayIndex)
        {
                const int sourceIndex = volumeFrames - 1 - displayIndex;
                std::string sourcePath;
                if (!sourceFiles.empty() && sourceIndex >= 0
                        && sourceIndex < static_cast<int>(sourceFiles.size()))
                {
                        sourcePath = sourceFiles[static_cast<std::size_t>(sourceIndex)];
                }
                else
                {
                        sourcePath = std::to_string(sourceIndex);
                }

                const bool updateInitial = (displayIndex == 0);
                FrameBuffer buffer = createFrameBuffer(sourceIndex,
                        width,
                        height,
                        samplesPerPixel,
                        scalarType,
                        updateInitial,
                        sourcePath);
                frames.append(std::move(buffer));
                m_slicePaths.emplace_back(sourcePath);
        }

        m_frames = std::move(frames);
        recalculateFrameTelemetry();

        if (resetInitialState && !m_frames.isEmpty())
        {
                try
                {
                        m_frameCache.ensureFrameCached(m_frames[0]);
                        const double defaultWidth = std::max<double>(m_frames[0].DefaultWindowWidth, 1.0);
                        m_initialState.WindowWidth = defaultWidth;
                        m_initialState.WindowCenter = m_frames[0].DefaultWindowCenter;
                        m_initialState.InvertColors = m_frames[0].PixelInfo.InvertMonochrome;
                }
                catch (const std::exception& ex)
                {
                        qCWarning(lcWidget2D)
                                << "Failed to prime default window state for 2D presenter:"
                                << ex.what();
                }
                catch (...)
                {
                        qCWarning(lcWidget2D)
                                << "Failed to prime default window state for 2D presenter due to an unknown error.";
                }
        }

        return !m_frames.isEmpty();
        }

        Widget2DImagePresenter::FrameBuffer Widget2DImagePresenter::createFrameBuffer(
                const int frameIndex,
                const int width,
                const int height,
                const int samplesPerPixel,
                const Widget2DScalarType scalarType,
                const bool shouldUpdateInitialState,
                const std::string& sourcePath)
        {
                Widget2DFrameBuilder frameBuilder(m_initialState);
                auto buffer = frameBuilder.createFrame(m_volume->PixelInfo,
                        width,
                        height,
                        samplesPerPixel,
                        scalarType,
                        frameIndex,
                        shouldUpdateInitialState);
                buffer.SourcePath = sourcePath;
                buffer.FrameIndex = frameIndex;
                return buffer;
        }

        void Widget2DImagePresenter::recalculateFrameTelemetry()
        {
                m_totalFrameBytes = 0;
                m_decodingDurationMs = 0;
                for (const auto& frame : m_frames)
                {
                        m_totalFrameBytes += static_cast<std::size_t>(frame.Data.size());
                }
        }

        void Widget2DImagePresenter::prefetchAllFrames()
        {
                QReadLocker locker(&m_stateLock);
                m_frameCache.prefetchAllFrames(m_frames);
        }

        std::vector<core::DicomWindowPreset> Widget2DImagePresenter::windowPresets() const
        {
                QReadLocker locker(&m_stateLock);
                if (!m_volume)
                {
                        return {};
                }
                return m_volume->PixelInfo.WindowPresets;
        }

        int Widget2DImagePresenter::appendSingleFrameImages(core::Series* series)
        {
        if (!series)
        {
                return 0;
        }

        if (!m_usesSeriesVolume)
        {
                return 0;
        }

        auto volume = series->getVolumeForSingleFrameSeries();
        if (!volume || !volume->ImageData)
        {
                return 0;
        }

        const auto sortedPaths = series->snapshotSingleFramePaths();
        if (sortedPaths.empty())
        {
                return 0;
        }

        bool matchesCurrentOrder = (m_frames.size() == static_cast<int>(sortedPaths.size()));
        if (matchesCurrentOrder)
        {
                for (std::size_t index = 0; index < sortedPaths.size(); ++index)
                {
                        const std::string& expectedPath = sortedPaths[sortedPaths.size() - 1 - index];
                        if (index >= static_cast<std::size_t>(m_slicePaths.size())
                                || m_slicePaths[static_cast<std::size_t>(index)] != expectedPath)
                        {
                                matchesCurrentOrder = false;
                                break;
                        }
                }
        }

        if (matchesCurrentOrder)
        {
                return 0;
        }

        auto* imageData = volume->ImageData.GetPointer();
        auto* pointData = imageData ? imageData->GetPointData() : nullptr;
        auto* scalars = pointData ? pointData->GetScalars() : nullptr;
        if (!scalars)
        {
                return 0;
        }

        int dimensions[3] = {0, 0, 0};
        imageData->GetDimensions(dimensions);
        const int width = dimensions[0];
        const int height = dimensions[1];
        const int depth = std::max(1, dimensions[2]);
        const int samplesPerPixel = (volume->PixelInfo.SamplesPerPixel > 0)
                ? volume->PixelInfo.SamplesPerPixel
                : scalars->GetNumberOfComponents();
        const int totalFrames = std::max(volume->NumberOfFrames, depth);

        if (static_cast<int>(sortedPaths.size()) != totalFrames)
        {
                return 0;
        }

        const Widget2DScalarType scalarType = mapVtkScalarTypeToWidgetScalar(scalars);

        QWriteLocker writeLock(&m_stateLock);
        if (!m_usesSeriesVolume)
        {
                return 0;
        }

        std::unordered_map<std::string, FrameBuffer> retainedFrames;
        retainedFrames.reserve(m_slicePaths.size());
        for (int index = 0; index < m_frames.size(); ++index)
        {
                if (index < static_cast<int>(m_slicePaths.size()))
                {
                        retainedFrames.emplace(m_slicePaths[static_cast<std::size_t>(index)],
                                std::move(m_frames[index]));
                }
        }

        std::vector<std::string> displayPaths(sortedPaths.rbegin(), sortedPaths.rend());
        std::unordered_map<std::string, int> pathToSourceIndex;
        pathToSourceIndex.reserve(sortedPaths.size());
        for (std::size_t idx = 0; idx < sortedPaths.size(); ++idx)
        {
                pathToSourceIndex.emplace(sortedPaths[idx], static_cast<int>(idx));
        }

        m_frames.clear();
        m_frames.reserve(static_cast<int>(displayPaths.size()));
        m_slicePaths.clear();
        m_slicePaths.reserve(displayPaths.size());
        m_volume = std::move(volume);

        int appendedCount = 0;

        for (std::size_t displayIndex = 0; displayIndex < displayPaths.size(); ++displayIndex)
        {
                const auto& path = displayPaths[displayIndex];
                auto retainedIt = retainedFrames.find(path);
                auto sourceIt = pathToSourceIndex.find(path);
                const int sourceIndex = (sourceIt != pathToSourceIndex.end())
                        ? sourceIt->second
                        : std::clamp(static_cast<int>(sortedPaths.size()) - 1 - static_cast<int>(displayIndex),
                                0,
                                std::max(totalFrames - 1, 0));

                if (retainedIt != retainedFrames.end())
                {
                        FrameBuffer frame = std::move(retainedIt->second);
                        frame.FrameIndex = sourceIndex;
                        m_frames.append(std::move(frame));
                        m_slicePaths.emplace_back(path);
                }
                else
                {
                        const bool updateInitial = m_frames.isEmpty();
                        FrameBuffer frame = createFrameBuffer(sourceIndex,
                                width,
                                height,
                                samplesPerPixel,
                                scalarType,
                                updateInitial,
                                path);
                        frame.FrameIndex = sourceIndex;
                        m_frames.append(std::move(frame));
                        m_slicePaths.emplace_back(path);
                        ++appendedCount;
                }
        }

        retainedFrames.clear();
        recalculateFrameTelemetry();
        m_frameCache.setVolume(m_volume);

        qCInfo(lcWidget2D)
                << "[Telemetry] Presenter synchronized with series"
                << "frames" << m_frames.size()
                << "newFrames" << appendedCount;

        return appendedCount;
        }

        QImage Widget2DImagePresenter::renderFrame(const int frameIndex,
                const Widget2dPresentationState& state)
        {
                QReadLocker locker(&m_stateLock);
                if (frameIndex < 0 || frameIndex >= m_frames.size())
                {
                        return {};
                }

                FrameBuffer& frame = m_frames[frameIndex];
                try
                {
                        m_frameCache.ensureFrameCached(frame);
                }
                catch (const std::exception& ex)
                {
                        qCWarning(lcWidget2D)
                                << "Failed to render frame" << frameIndex
                                << ":" << ex.what();
                        return {};
                }
                catch (...)
                {
                        qCWarning(lcWidget2D)
                                << "Failed to render frame" << frameIndex
                                << ": unknown error";
                        return {};
                }

                return m_frameRenderer.renderFrame(frame, state);
        }
bool Widget2DImagePresenter::sampleValue(const int frameIndex, const int x, const int y, double& storedValue, double& huValue)
{
        QReadLocker locker(&m_stateLock);
        if (frameIndex < 0 || frameIndex >= m_frames.size())
        {
                return false;
        }

        FrameBuffer& frame = m_frames[frameIndex];
        if (x < 0 || y < 0 || x >= frame.Width || y >= frame.Height)
        {
                return false;
        }

        try
        {
                m_frameCache.ensureFrameCached(frame);
        }
        catch (...)
        {
                return false;
        }

        if (frame.Data.isEmpty())
        {
                return false;
        }

        const int components = std::max(frame.SamplesPerPixel, 1);
        const std::size_t offsetIndex = static_cast<std::size_t>(y)
                * static_cast<std::size_t>(frame.Width)
                * static_cast<std::size_t>(components)
                + static_cast<std::size_t>(x) * static_cast<std::size_t>(components);
        const std::size_t bytesPerSample = Widget2DFrameBuilder::bytesPerSample(frame.ScalarType);
        if (bytesPerSample == 0)
        {
                return false;
        }

        const std::size_t byteOffset = offsetIndex * bytesPerSample;
        if (byteOffset + bytesPerSample > static_cast<std::size_t>(frame.Data.size()))
        {
                return false;
        }

        const char* basePtr = frame.Data.constData() + static_cast<int>(byteOffset);
        double sample = 0.0;

        switch (frame.ScalarType)
        {
        case Widget2DScalarType::Uint8:
                sample = static_cast<double>(*reinterpret_cast<const std::uint8_t*>(basePtr));
                break;
        case Widget2DScalarType::Sint8:
                sample = static_cast<double>(*reinterpret_cast<const std::int8_t*>(basePtr));
                break;
        case Widget2DScalarType::Uint16:
                sample = static_cast<double>(*reinterpret_cast<const std::uint16_t*>(basePtr));
                break;
        case Widget2DScalarType::Sint16:
                sample = static_cast<double>(*reinterpret_cast<const std::int16_t*>(basePtr));
                break;
        case Widget2DScalarType::Uint32:
                sample = static_cast<double>(*reinterpret_cast<const std::uint32_t*>(basePtr));
                break;
        case Widget2DScalarType::Sint32:
                sample = static_cast<double>(*reinterpret_cast<const std::int32_t*>(basePtr));
                break;
        case Widget2DScalarType::Float32:
                sample = static_cast<double>(*reinterpret_cast<const float*>(basePtr));
                break;
        case Widget2DScalarType::Float64:
                sample = *reinterpret_cast<const double*>(basePtr);
                break;
        default:
                return false;
        }

        storedValue = sample;
        const double slope = frame.PixelInfo.RescaleSlope;
        const double intercept = frame.PixelInfo.RescaleIntercept;
        huValue = sample * slope + intercept;
        return true;
}
}
