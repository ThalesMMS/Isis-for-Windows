/*
 * ------------------------------------------------------------------------------------
 *  File: widget2drenderer.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
*      Implements render window configuration for 2D widgets, delegating overlay updates to a dedicated helper.
 *
 *  License:
 *      This file is part of a derived work based on the Asclepios DICOM Viewer,
 *      licensed under the MIT License.
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a copy
 *      of this software and associated documentation files (the "Software"), to deal
 *      in the Software without restriction, including without limitation the rights
 *      to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *      copies of the Software, and to permit persons to whom the Software is
 *      furnished to do so, subject to the following conditions:
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

#include "widget2drenderer.h"

#include <QImage>
#include <QLabel>
#include <QLoggingCategory>
#include <QPainter>
#include <QPixmap>
#include <algorithm>
#include <cmath>
#include <limits>

#include "widget2d.h"
#include "widget2dimagepresenter.h"
#include "widget2doverlayupdater.h"
#include "widget2dstate.h"

Q_DECLARE_LOGGING_CATEGORY(lcWidget2D)

namespace isis::gui
{
        Widget2DRenderer::Widget2DRenderer(Widget2D& t_widget, Widget2DState& t_state)
                : QObject(&t_widget)
                , m_widget(t_widget)
                , m_state(t_state)
        {
        }

        void Widget2DRenderer::refreshDisplayedFrame(bool t_updateOverlay)
        {
                if (!m_widget.m_imageLabel || m_widget.m_cachedFrame.isNull())
                {
                        return;
                }

                const QSize frameSize = m_widget.m_cachedFrame.size();
                if (frameSize.isEmpty())
                {
                        m_widget.m_imageLabel->clear();
                        return;
                }

                const double clampedZoom = std::clamp(m_widget.m_manualZoomFactor, 0.1, 8.0);
                m_widget.m_manualZoomFactor = clampedZoom;
                m_state.setManualZoomFactor(clampedZoom);

                double effectiveZoom = clampedZoom;
                if (m_widget.m_fitToWindowEnabled)
                {
                        const QSize labelSize = m_widget.m_imageLabel->size();
                        if (labelSize.width() > 0 && labelSize.height() > 0)
                        {
                                const double widthScale = static_cast<double>(labelSize.width())
                                        / static_cast<double>(frameSize.width());
                                const double heightScale = static_cast<double>(labelSize.height())
                                        / static_cast<double>(frameSize.height());
                                const double fitScale = std::min(widthScale, heightScale);
                                if (fitScale > 0.0 && std::isfinite(fitScale))
                                {
                                        effectiveZoom = fitScale;
                                }
                        }
                }

                m_widget.m_displayZoomFactor = effectiveZoom;
                m_state.setDisplayZoomFactor(effectiveZoom);

                QPixmap pixmap = QPixmap::fromImage(m_widget.m_cachedFrame);
                const int targetWidth = std::max<int>(
                        1,
                        static_cast<int>(std::lround(static_cast<double>(frameSize.width()) * effectiveZoom)));
                const int targetHeight = std::max<int>(
                        1,
                        static_cast<int>(std::lround(static_cast<double>(frameSize.height()) * effectiveZoom)));
                const QSize targetSize(targetWidth, targetHeight);
                if (targetSize != pixmap.size())
                {
                        pixmap = pixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                }

                if (m_widget.m_imageLabel)
                {
                        if (!m_widget.m_fitToWindowEnabled)
                        {
                                const QSize labelSize = m_widget.m_imageLabel->size();
                                if (labelSize.isValid())
                                {
                                        m_widget.clampPanOffset(labelSize, pixmap.size());
                                        m_state.setPanOffset(m_widget.m_panOffset);

                                        QPixmap displayPixmap(labelSize);
                                        displayPixmap.fill(Qt::black);
                                        QPainter painter(&displayPixmap);
                                        const QPointF baseOffset(
                                                static_cast<double>(labelSize.width() - pixmap.width()) / 2.0,
                                                static_cast<double>(labelSize.height() - pixmap.height()) / 2.0);
                                        const QPointF topLeft = baseOffset + m_widget.m_panOffset;
                                        painter.drawPixmap(topLeft, pixmap);
                                        painter.end();
                                        m_widget.m_imageLabel->setPixmap(displayPixmap);
                                }
                                else
                                {
                                        m_widget.m_imageLabel->setPixmap(pixmap);
                                }
                        }
                        else
                        {
                                if (!m_widget.m_panOffset.isNull())
                                {
                                        m_widget.resetPanOffset();
                                        m_state.resetPanOffset();
                                }
                                m_widget.m_imageLabel->setPixmap(pixmap);
                        }
                }

                if (!m_widget.m_imageLabel->isVisible())
                {
                        m_widget.m_imageLabel->show();
                }

                if (t_updateOverlay)
                {
                        updateOverlay(m_widget.m_cachedFrame, m_widget.m_currentFrameIndex);
                }
        }

        void Widget2DRenderer::applyLoadedFrame(int t_index)
        {
                if (!m_widget.m_renderingActive
                        || !m_widget.m_imagePresenter
                        || !m_widget.m_imagePresenter->isValid()
                        || !m_widget.m_imageLabel)
                {
                        return;
                }

                const int maxIndex = m_widget.m_imagePresenter->frameCount() - 1;
                if (maxIndex < 0)
                {
                        return;
                }

                const int clampedIndex = std::clamp(t_index, 0, maxIndex);
                m_widget.m_currentFrameIndex = clampedIndex;
                const Widget2DState& constState = m_state;
                const auto& presentation = constState.presentation();
                const QImage frameImage = m_widget.m_imagePresenter->renderFrame(clampedIndex, presentation);
                if (frameImage.isNull())
                {
                        m_widget.m_cachedFrame = {};
                        m_widget.clearCursorInfo();
                        m_widget.m_displayZoomFactor = 1.0;
                        m_state.setDisplayZoomFactor(1.0);
                        if (m_widget.m_imageLabel)
                        {
                                m_widget.m_imageLabel->clear();
                        }
                        hideOverlay();
                        return;
                }

                m_widget.m_cachedFrame = frameImage;
                m_widget.updateOrientationLabels(clampedIndex);
                refreshDisplayedFrame(false);
                updateOverlay(frameImage, clampedIndex);
                if (!m_widget.m_reportedFirstFrame && m_widget.m_firstFrameTimer.isValid())
                {
                        const qint64 elapsedToFirstFrame = m_widget.m_firstFrameTimer.elapsed();
                        const QString seriesUid = (m_widget.m_series)
                                ? QString::fromStdString(m_widget.m_series->getUID())
                                : QStringLiteral("n/a");
                        qCInfo(lcWidget2D)
                                << "[Telemetry] Time to first frame"
                                << "elapsedMs" << elapsedToFirstFrame
                                << "seriesUid" << seriesUid
                                << "frameIndex" << clampedIndex;
                        m_widget.m_reportedFirstFrame = true;
                }
        }

        void Widget2DRenderer::hideOverlay()
        {
                if (m_widget.m_overlayUpdater)
                {
                        m_widget.m_overlayUpdater->hideOverlay();
                }
                m_widget.m_windowLevelDragging = false;
                m_state.setWindowLevelDragging(false);
        }

        void Widget2DRenderer::updateOverlay(const QImage& t_frameImage, int t_frameIndex)
        {
                Q_UNUSED(t_frameImage);
                if (!m_widget.m_renderingActive || !m_widget.m_overlayUpdater)
                {
                        return;
                }
                if (!m_widget.m_imageLabel)
                {
                        return;
                }

                const QString seriesNumber = (m_widget.m_series)
                        ? QString::fromStdString(m_widget.m_series->getNumber())
                        : QString();
                const int frameCount = (m_widget.m_imagePresenter)
                        ? std::max<int>(m_widget.m_imagePresenter->frameCount(), 0)
                        : 0;

                const Widget2DState& constState = m_state;
                const auto& presentation = constState.presentation();
                double windowCenter = presentation.WindowCenter;
                double windowWidth = presentation.WindowWidth;
                if (m_widget.m_imagePresenter)
                {
                        const auto initialState = m_widget.m_imagePresenter->initialState();
                        if (windowWidth <= 0.0)
                        {
                                windowWidth = initialState.WindowWidth;
                                windowCenter = initialState.WindowCenter;
                        }
                }

                const double zoomFactor = (m_widget.m_displayZoomFactor > 0.0 && std::isfinite(m_widget.m_displayZoomFactor))
                        ? m_widget.m_displayZoomFactor
                        : m_widget.m_manualZoomFactor;

                double spacingX = 0.0;
                double spacingY = 0.0;
                double sliceLocation = std::numeric_limits<double>::quiet_NaN();

                if (auto* const currentImage = m_widget.imageForFrameIndex(t_frameIndex))
                {
                        spacingX = currentImage->getPixelSpacingX();
                        spacingY = currentImage->getPixelSpacingY();
                        sliceLocation = currentImage->getSliceLocation();
                }
                else if (m_widget.m_image)
                {
                        spacingX = m_widget.m_image->getPixelSpacingX();
                        spacingY = m_widget.m_image->getPixelSpacingY();
                        sliceLocation = m_widget.m_image->getSliceLocation();
                }

                if (spacingX < 0.0)
                {
                        spacingX = 0.0;
                }
                if (spacingY < 0.0)
                {
                        spacingY = 0.0;
                }

                m_widget.m_overlayUpdater->updateFrameOverlay(seriesNumber,
                        t_frameIndex,
                        frameCount,
                        windowCenter,
                        windowWidth,
                        spacingX,
                        spacingY,
                        sliceLocation,
                        zoomFactor,
                        m_widget.m_activeTool,
                        m_widget.m_imageLabel,
                        m_widget.m_renderingActive);
                m_widget.publishFrameMetrics(t_frameIndex, frameCount, windowCenter, windowWidth, zoomFactor);
        }
}
