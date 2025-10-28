/*
 * ------------------------------------------------------------------------------------
 *  File: widget2dloadcontroller.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements the helper that manages asynchronous frame loading for Widget2D.
 *
 *  License:
 *      This file is part of a derived work based on the Asclepios DICOM Viewer,
 *      licensed under the MIT License.
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a copy
 *      of this software and associated documentation files (the "Software"), to deal
 *      in the Software without restriction, including without limitation the rights
 *      to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *      copies of the Software, and to permit persons to do so, subject to the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included in
 *      all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *      FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *      AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *      LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *      OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *      SOFTWARE.
 * ------------------------------------------------------------------------------------
 */

#include "widget2dloadcontroller.h"

#include <QElapsedTimer>
#include <QEventLoop>
#include <QLabel>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QObject>
#include <QThread>
#include <QtConcurrent/QtConcurrent>

#include <algorithm>

#include "widget2d.h"
#include "widget2doverlayupdater.h"
#include "widget2dimagepresenter.h"
#include "widget2dinteractor.h"
#include "widget2drenderer.h"
#include "widget2dstate.h"
#include "loadinganimation.h"
#include "tabwidget.h"
#include "series.h"
#include "image.h"

Q_DECLARE_LOGGING_CATEGORY(lcWidget2D)

namespace isis::gui
{
        Widget2DLoadController::Widget2DLoadController(Widget2D& t_widget)
                : QObject()
                , m_widget(t_widget)
        {
        }

        bool Widget2DLoadController::startRendering()
        {
        qCInfo(lcWidget2D) << "Starting GDCM-based 2D rendering pipeline.";
                try
                {
                        cancelPrefetchIfRunning();

                        const auto singleFrameCount = static_cast<int>(m_widget.m_series->snapshotSingleFramePaths().size());
                        const auto expectedFrames = m_widget.m_image->getIsMultiFrame()
                                ? m_widget.m_image->getNumberOfFrames()
                                : singleFrameCount;
                        qCInfo(lcWidget2D)
                                << "Render requested. Multi-frame:" << m_widget.m_image->getIsMultiFrame()
                                << "expected frames:" << expectedFrames
                                << "series UID:" << QString::fromStdString(m_widget.m_series->getUID())
                                << "series index:" << m_widget.m_series->getIndex()
                                << "image SOP UID:" << QString::fromStdString(m_widget.m_image->getSOPInstanceUID())
                                << "image index:" << m_widget.m_image->getIndex()
                                << "path:" << QString::fromStdString(m_widget.m_image->getImagePath());

                        m_widget.ensureImageLabel();
                        m_widget.hideOverlayWidget();
                        if (m_widget.m_errorLabel)
                        {
                                m_widget.m_errorLabel->hide();
                        }
                        if (m_widget.m_imageLabel)
                        {
                                m_widget.m_imageLabel->clear();
                                m_widget.m_imageLabel->hide();
                        }
                        if (m_widget.m_tabWidget)
                        {
                                m_widget.m_tabWidget->setAcceptDrops(false);
                        }

                        ensureImageLoadWatcher();

                        if (m_widget.m_scroll)
                        {
                                m_widget.m_scroll->hide();
                                m_widget.m_scroll->setValue(0);
                                m_widget.m_scroll->setMaximum(0);
                        }

                        m_widget.startLoadingAnimation();

                        resetWidgetStateForLoad();
                        m_widget.updateActiveToolUi();
                        qCInfo(lcWidget2D)
                                << "QtConcurrent::run dispatched for GDCM pipeline. Running:"
                                << m_imageLoadFuture.isRunning();
                        return true;
                }
                catch (const std::exception& ex)
                {
                        handleFailure(QString::fromUtf8(ex.what()));
                }
                catch (...)
                {
                        handleFailure({});
                }

                return false;
        }

