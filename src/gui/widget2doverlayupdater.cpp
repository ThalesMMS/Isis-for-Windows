/*
 * ------------------------------------------------------------------------------------
 *  File: widget2doverlayupdater.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements the helper that drives overlay metadata and annotations for the 2D widget.
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

#include "widget2doverlayupdater.h"

#include <QLabel>
#include <QObject>
#include <QtGlobal>
#include <algorithm>
#include <cmath>

#include "dcmtkoverlaywidget.h"
#include "widget2d.h"
#include "series.h"

namespace isis::gui
{
        void Widget2DOverlayUpdater::ensureOverlayWidget(QLabel* t_parentLabel)
        {
                if (!t_parentLabel)
                {
                        return;
                }
                if (!m_overlayWidget)
                {
                        m_overlayWidget = new DcmtkOverlayWidget(t_parentLabel);
                        m_overlayWidget->hide();
                }
                else if (m_overlayWidget->parentWidget() != t_parentLabel)
                {
                        m_overlayWidget->setParent(t_parentLabel);
                }
        }

        void Widget2DOverlayUpdater::hideOverlay()
        {
                if (!m_overlayWidget)
                {
                        return;
                }
                m_overlayWidget->clear();
                m_overlayWidget->hide();
        }

        void Widget2DOverlayUpdater::updateToolOverlay(InteractionTool t_tool,
                bool t_renderingActive,
                QLabel* t_parentLabel)
        {
                if (!t_renderingActive)
                {
                        return;
                }
                ensureOverlayWidget(t_parentLabel);
                if (!m_overlayWidget)
                {
                        return;
                }
                m_overlayWidget->setToolName(toolDisplayName(t_tool));
                m_overlayWidget->update();
        }

        void Widget2DOverlayUpdater::updateMetadata(isis::core::Series* t_series, QLabel* t_parentLabel)
        {
                if (!t_series)
                {
                        return;
                }
                ensureOverlayWidget(t_parentLabel);
                if (!m_overlayWidget)
                {
                        return;
                }
                const auto* metadata = t_series->getMetadataForSeries();
                m_overlayWidget->setMetadata(metadata);
        }

        void Widget2DOverlayUpdater::updateFrameOverlay(const QString& t_seriesNumber,
                int t_frameIndex,
                int t_frameCount,
                double t_windowCenter,
                double t_windowWidth,
                double t_spacingX,
                double t_spacingY,
                double t_sliceLocation,
                double t_zoomFactor,
                InteractionTool t_tool,
                QLabel* t_parentLabel,
                bool t_renderingActive)
        {
                if (!t_renderingActive)
                {
                        hideOverlay();
                        return;
                }

                ensureOverlayWidget(t_parentLabel);
                if (!m_overlayWidget || !t_parentLabel)
                {
                        return;
                }

                const double clampedZoom = (t_zoomFactor > 0.0 && std::isfinite(t_zoomFactor))
                        ? t_zoomFactor
                        : 1.0;

                const double sanitizedWindowWidth = std::max<double>(t_windowWidth, 1.0);

                m_overlayWidget->setSeriesInformation(t_seriesNumber, t_frameIndex, std::max<int>(t_frameCount, 0));
                m_overlayWidget->setWindowLevel(t_windowCenter, sanitizedWindowWidth);
                m_overlayWidget->setSpacing(t_spacingX, t_spacingY);
                m_overlayWidget->setSliceLocation(t_sliceLocation);
                m_overlayWidget->setZoom(clampedZoom);
                m_overlayWidget->setToolName(toolDisplayName(t_tool));
                m_overlayWidget->setGeometry(t_parentLabel->rect());
                m_overlayWidget->raise();
                m_overlayWidget->show();
                m_overlayWidget->update();
        }

        QString Widget2DOverlayUpdater::toolDisplayName(InteractionTool t_tool) const
        {
                switch (t_tool)
                {
                case InteractionTool::scroll:
                        return QObject::tr("Scroll");
                case InteractionTool::window:
                        return QObject::tr("Window");
                case InteractionTool::zoom:
                        return QObject::tr("Zoom");
                case InteractionTool::pan:
                        return QObject::tr("Pan");
                default:
                        break;
                }
                return QObject::tr("Unknown");
        }
}

