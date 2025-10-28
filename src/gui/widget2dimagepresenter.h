/*
 * ------------------------------------------------------------------------------------
 *  File: widget2dimagepresenter.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Provides a reusable GDCM-backed frame loader/renderer for the 2D widget,
 *      handling pixel buffer caching, VOI defaults, and asynchronous prefetching.
 *
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <QImage>
#include <QMutex>
#include <QReadLocker>
#include <QReadWriteLock>
#include <QVector>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "widget2dframecache.h"
#include "widget2dframerenderer.h"
#include "widget2dimageframe.h"
#include "widget2dpresentationstate.h"

namespace isis::core
{
        class Series;
        class Image;
        struct DicomVolume;
}

namespace isis::gui
{
        class Widget2DImagePresenter
        {
        public:
                using FrameBuffer = Widget2DImageFrame;

                Widget2DImagePresenter();
                ~Widget2DImagePresenter() = default;

                [[nodiscard]] bool isValid() const;
                [[nodiscard]] int frameCount() const;
                [[nodiscard]] Widget2dPresentationState initialState() const
                {
                        QReadLocker locker(&m_stateLock);
                        return m_initialState;
                }
                [[nodiscard]] QImage renderFrame(int frameIndex, const Widget2dPresentationState& state);
                bool sampleValue(int frameIndex, int x, int y, double& storedValue, double& huValue);
                [[nodiscard]] std::size_t totalAllocatedFrameBytes() const
                {
                        QReadLocker locker(&m_stateLock);
                        return m_totalFrameBytes;
                }
                [[nodiscard]] qint64 decodingDurationMs() const
                {
                        QReadLocker locker(&m_stateLock);
                        return m_decodingDurationMs;
                }
                void prefetchAllFrames();
                int appendSingleFrameImages(isis::core::Series* series);
                [[nodiscard]] std::vector<core::DicomWindowPreset> windowPresets() const;

                static std::shared_ptr<Widget2DImagePresenter> load(
                        isis::core::Series* series,
                        isis::core::Image* image);

        private:
                Widget2dPresentationState m_initialState = {};
                QVector<FrameBuffer> m_frames = {};
                std::shared_ptr<const core::DicomVolume> m_volume = {};
                std::size_t m_totalFrameBytes = 0;
                qint64 m_decodingDurationMs = 0;
                mutable QMutex m_cacheMutex = {};
                mutable QReadWriteLock m_stateLock;
                bool m_usesSeriesVolume = false;
                std::vector<std::string> m_slicePaths = {};
                Widget2DFrameCache m_frameCache;
                Widget2DFrameRenderer m_frameRenderer;

                bool loadVolumeForImage(core::Series* series, core::Image* image);
                bool loadVolumeForSeries(core::Series* series);
                bool rebuildFramesFromVolume(bool resetInitialState);
                FrameBuffer createFrameBuffer(int frameIndex,
                        int width,
                        int height,
                        int samplesPerPixel,
                        Widget2DScalarType scalarType,
                        bool shouldUpdateInitialState,
                        const std::string& sourcePath);
                void recalculateFrameTelemetry();
        };
}