        void Widget2DLoadController::handleFailure(const QString& t_reason)
        {
                m_widget.stopLoadingAnimation();
                cancelPrefetchIfRunning();
                m_widget.m_pendingScrollRefresh = false;
                m_widget.m_pendingSeriesRefresh = nullptr;
                m_widget.m_pendingImageRefresh = nullptr;
                if (!t_reason.isEmpty())
                {
                        qCWarning(lcWidget2D) << "GDCM rendering pipeline failed:" << t_reason;
                }
                else
                {
                        qCWarning(lcWidget2D) << "GDCM rendering pipeline failed.";
                }

                m_widget.m_renderingActive = false;
                m_widget.m_imagePresenter.reset();
                m_widget.m_state.setPresentation({});
                m_widget.m_cachedFrame = {};
                m_widget.m_displayZoomFactor = 1.0;
                m_widget.m_state.setDisplayZoomFactor(1.0);
                m_widget.m_manualZoomFactor = 1.0;
                m_widget.m_state.setManualZoomFactor(1.0);
                m_widget.m_fitToWindowEnabled = false;
                m_widget.m_state.setFitToWindowEnabled(false);
                m_widget.m_windowLevelDragging = false;
                m_widget.m_state.setWindowLevelDragging(false);
                m_widget.m_state.setInitialWindowCenter(0.0);
                m_widget.m_state.setInitialWindowWidth(1.0);
                m_widget.m_reportedFirstFrame = false;
                m_widget.m_firstFrameTimer.invalidate();
                m_widget.m_frameLoadTimer.invalidate();
                m_widget.hideOverlayWidget();
                if (m_widget.m_imageLabel)
                {
                        m_widget.m_imageLabel->hide();
                        m_widget.m_imageLabel->clear();
                }
                if (m_widget.m_tabWidget)
                {
                        m_widget.m_tabWidget->setAcceptDrops(true);
                }
                if (m_widget.m_scroll)
                {
                        m_widget.m_scroll->hide();
                        m_widget.m_scroll->setValue(0);
                        m_widget.m_scroll->setMaximum(0);
                }
                m_imageLoadFuture = {};
                if (m_widget.m_errorLabel)
                {
                        const auto reason = t_reason.trimmed();
                        const auto message = reason.isEmpty()
                                ? QObject::tr("Unable to render the selected image.")
                                : QObject::tr("Unable to render the selected image.\n%1").arg(reason);
                        m_widget.m_errorLabel->setText(message);
                        m_widget.m_errorLabel->setToolTip(reason);
                        m_widget.m_errorLabel->show();
                }
                m_widget.updateAvailableWindowPresets();
        }

        void Widget2DLoadController::onImagesLoaded()
        {
                if (!m_imageLoadWatcher)
                {
                        return;
                }

                PresenterPtr presenter;
                try
                {
                        presenter = m_imageLoadWatcher->result();
                }
                catch (const std::exception& ex)
                {
                        handleFailure(QString::fromUtf8(ex.what()));
                        return;
                }
                catch (...)
                {
                        handleFailure({});
                        return;
                }

                if (!presenter || !presenter->isValid())
                {
                        m_widget.stopLoadingAnimation();
                        handleFailure(QObject::tr("No frames decoded from the selected dataset."));
                        return;
                }

                m_widget.m_imagePresenter = std::move(presenter);
                const auto initialState = m_widget.m_imagePresenter->initialState();
                m_widget.m_state.resetForNewSeries(initialState);
                m_widget.updateAvailableWindowPresets();
                const QString seriesUid = m_widget.m_series
                        ? QString::fromStdString(m_widget.m_series->getUID())
                        : QStringLiteral("n/a");
                qCInfo(lcWidget2D)
                        << "[Diagnostics] 2D initial window"
                        << "seriesUid" << seriesUid
                        << "windowCenter" << initialState.WindowCenter
                        << "windowWidth" << initialState.WindowWidth
                        << "invertColors" << initialState.InvertColors;
                m_widget.m_renderingActive = true;
                m_widget.m_isImageLoaded = true;
                if (m_widget.m_frameLoadTimer.isValid())
                {
                        qCInfo(lcWidget2D)
                                << "[Telemetry] QtConcurrent future completed"
                                << "elapsedMs" << m_widget.m_frameLoadTimer.elapsed();
                        m_widget.m_frameLoadTimer.invalidate();
                }

                logPresenterStats();

                if (m_widget.m_tabWidget)
                {
                        m_widget.m_tabWidget->setAcceptDrops(true);
                }
                if (m_widget.m_errorLabel)
                {
                        m_widget.m_errorLabel->hide();
                }
                m_widget.disconnectScroll();
                if (m_widget.m_scroll)
                {
                        const int maxIndex = std::max<int>(m_widget.m_imagePresenter->frameCount() - 1, 0);
                        const int value = std::clamp(m_widget.m_imageIndex, 0, maxIndex);
                        m_widget.setSliderValues(0, maxIndex, value);
                        m_widget.m_scroll->setVisible(maxIndex > 0);
                        Q_UNUSED(QObject::connect(m_widget.m_scroll, &QScrollBar::valueChanged,
                                &m_widget, &Widget2D::onChangeImage));
                }
                const int targetIndex = (m_widget.m_scroll) ? m_widget.m_scroll->value() : 0;
                if (m_widget.m_overlayUpdater)
                {
                        m_widget.m_overlayUpdater->updateMetadata(m_widget.m_series, m_widget.m_imageLabel);
                }
                m_widget.m_manualZoomFactor = 1.0;
                m_widget.m_state.setManualZoomFactor(1.0);
                m_widget.m_cachedFrame = {};
                m_widget.m_displayZoomFactor = 1.0;
                m_widget.m_state.setDisplayZoomFactor(1.0);
                m_widget.setFitToWindowEnabled(true);
                m_widget.m_windowLevelDragging = false;
                m_widget.m_state.setWindowLevelDragging(false);
                m_widget.applyLoadedFrame(targetIndex);
                m_widget.refreshDisplayedFrame(true);
                // Allow the freshly rendered frame to remain visible while prefetch continues.
                m_widget.stopLoadingAnimation();
                schedulePresenterPrefetch();
                m_imageLoadFuture = {};
                qCInfo(lcWidget2D)
                        << "GDCM rendering completed."
                        << "frames:" << m_widget.m_imagePresenter->frameCount();
        }

