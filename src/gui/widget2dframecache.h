/*
 * ------------------------------------------------------------------------------------
 *  File: widget2dframecache.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Declares helpers that ensure 2D DICOM frames are decoded and cached, handling
 *      telemetry counters and background prefetching.
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <memory>
#include <QtGlobal>

#include <QMutex>
#include <QVector>
#include <QWaitCondition>

#include "widget2dimageframe.h"

namespace isis::gui
{
	class Widget2DFrameCache
	{
	public:
		Widget2DFrameCache(QMutex& cacheMutex,
			std::size_t& totalFrameBytes,
			qint64& decodingDurationMs);

                void setVolume(const std::shared_ptr<const isis::core::DicomVolume>& volume);
		void ensureFrameCached(Widget2DImageFrame& frame);
		void prefetchAllFrames(QVector<Widget2DImageFrame>& frames);

        private:
                QMutex& m_cacheMutex;
                std::size_t& m_totalFrameBytes;
                qint64& m_decodingDurationMs;
                QWaitCondition m_decodeWait;
                std::weak_ptr<const isis::core::DicomVolume> m_volume;
	};
}