        void Widget2DLoadController::onFramePrefetchFinished()
        {
                m_widget.stopLoadingAnimation();
                m_framePrefetchFuture = {};
                logPrefetchStats();
        }

        void Widget2DLoadController::waitForPendingTasks() const
        {
                if (m_imageLoadWatcher)
                {
                        m_imageLoadWatcher->waitForFinished();
                }

                if (m_framePrefetchFuture.isRunning())
                {
                        const_cast<QFuture<void>&>(m_framePrefetchFuture).waitForFinished();
                }

                if (!m_appendFuture.isRunning())
                {
                        return;
                }

                if (QThread::currentThread() != thread())
                {
                        const_cast<QFuture<int>&>(m_appendFuture).waitForFinished();
                        return;
                }

                if (!m_appendWatcher)
                {
                        return;
                }

                QEventLoop loop;
                const QMetaObject::Connection connection = QObject::connect(
                        m_appendWatcher.get(),
                        &QFutureWatcher<int>::finished,
                        &loop,
                        &QEventLoop::quit);

                if (!m_appendFuture.isFinished())
                {
                        loop.exec();
                }

                QObject::disconnect(connection);
        }

        void Widget2DLoadController::requestAppendFrames(core::Series* t_series)
        {
                if (!t_series || !m_widget.m_imagePresenter || !m_widget.m_imagePresenter->isValid())
                {
                        return;
                }

                if (m_appendFuture.isRunning())
                {
                        qCDebug(lcWidget2D)
                                << "Append request ignored because another append is still running.";
                        return;
                }

                ensureAppendWatcher();

                auto presenterRef = m_widget.m_imagePresenter;
                m_pendingAppendSeries = t_series;
                m_appendFuture = QtConcurrent::run([presenterRef, t_series]() -> int
                {
                        if (!presenterRef)
                        {
                                return 0;
                        }

                        try
                        {
                                return presenterRef->appendSingleFrameImages(t_series);
                        }
                        catch (const std::exception& ex)
                        {
                                qCWarning(lcWidget2D)
                                        << "appendSingleFrameImages() failed:" << ex.what();
                        }
                        catch (...)
                        {
                                qCWarning(lcWidget2D)
                                        << "appendSingleFrameImages() failed due to an unknown error.";
                        }
                        return 0;
                });
                m_appendWatcher->setFuture(m_appendFuture);
        }

        bool Widget2DLoadController::isAppendRunning() const
        {
                return m_appendFuture.isRunning();
        }

        void Widget2DLoadController::cancelPrefetchIfRunning()
        {
                if (m_framePrefetchFuture.isRunning())
                {
                        m_framePrefetchFuture.cancel();
                        m_framePrefetchFuture.waitForFinished();
                }
                m_framePrefetchFuture = {};
                if (m_framePrefetchWatcher)
                {
                        m_framePrefetchWatcher.reset();
                }
        }

        void Widget2DLoadController::ensureImageLoadWatcher()
        {
                if (!m_imageLoadWatcher)
                {
                        m_imageLoadWatcher = std::make_unique<QFutureWatcher<PresenterPtr>>(this);
                        Q_UNUSED(QObject::connect(m_imageLoadWatcher.get(),
                                &QFutureWatcher<PresenterPtr>::finished,
                                &m_widget,
                                [this]()
                                {
                                        onImagesLoaded();
                                }));
                }

                m_imageLoadFuture = QtConcurrent::run(&Widget2DImagePresenter::load,
                        m_widget.m_series,
                        m_widget.m_image);
                m_imageLoadWatcher->setFuture(m_imageLoadFuture);
        }

        void Widget2DLoadController::ensureFramePrefetchWatcher()
        {
                if (!m_framePrefetchWatcher)
                {
                        m_framePrefetchWatcher = std::make_unique<QFutureWatcher<void>>(this);
                        Q_UNUSED(QObject::connect(m_framePrefetchWatcher.get(),
                                &QFutureWatcher<void>::finished,
                                &m_widget,
                                [this]()
                                {
                                        onFramePrefetchFinished();
                                }));
                }
        }

        void Widget2DLoadController::ensureAppendWatcher()
        {
                if (!m_appendWatcher)
                {
                        m_appendWatcher = std::make_unique<QFutureWatcher<int>>(this);
                        Q_UNUSED(QObject::connect(m_appendWatcher.get(),
                                &QFutureWatcher<int>::finished,
                                this,
                                &Widget2DLoadController::onAppendFramesFinished));
                }
        }

        void Widget2DLoadController::resetWidgetStateForLoad()
        {
                m_widget.m_isImageLoaded = false;
                m_widget.m_renderingActive = false;
                m_widget.m_imagePresenter.reset();
                m_widget.m_state.setPresentation({});
                m_widget.m_currentFrameIndex = 0;
                m_widget.m_cachedFrame = {};
                m_widget.m_displayZoomFactor = 1.0;
                m_widget.m_manualZoomFactor = 1.0;
                m_widget.m_fitToWindowEnabled = false;
                m_widget.m_windowLevelDragging = false;
                m_widget.m_scrollDragging = false;
                m_widget.m_zoomDragging = false;
                m_widget.m_panDragging = false;
                m_widget.m_scrollDragAccumulator = 0.0;
                m_widget.resetPanOffset();
                m_widget.m_reportedFirstFrame = false;
                m_widget.m_firstFrameTimer.restart();
                m_widget.m_frameLoadTimer.restart();
                m_widget.m_state.resetInteractiveState();
                m_widget.m_state.setInitialWindowCenter(0.0);
                m_widget.m_state.setInitialWindowWidth(1.0);
                m_widget.m_pendingScrollRefresh = false;
                m_widget.m_pendingSeriesRefresh = nullptr;
                m_widget.m_pendingImageRefresh = nullptr;
                m_widget.updateAvailableWindowPresets();
        }

        void Widget2DLoadController::schedulePresenterPrefetch()
        {
                if (!m_widget.m_imagePresenter || m_widget.m_imagePresenter->frameCount() <= 1)
                {
                        m_widget.stopLoadingAnimation();
                        return;
                }

                ensureFramePrefetchWatcher();

                auto presenterRef = m_widget.m_imagePresenter;
                m_framePrefetchFuture = QtConcurrent::run([presenterRef]()
                {
                        try
                        {
                                presenterRef->prefetchAllFrames();
                        }
                        catch (const std::exception& ex)
                        {
                                qCWarning(lcWidget2D)
                                        << "Background prefetch failed:" << ex.what();
                        }
                        catch (...)
                        {
                                qCWarning(lcWidget2D)
                                        << "Background prefetch failed due to an unknown error.";
                        }
                });
                m_framePrefetchWatcher->setFuture(m_framePrefetchFuture);
        }

        void Widget2DLoadController::onAppendFramesFinished()
        {
                if (!m_appendWatcher)
                {
                        return;
                }

                int appended = 0;
                try
                {
                        appended = m_appendWatcher->result();
                }
                catch (const std::exception& ex)
                {
                        qCWarning(lcWidget2D)
                                << "appendSingleFrameImages() future failed:" << ex.what();
                }
                catch (...)
                {
                        qCWarning(lcWidget2D)
                                << "appendSingleFrameImages() future failed due to an unknown error.";
                }

                m_appendFuture = {};
                emit framesReady(m_pendingAppendSeries, appended);
                m_pendingAppendSeries = nullptr;
        }

        void Widget2DLoadController::logPresenterStats() const
        {
                if (!m_widget.m_imagePresenter)
                {
                        return;
                }

                qCInfo(lcWidget2D)
                        << "[Telemetry] GDCM presenter stats"
                        << "frames" << m_widget.m_imagePresenter->frameCount()
                        << "allocatedBytes" << static_cast<unsigned long long>(
                                m_widget.m_imagePresenter->totalAllocatedFrameBytes())
                        << "decodingMs" << m_widget.m_imagePresenter->decodingDurationMs();
        }

        void Widget2DLoadController::logPrefetchStats() const
        {
                if (!m_widget.m_imagePresenter)
                {
                        return;
                }

                qCInfo(lcWidget2D)
                        << "[Telemetry] GDCM prefetch completed"
                        << "frames" << m_widget.m_imagePresenter->frameCount()
                        << "allocatedBytes" << static_cast<unsigned long long>(
                                m_widget.m_imagePresenter->totalAllocatedFrameBytes())
                        << "decodingMs" << m_widget.m_imagePresenter->decodingDurationMs();
        }
}
