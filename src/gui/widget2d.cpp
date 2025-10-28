/*
 * ------------------------------------------------------------------------------------
 *  File: widget2d.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements the Qt-based 2D viewer logic, coordinating image loading, overlay, interaction, and rendering helpers.
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

#include "widget2d.h"
#include "widget2dinteractor.h"
#include "widget2dimagepresenter.h"
#include "widget2dloadcontroller.h"
#include "widget2doverlayupdater.h"
#include "widget2drenderer.h"
#include <QFile>
#include <QFocusEvent>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QMetaType>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QTransform>
#include <QString>
#include <QKeyEvent>
#include <QCursor>
#include <QWheelEvent>
#include <QSet>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include "patient.h"
#include "series.h"
#include "study.h"
#include "tabwidget.h"

Q_DECLARE_LOGGING_CATEGORY(lcLoadingAnimation)
Q_LOGGING_CATEGORY(lcWidget2D, "isis.gui.widget2d")
Q_DECLARE_METATYPE(isis::gui::Widget2D::FrameMetrics)
Q_DECLARE_METATYPE(isis::gui::Widget2D::CursorInfo)
Q_DECLARE_METATYPE(isis::gui::Widget2D::WindowPreset)


bool isis::gui::Widget2D::startVolumeRendering()
{
        return (m_loadController) ? m_loadController->startRendering() : false;
}

void isis::gui::Widget2D::ensureImageLabel()
{
        if (!m_imageLabel)
        {
                m_imageLabel = new QLabel(this);
                m_imageLabel->setAlignment(Qt::AlignCenter);
                m_imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                m_imageLabel->setMinimumSize(QSize(1, 1));
                m_imageLabel->setStyleSheet(QStringLiteral("background-color: #000000; border: none;"));
                m_imageLabel->setMouseTracking(true);
                m_imageLabel->hide();
        }
        if (m_imageLabel)
        {
                m_imageLabel->removeEventFilter(this);
                m_imageLabel->installEventFilter(this);
        }
        ensureOverlayWidget();
        ensureOrientationLabels();
        updateOrientationLabels(m_currentFrameIndex);
        positionOrientationLabels();
        setCursorForActiveTool();
}

void isis::gui::Widget2D::ensureOverlayWidget()
{
        if (!m_overlayUpdater)
        {
                return;
        }
        m_overlayUpdater->ensureOverlayWidget(m_imageLabel);
}

void isis::gui::Widget2D::ensureOrientationLabels()
{
        if (!m_imageLabel)
        {
                return;
        }

        for (int i = 0; i < 4; ++i)
        {
                if (!m_orientationLabels[i])
                {
                        auto* label = new QLabel(m_imageLabel);
                        label->setObjectName(QStringLiteral("orientationLabel%1").arg(i));
                        label->setAttribute(Qt::WA_TransparentForMouseEvents, true);
                        label->setStyleSheet(QStringLiteral("color: #f8d560; font-weight: 600; font-size: 12pt; background: transparent;"));
                        label->setAlignment(Qt::AlignCenter);
                        auto* shadow = new QGraphicsDropShadowEffect(label);
                        shadow->setBlurRadius(8.0);
                        shadow->setColor(QColor(0, 0, 0, 180));
                        shadow->setOffset(0.0);
                        label->setGraphicsEffect(shadow);
                        label->hide();
                        m_orientationLabels[i] = label;
                }
        }
}

void isis::gui::Widget2D::positionOrientationLabels() const
{
        if (!m_imageLabel)
        {
                return;
        }

        const QRect rect = m_imageLabel->rect();
        if (rect.isEmpty())
        {
                return;
        }

        const int margin = 24;
        const QPoint center = rect.center();

        auto placeLabel = [&](QLabel* label, const QPoint& point)
        {
                if (!label)
                {
                        return;
                }
                label->adjustSize();
                const QSize size = label->size();
                const int x = point.x() - size.width() / 2;
                const int y = point.y() - size.height() / 2;
                label->move(std::clamp(x, margin - size.width() / 2, rect.width() - size.width() - margin + size.width() / 2),
                        std::clamp(y, margin - size.height() / 2, rect.height() - size.height() - margin + size.height() / 2));
                if (m_imageLabel->isVisible())
                {
                        label->show();
                }
        };

        placeLabel(m_orientationLabels[0], QPoint(center.x(), margin + m_orientationLabels[0]->height() / 2));
        placeLabel(m_orientationLabels[1], QPoint(center.x(), rect.height() - margin - m_orientationLabels[1]->height() / 2));
        placeLabel(m_orientationLabels[2], QPoint(margin + m_orientationLabels[2]->width() / 2, center.y()));
        placeLabel(m_orientationLabels[3], QPoint(rect.width() - margin - m_orientationLabels[3]->width() / 2, center.y()));
}

void isis::gui::Widget2D::updateOrientationLabels(const int frameIndex)
{
        ensureOrientationLabels();

        const core::Image* image = orientationSourceImage(frameIndex);
        QString labels[4] = {
                QStringLiteral("A"),
                QStringLiteral("P"),
                QStringLiteral("L"),
                QStringLiteral("R")};

        const auto isFiniteVector = [](const std::array<double, 3>& values)
        {
                for (double value : values)
                {
                        if (!std::isfinite(value))
                        {
                                return false;
                        }
                }
                return true;
        };

        if (image && image->hasImageOrientationPatient()
                && isFiniteVector(image->getImageOrientationRow())
                && isFiniteVector(image->getImageOrientationColumn()))
        {
                const auto& row = image->getImageOrientationRow();
                const auto& column = image->getImageOrientationColumn();

                const std::array<double, 3> topDir = {-column[0], -column[1], -column[2]};
                const std::array<double, 3> bottomDir = {column[0], column[1], column[2]};
                const std::array<double, 3> leftDir = {-row[0], -row[1], -row[2]};
                const std::array<double, 3> rightDir = {row[0], row[1], row[2]};

                const QString topLabel = orientationLabelForVector(topDir);
                const QString bottomLabel = orientationLabelForVector(bottomDir);
                const QString leftLabel = orientationLabelForVector(leftDir);
                const QString rightLabel = orientationLabelForVector(rightDir);

                if (!topLabel.isEmpty())
                {
                        labels[0] = topLabel;
                }
                if (!bottomLabel.isEmpty())
                {
                        labels[1] = bottomLabel;
                }
                if (!leftLabel.isEmpty())
                {
                        labels[2] = leftLabel;
                }
                if (!rightLabel.isEmpty())
                {
                        labels[3] = rightLabel;
                }
        }

        for (int i = 0; i < 4; ++i)
        {
                if (!m_orientationLabels[i])
                {
                        continue;
                }
                m_orientationLabels[i]->setText(labels[i]);
                if (m_imageLabel && m_imageLabel->isVisible())
                {
                        m_orientationLabels[i]->show();
                }
        }

        positionOrientationLabels();
}

QString isis::gui::Widget2D::orientationLabelForVector(const std::array<double, 3>& direction) const
{
        std::array<double, 3> normalized = direction;
        const double magnitude = std::sqrt(normalized[0] * normalized[0]
                + normalized[1] * normalized[1]
                + normalized[2] * normalized[2]);
        if (!(magnitude > 1e-6))
        {
                return QString();
        }

        normalized[0] /= magnitude;
        normalized[1] /= magnitude;
        normalized[2] /= magnitude;

        struct AxisContribution
        {
                double value = 0.0;
                QChar positive = QLatin1Char('L');
                QChar negative = QLatin1Char('R');
        };

        AxisContribution axes[3] = {
                {normalized[0], QLatin1Char('L'), QLatin1Char('R')},
                {normalized[1], QLatin1Char('P'), QLatin1Char('A')},
                {normalized[2], QLatin1Char('H'), QLatin1Char('F')}};

        std::sort(std::begin(axes), std::end(axes), [](const AxisContribution& lhs, const AxisContribution& rhs)
                {
                        return std::abs(lhs.value) > std::abs(rhs.value);
                });

        QString label;
        constexpr double contributionThreshold = 0.2;
        for (const AxisContribution& axis : axes)
        {
                const double magnitudeComponent = std::abs(axis.value);
                if (magnitudeComponent < contributionThreshold)
                {
                        continue;
                }
                label.append(axis.value >= 0.0 ? axis.positive : axis.negative);
                if (label.size() >= 2)
                {
                        break;
                }
        }

        if (label.isEmpty())
        {
                const AxisContribution& dominant = axes[0];
                label.append(dominant.value >= 0.0 ? dominant.positive : dominant.negative);
        }

        return label;
}

const isis::core::Image* isis::gui::Widget2D::orientationSourceImage(const int frameIndex) const
{
        if (auto* const frameImage = imageForFrameIndex(frameIndex))
        {
                return frameImage;
        }

        if (m_image)
        {
                return m_image;
        }

        if (m_series)
        {
                const auto& singleFrameImages = m_series->getSingleFrameImages();
                if (!singleFrameImages.empty())
                {
                        return singleFrameImages.begin()->get();
                }
        }

        return nullptr;
}

std::optional<QPoint> isis::gui::Widget2D::mapLabelPointToImagePixel(const QPoint& labelPos) const
{
        if (!m_imageLabel || m_cachedFrame.isNull())
        {
                return std::nullopt;
        }

        const QSize labelSize = m_imageLabel->size();
        const QSize frameSize = m_cachedFrame.size();
        if (!labelSize.isValid() || !frameSize.isValid())
        {
                return std::nullopt;
        }

        const double zoom = (m_displayZoomFactor > 0.0 && std::isfinite(m_displayZoomFactor))
                ? m_displayZoomFactor
                : m_manualZoomFactor;
        if (!(zoom > 0.0))
        {
                return std::nullopt;
        }

        const QSizeF scaledSize(static_cast<double>(frameSize.width()) * zoom,
                static_cast<double>(frameSize.height()) * zoom);
        QPointF baseOffset(
                (static_cast<double>(labelSize.width()) - scaledSize.width()) / 2.0,
                (static_cast<double>(labelSize.height()) - scaledSize.height()) / 2.0);
        const QPointF topLeft = baseOffset + m_panOffset;
        const QPointF relative = QPointF(labelPos) - topLeft;

        if (relative.x() < 0.0 || relative.y() < 0.0
                || relative.x() >= scaledSize.width()
                || relative.y() >= scaledSize.height())
        {
                return std::nullopt;
        }

        const double invZoom = 1.0 / zoom;
        const int pixelX = static_cast<int>(std::floor(relative.x() * invZoom));
        const int pixelY = static_cast<int>(std::floor(relative.y() * invZoom));

        if (pixelX < 0 || pixelY < 0 || pixelX >= frameSize.width() || pixelY >= frameSize.height())
        {
                return std::nullopt;
        }

        return QPoint(pixelX, pixelY);
}

void isis::gui::Widget2D::handleCursorHover(const QPoint& labelPos)
{
        if (!m_renderingActive || !m_imagePresenter || !m_imagePresenter->isValid())
        {
                clearCursorInfo();
                return;
        }

        if (!m_imageLabel || !m_imageLabel->rect().contains(labelPos))
        {
                clearCursorInfo();
                return;
        }

        const std::optional<QPoint> pixelPos = mapLabelPointToImagePixel(labelPos);
        if (!pixelPos)
        {
                clearCursorInfo();
                return;
        }

        CursorInfo info;
        info.valid = true;
        info.pixel = *pixelPos;

        if (const core::Image* image = orientationSourceImage(m_currentFrameIndex))
        {
                const double spacingX = image->getPixelSpacingX();
                const double spacingY = image->getPixelSpacingY();
                if (std::isfinite(spacingX) && std::isfinite(spacingY)
                        && spacingX > 0.0 && spacingY > 0.0)
                {
                        info.millimeters = QPointF(
                                static_cast<double>(info.pixel.x()) * spacingX,
                                static_cast<double>(info.pixel.y()) * spacingY);
                }
        }

        double storedValue = 0.0;
        double huValue = 0.0;
        if (m_imagePresenter->sampleValue(m_currentFrameIndex,
                info.pixel.x(),
                info.pixel.y(),
                storedValue,
                huValue))
        {
                info.hasIntensity = true;
                info.rawValue = storedValue;
                info.huValue = huValue;
        }
        else
        {
                info.hasIntensity = false;
                info.rawValue = std::numeric_limits<double>::quiet_NaN();
                info.huValue = std::numeric_limits<double>::quiet_NaN();
        }

        const auto nearlyEqual = [](double lhs, double rhs)
        {
                return std::abs(lhs - rhs) <= 0.1;
        };

        if (m_hasCursorInfo
                && info.valid == m_lastCursorInfo.valid
                && info.hasIntensity == m_lastCursorInfo.hasIntensity
                && info.pixel == m_lastCursorInfo.pixel
                && (info.millimeters - m_lastCursorInfo.millimeters).manhattanLength() <= 0.1
                && (info.hasIntensity
                        ? (nearlyEqual(info.rawValue, m_lastCursorInfo.rawValue)
                                && nearlyEqual(info.huValue, m_lastCursorInfo.huValue))
                        : true))
        {
                return;
        }

        m_lastCursorInfo = info;
        m_hasCursorInfo = info.valid;
        emit cursorInfoChanged(info);
}

void isis::gui::Widget2D::clearCursorInfo()
{
        if (!m_hasCursorInfo)
        {
                return;
        }

        m_hasCursorInfo = false;
        m_lastCursorInfo = {};
        CursorInfo info = {};
        emit cursorInfoChanged(info);
}

void isis::gui::Widget2D::updateAvailableWindowPresets()
{
        QVector<WindowPreset> presets;
        if (m_imagePresenter && m_imagePresenter->isValid())
        {
                const auto available = m_imagePresenter->windowPresets();
                QSet<QString> uniqueness;
                uniqueness.reserve(static_cast<int>(available.size()));
                for (const auto& preset : available)
                {
                        if (!std::isfinite(preset.Width) || preset.Width <= 0.0)
                        {
                                continue;
                        }
                        if (!std::isfinite(preset.Center))
                        {
                                continue;
                        }
                        const QString key = QStringLiteral("%1|%2")
                                                     .arg(preset.Center, 0, 'g', 15)
                                                     .arg(preset.Width, 0, 'g', 15);
                        if (uniqueness.contains(key))
                        {
                                continue;
                        }
                        uniqueness.insert(key);

                        WindowPreset qtPreset;
                        qtPreset.center = preset.Center;
                        qtPreset.width = preset.Width;
                        QString explanation = QString::fromStdString(preset.Explanation).trimmed();
                        if (explanation.isEmpty())
                        {
                                explanation = tr("DICOM WL %1  WW %2")
                                        .arg(QString::number(preset.Center, 'f', 0),
                                                QString::number(preset.Width, 'f', 0));
                        }
                        else
                        {
                                explanation = tr("%1  |  WL %2  WW %3")
                                        .arg(explanation,
                                                QString::number(preset.Center, 'f', 0),
                                                QString::number(preset.Width, 'f', 0));
                        }
                        qtPreset.label = explanation;
                        presets.append(std::move(qtPreset));
                }
        }
        m_availableWindowPresets = std::move(presets);
        const auto& presentation = m_state.presentation();
        double activeCenter = presentation.WindowCenter;
        double activeWidth = presentation.WindowWidth;
        if (!std::isfinite(activeWidth) || activeWidth <= 0.0)
        {
                activeWidth = m_state.initialWindowWidth();
        }
        if (!std::isfinite(activeCenter))
        {
                activeCenter = m_state.initialWindowCenter();
        }
        emit windowPresetsChanged(m_availableWindowPresets, activeCenter, activeWidth);
}

void isis::gui::Widget2D::refreshDisplayedFrame(const bool t_updateOverlay)
{
        if (m_renderer)
        {
                m_renderer->refreshDisplayedFrame(t_updateOverlay);
        }
}

void isis::gui::Widget2D::applyLoadedFrame(const int t_index)
{
        if (m_renderer)
        {
                m_renderer->applyLoadedFrame(t_index);
        }
}

void isis::gui::Widget2D::positionLoadingAnimation()
{
        if (!m_loadingAnimation)
        {
                return;
        }

	const QRect targetRect = (m_imageLabel && !m_imageLabel->geometry().isEmpty())
		? m_imageLabel->geometry()
		: rect();
	if (targetRect.isEmpty())
	{
		return;
	}
	const QSize overlaySize = m_loadingAnimation->size();
	const QPoint desired = targetRect.center() - QPoint(overlaySize.width() / 2, overlaySize.height() / 2);
	const int minX = targetRect.left();
	const int maxX = targetRect.right() - overlaySize.width() + 1;
	const int minY = targetRect.top();
	const int maxY = targetRect.bottom() - overlaySize.height() + 1;
	const int clampedX = std::max(minX, std::min(desired.x(), maxX));
	const int clampedY = std::max(minY, std::min(desired.y(), maxY));
	m_loadingAnimation->move(clampedX, clampedY);
        qCDebug(lcLoadingAnimation) << "Widget2D overlay positioned"
                << "widget" << this
                << "targetRect" << targetRect
                << "overlaySize" << overlaySize
                << "topLeft" << QPoint(clampedX, clampedY);
}

void isis::gui::Widget2D::publishFrameMetrics(int frameIndex, int frameCount, double windowCenter,
        double windowWidth, double zoomFactor)
{
        FrameMetrics metrics;
        metrics.frameIndex = std::max(frameIndex, 0);
        metrics.frameCount = std::max(frameCount, 0);
        metrics.windowCenter = windowCenter;
        metrics.windowWidth = windowWidth;
        metrics.zoomFactor = zoomFactor;

        if (auto* const image = imageForFrameIndex(metrics.frameIndex))
        {
                metrics.pixelSpacingX = image->getPixelSpacingX();
                metrics.pixelSpacingY = image->getPixelSpacingY();
                metrics.sliceLocation = image->getSliceLocation();
        }
        else if (m_image)
        {
                metrics.pixelSpacingX = m_image->getPixelSpacingX();
                metrics.pixelSpacingY = m_image->getPixelSpacingY();
                metrics.sliceLocation = m_image->getSliceLocation();
        }

        if (metrics.pixelSpacingX < 0.0)
        {
                metrics.pixelSpacingX = 0.0;
        }
        if (metrics.pixelSpacingY < 0.0)
        {
                metrics.pixelSpacingY = 0.0;
        }

        m_lastFrameMetrics = metrics;
        m_hasLastFrameMetrics = true;
        emit frameMetricsChanged(metrics);
}

isis::core::Image* isis::gui::Widget2D::imageForFrameIndex(const int frameIndex) const
{
        if (!m_series || frameIndex < 0)
        {
                return nullptr;
        }

        const auto& singleFrameImages = m_series->getSingleFrameImages();
        const int singleFrameCount = static_cast<int>(singleFrameImages.size());
        if (singleFrameCount <= 0)
        {
                return nullptr;
        }

        try
        {
                return m_series->getSingleFrameImageByIndex(std::min(frameIndex, singleFrameCount - 1));
        }
        catch (...)
        {
                return nullptr;
        }
}

void isis::gui::Widget2D::forceFrameMetricsUpdate()
{
        if (!m_renderingActive || !m_hasLastFrameMetrics)
        {
                return;
        }

        emit frameMetricsChanged(m_lastFrameMetrics);
}

void isis::gui::Widget2D::hideOverlayWidget()
{
        if (m_renderer)
        {
                m_renderer->hideOverlay();
        }
}

void isis::gui::Widget2D::keyPressEvent(QKeyEvent* t_event)
{
        if (!t_event)
        {
                QWidget::keyPressEvent(t_event);
                return;
        }

        bool handled = false;
        if (!t_event->modifiers())
        {
                switch (t_event->key())
                {
                case Qt::Key_B:
                        setActiveTool(InteractionTool::scroll);
                        handled = true;
                        break;
                case Qt::Key_W:
                        setActiveTool(InteractionTool::window);
                        handled = true;
                        break;
                case Qt::Key_Z:
                        setActiveTool(InteractionTool::zoom);
                        handled = true;
                        break;
                case Qt::Key_M:
                        setActiveTool(InteractionTool::pan);
                        handled = true;
                        break;
                default:
                        break;
                }
        }

        if (handled)
        {
                t_event->accept();
                return;
        }
        QWidget::keyPressEvent(t_event);
}

void isis::gui::Widget2D::updateActiveToolUi()
{
        setCursorForActiveTool();
        updateToolOverlay();
}

void isis::gui::Widget2D::updateToolOverlay()
{
        if (!m_overlayUpdater)
        {
                return;
        }
        m_overlayUpdater->updateToolOverlay(m_activeTool, m_renderingActive, m_imageLabel);
}

void isis::gui::Widget2D::resetPanOffset()
{
        m_panOffset = {};
        m_state.resetPanOffset();
}

void isis::gui::Widget2D::clampPanOffset(const QSize& labelSize, const QSize& targetSize)
{
        if (labelSize.isEmpty())
        {
                return;
        }

        const QPointF baseOffset(
                static_cast<double>(labelSize.width() - targetSize.width()) / 2.0,
                static_cast<double>(labelSize.height() - targetSize.height()) / 2.0);
        QPointF desiredTopLeft = baseOffset + m_panOffset;

        if (targetSize.width() <= labelSize.width())
        {
                desiredTopLeft.setX(baseOffset.x());
        }
        else
        {
                const double minX = static_cast<double>(labelSize.width() - targetSize.width());
                const double maxX = 0.0;
                desiredTopLeft.setX(std::clamp(desiredTopLeft.x(), minX, maxX));
        }

        if (targetSize.height() <= labelSize.height())
        {
                desiredTopLeft.setY(baseOffset.y());
        }
        else
        {
                const double minY = static_cast<double>(labelSize.height() - targetSize.height());
                const double maxY = 0.0;
                desiredTopLeft.setY(std::clamp(desiredTopLeft.y(), minY, maxY));
        }        m_panOffset = desiredTopLeft - baseOffset;
        m_state.setPanOffset(m_panOffset);
}

void isis::gui::Widget2D::setCursorForActiveTool(const bool t_handClosed)
{
        Qt::CursorShape shape = Qt::ArrowCursor;
        switch (m_activeTool)
        {
        case InteractionTool::scroll:
                shape = Qt::ArrowCursor;
                break;
        case InteractionTool::window:
                shape = Qt::CrossCursor;
                break;
        case InteractionTool::zoom:
                shape = Qt::SizeVerCursor;
                break;
        case InteractionTool::pan:
                shape = t_handClosed ? Qt::ClosedHandCursor : Qt::OpenHandCursor;
                break;
        default:
                shape = Qt::ArrowCursor;
                break;
        }

        setCursor(shape);
        if (m_imageLabel)
        {
                m_imageLabel->setCursor(shape);
        }
}

void isis::gui::Widget2D::ensureProgressiveRefreshTimer()
{
        if (m_progressiveRefreshTimer)
        {
                return;
        }

        m_progressiveRefreshTimer = std::make_unique<QTimer>(this);
        m_progressiveRefreshTimer->setInterval(1000);
        m_progressiveRefreshTimer->setSingleShot(true);
        Q_UNUSED(connect(m_progressiveRefreshTimer.get(),
                &QTimer::timeout,
                this,
                &Widget2D::onProgressiveRefreshTimeout));
}

void isis::gui::Widget2D::scheduleProgressiveRefresh()
{
        if (!m_series)
        {
                return;
        }

        ensureProgressiveRefreshTimer();
        m_progressiveRefreshPending = true;
        m_restoreFrameIndex = m_currentFrameIndex;
        if (m_progressiveRefreshTimer && !m_progressiveRefreshTimer->isActive())
        {
                m_progressiveRefreshTimer->start();
        }
}

void isis::gui::Widget2D::onProgressiveRefreshTimeout()
{
        if (!m_series || !m_loadController)
        {
                m_progressiveRefreshPending = false;
                if (m_progressiveRefreshTimer && m_progressiveRefreshTimer->isActive())
                {
                        m_progressiveRefreshTimer->stop();
                }
                return;
        }

        if (!m_progressiveRefreshPending)
        {
                if (m_progressiveRefreshTimer && m_progressiveRefreshTimer->isActive() && !m_loadController->isAppendRunning())
                {
                        m_progressiveRefreshTimer->stop();
                }
                return;
        }

        if (m_loadController->isAppendRunning())
        {
                return;
        }

        m_loadController->requestAppendFrames(m_series);
        m_progressiveRefreshPending = false;
        if (m_progressiveRefreshTimer && !m_loadController->isAppendRunning())
        {
                m_progressiveRefreshTimer->stop();
        }
}

void isis::gui::Widget2D::adjustFrameByStep(const int t_step)
{
        if (!m_renderingActive || !m_imagePresenter || t_step == 0)
        {
                return;
        }
        const int frameCount = std::max<int>(m_imagePresenter->frameCount(), 0);
        if (frameCount <= 0)
        {
                return;
        }
        const int targetIndex = std::clamp(m_currentFrameIndex + t_step, 0, frameCount - 1);
        if (targetIndex == m_currentFrameIndex)
        {
                return;
        }
        if (m_scroll)
        {
                const QSignalBlocker blocker(m_scroll);
                m_scroll->setValue(targetIndex);
        }
        applyLoadedFrame(targetIndex);
}

void isis::gui::Widget2D::applyWindowPreset(const double center, const double width)
{
        if (!m_imagePresenter)
        {
                return;
        }

        if (width <= 0.0)
        {
                resetWindowLevel();
                return;
        }

        auto& presentation = m_state.presentation();
        presentation.WindowCenter = center;
        presentation.WindowWidth = std::max(width, 1.0);
        applyLoadedFrame(m_currentFrameIndex);
}

void isis::gui::Widget2D::resetWindowLevel()
{
        if (!m_imagePresenter)
        {
                return;
        }
        const auto initialState = m_imagePresenter->initialState();
        auto& presentation = m_state.presentation();
        presentation.WindowCenter = initialState.WindowCenter;
        presentation.WindowWidth = initialState.WindowWidth;
        applyLoadedFrame(m_currentFrameIndex);
}

void isis::gui::Widget2D::handleRenderingFailure(const QString& t_reason)
{
        if (m_loadController)
        {
                m_loadController->handleFailure(t_reason);
        }
}

bool isis::gui::Widget2D::eventFilter(QObject* t_watched, QEvent* t_event)
{
        if (m_interactor && m_interactor->handleEvent(t_watched, t_event))
        {
                return true;
        }
        return WidgetBase::eventFilter(t_watched, t_event);
}


isis::gui::Widget2D::Widget2D(QWidget* parent)
        : WidgetBase(parent)
{
        qRegisterMetaType<Widget2D::FrameMetrics>("isis::gui::Widget2D::FrameMetrics");
        qRegisterMetaType<Widget2D::CursorInfo>("isis::gui::Widget2D::CursorInfo");
        qRegisterMetaType<Widget2D::WindowPreset>("isis::gui::Widget2D::WindowPreset");
        qRegisterMetaType<QVector<Widget2D::WindowPreset>>("QVector<isis::gui::Widget2D::WindowPreset>");
        initData();
        initView();
        createConnections();
        m_tabWidget = parent;
        m_state.resetInteractiveState();
        m_loadController = std::make_unique<Widget2DLoadController>(*this);
        if (m_loadController)
        {
                Q_UNUSED(connect(m_loadController.get(),
                        &Widget2DLoadController::framesReady,
                        this,
                        &Widget2D::onFramesReady));
        }
        m_overlayUpdater = std::make_unique<Widget2DOverlayUpdater>();
        m_interactor = std::make_unique<Widget2DInteractor>(*this, m_state);
        m_renderer = std::make_unique<Widget2DRenderer>(*this, m_state);
        ensureProgressiveRefreshTimer();
        if (m_progressiveRefreshTimer)
        {
                m_progressiveRefreshTimer->stop();
        }
        updateActiveToolUi();
        updateAvailableWindowPresets();
}
//-----------------------------------------------------------------------------
isis::gui::Widget2D::~Widget2D() = default;

//-----------------------------------------------------------------------------
void isis::gui::Widget2D::initView()
{
        if (layout())
        {
                delete layout();
        }
        m_ui.setupUi(this);
        auto* containerLayout = new QHBoxLayout(this);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->setSpacing(0);
        setLayout(containerLayout);
        ensureImageLabel();
        if (m_imageLabel && layout()->indexOf(m_imageLabel) == -1)
        {
                layout()->addWidget(m_imageLabel);
        }
        if (!m_errorLabel)
        {
                m_errorLabel = new QLabel(tr("Unable to render the selected image."), this);
                m_errorLabel->setAlignment(Qt::AlignCenter);
                m_errorLabel->setWordWrap(true);
                m_errorLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                m_errorLabel->setStyleSheet(QStringLiteral("color: #f44336; background-color: rgba(0, 0, 0, 180);"
                                                       "padding: 16px;"));
                m_errorLabel->hide();
        }
        layout()->addWidget(m_errorLabel);
        layout()->addWidget(m_scroll);
        if (m_imageLabel && m_vtkEvents)
        {
                m_imageLabel->installEventFilter(m_vtkEvents.get());
        }
}
//-----------------------------------------------------------------------------
void isis::gui::Widget2D::initData()
{
        disconnectScroll();
        if (auto* widgetLayout = layout())
        {
                if (m_scroll)
                {
                        widgetLayout->removeWidget(m_scroll);
                }
                if (m_imageLabel)
                {
                        widgetLayout->removeWidget(m_imageLabel);
                }
                if (m_errorLabel)
                {
                        widgetLayout->removeWidget(m_errorLabel);
                }
        }
        delete m_scroll;
        m_scroll = new QScrollBar(Qt::Vertical, this);
        setScrollStyle();
        if (!m_vtkEvents)
        {
                m_vtkEvents = std::make_unique<vtkEventFilter>(this);
        }
        m_pendingScrollRefresh = false;
        m_pendingSeriesRefresh = nullptr;
        m_pendingImageRefresh = nullptr;
        m_restoreFrameIndex = -1;
        m_progressiveRefreshPending = false;
        m_restoreFrameIndex = -1;
        m_hasCursorInfo = false;
        m_lastCursorInfo = {};
        if (m_progressiveRefreshTimer)
        {
                m_progressiveRefreshTimer->stop();
        }
}
//-----------------------------------------------------------------------------
void isis::gui::Widget2D::render()
{
        m_renderAbortedDueToMissingContext = false;
        if (!m_series || !m_image)
        {
                const auto trigger = m_lastRenderRequestSource.isEmpty()
                        ? QStringLiteral("unknown")
                        : m_lastRenderRequestSource;
                qCWarning(lcWidget2D)
                        << "Render aborted due to missing series/image context."
                        << "seriesMissing" << (m_series == nullptr)
                        << "imageMissing" << (m_image == nullptr)
                        << "trigger" << trigger
                        << "seriesPtr" << static_cast<const void*>(m_series)
                        << "imagePtr" << static_cast<const void*>(m_image);
                m_isImageLoaded = false;
                m_renderAbortedDueToMissingContext = true;
                return;
        }

        if (!startVolumeRendering())
        {
                qCWarning(lcWidget2D) << "GDCM rendering path failed to start."
                                      << "series" << static_cast<const void*>(m_series)
                                      << "image" << static_cast<const void*>(m_image);
        }
}

void isis::gui::Widget2D::waitForPendingTasks() const
{
        if (m_loadController)
        {
                m_loadController->waitForPendingTasks();
        }
}

void isis::gui::Widget2D::setFitToWindowEnabled(const bool t_enabled)
{
        if (m_fitToWindowEnabled == t_enabled)
        {
                return;
        }

        m_fitToWindowEnabled = t_enabled;
        m_state.setFitToWindowEnabled(t_enabled);
        if (m_fitToWindowEnabled)
        {
                resetPanOffset();
        }
        if (m_renderingActive && m_imagePresenter && m_imagePresenter->isValid())
        {
                refreshDisplayedFrame(true);
        }
}

void isis::gui::Widget2D::setActiveTool(const InteractionTool t_tool)
{
        if (m_activeTool == t_tool)
        {
                return;
        }

        m_activeTool = t_tool;
        m_windowLevelDragging = false;
        m_scrollDragging = false;
        m_zoomDragging = false;
        m_panDragging = false;
        m_scrollDragAccumulator = 0.0;
        updateActiveToolUi();
        emit activeToolChanged(m_activeTool);
}
//-----------------------------------------------------------------------------
void isis::gui::Widget2D::createConnections()
{
        setFocusPolicy(Qt::FocusPolicy::WheelFocus);
        if (m_imageLabel && m_vtkEvents)
        {
                m_imageLabel->installEventFilter(m_vtkEvents.get());
                Q_UNUSED(connect(m_vtkEvents.get(),
                        &vtkEventFilter::activateWidget,
                        this, &Widget2D::onActivateWidget));
                Q_UNUSED(connect(m_vtkEvents.get(),
                        &vtkEventFilter::setMaximized,
                        this, &Widget2D::onSetMaximized));
        }
}
//-----------------------------------------------------------------------------
void isis::gui::Widget2D::resetView()
{
        m_state.resetInteractiveState();
        resetScroll();
        if (m_errorLabel)
        {
                m_errorLabel->hide();
        }
        if (m_imageLabel)
        {
                m_imageLabel->hide();
                m_imageLabel->clear();
        }
        for (auto& orientationLabel : m_orientationLabels)
        {
                if (orientationLabel)
                {
                        orientationLabel->hide();
                }
        }
        m_imagePresenter.reset();
        m_state.setPresentation({});
        m_renderingActive = false;
        m_currentFrameIndex = 0;
        m_cachedFrame = {};
        m_displayZoomFactor = 1.0;
        m_manualZoomFactor = 1.0;
        m_fitToWindowEnabled = false;
        m_windowLevelDragging = false;
        m_scrollDragging = false;
        m_zoomDragging = false;
        m_panDragging = false;
        m_scrollDragAccumulator = 0.0;
        resetPanOffset();
        m_state.setInitialWindowCenter(0.0);
        m_state.setInitialWindowWidth(1.0);
        hideOverlayWidget();
        m_pendingScrollRefresh = false;
        m_pendingSeriesRefresh = nullptr;
        m_pendingImageRefresh = nullptr;
        m_restoreFrameIndex = -1;
        m_hasLastFrameMetrics = false;
        m_lastFrameMetrics = {};
        if (m_loadController)
        {
                m_loadController->waitForPendingTasks();
        }
        m_isImageLoaded = false;
        m_image = nullptr;
        m_series = nullptr;
        m_activeTool = InteractionTool::scroll;
        updateActiveToolUi();
        //todo reset title of tab
        disconnectScroll();
        createConnections();
        qCInfo(lcWidget2D) << "View reset.";
}
//-----------------------------------------------------------------------------
void isis::gui::Widget2D::setSliderValues(const int& t_min, const int& t_max, const int& t_value)
{
        if (m_scroll)
        {
                const QSignalBlocker blocker(m_scroll);
                m_scroll->setMinimum(t_min);
                m_scroll->setMaximum(t_max);
                m_scroll->setValue(t_value);
                qCDebug(lcWidget2D) << "Scroll slider configured. Min:" << t_min
                                    << "Max:" << t_max << "Value:" << t_value;
        }
}
//-----------------------------------------------------------------------------
void isis::gui::Widget2D::onActivateWidget(const bool& t_flag)
{
	if (t_flag)
	{
		auto* event = new QFocusEvent(QEvent::FocusIn,
			Qt::FocusReason::MouseFocusReason);
		focusInEvent(event);
		delete event;
	}
}
//-----------------------------------------------------------------------------
void isis::gui::Widget2D::onApplyTransformation(const transformationType& t_type)
{
        if (m_renderingActive && m_imagePresenter && m_imagePresenter->isValid())
        {
                auto& presentation = m_state.presentation();
                switch (t_type)
                {
                case transformationType::flipHorizontal:
                        presentation.FlipHorizontal = !presentation.FlipHorizontal;
                        break;
                case transformationType::flipVertical:
                        presentation.FlipVertical = !presentation.FlipVertical;
                        break;
                case transformationType::rotateLeft:
                        presentation.RotationSteps = (presentation.RotationSteps + 3) % 4;
                        break;
                case transformationType::rotateRight:
                        presentation.RotationSteps = (presentation.RotationSteps + 1) % 4;
                        break;
                case transformationType::invert:
                        presentation.InvertColors = !presentation.InvertColors;
                        break;
                default:
                        break;
                }
                applyLoadedFrame(m_currentFrameIndex);
                return;
        }

}
//-----------------------------------------------------------------------------
void isis::gui::Widget2D::onRefreshScrollValues(isis::core::Series* t_series, isis::core::Image* t_image)
{
        auto* const study = t_series->getParentObject();
        if (canScrollBeRefreshed(study->getParentObject()->getIndex(),
                study->getIndex(), t_series->getIndex()))
        {
                if (!m_image->getIsMultiFrame())
                {
                        const auto availableImages = static_cast<int>(t_series->snapshotSingleFramePaths().size());
                        const bool presenterReady = m_imagePresenter && m_imagePresenter->isValid();

                        if (!presenterReady)
                        {
                                if (availableImages > 0)
                                {
                                        m_pendingScrollRefresh = true;
                                        m_pendingSeriesRefresh = t_series;
                                        m_pendingImageRefresh = t_image;
                                        if (m_loadController && !m_loadController->isAppendRunning())
                                        {
                                                m_loadController->requestAppendFrames(t_series);
                                        }
                                        scheduleProgressiveRefresh();
                                }
                                return;
                        }

                        if (m_imagePresenter && m_imagePresenter->isValid() && availableImages > m_imagePresenter->frameCount())
                        {
                                m_pendingScrollRefresh = true;
                                m_pendingSeriesRefresh = t_series;
                                m_pendingImageRefresh = t_image;
                                if (m_imagePresenter->frameCount() == 0 && m_loadController && !m_loadController->isAppendRunning())
                                {
                                        m_loadController->requestAppendFrames(t_series);
                                }
                                scheduleProgressiveRefresh();
                                return;
                        }

                        const int presenterFrames = m_imagePresenter ? m_imagePresenter->frameCount() : availableImages;
                        const int maxIndex = std::max(presenterFrames - 1, 0);
                        const int currentScrollValue = m_scroll ? m_scroll->value() : 0;
                        int value =
                                (t_image->getIndex() <= currentScrollValue
                                && presenterFrames > 1 && t_image->getIndex() > 0)
                                ? currentScrollValue + 1
                                : currentScrollValue;
                        value = std::clamp(value, 0, maxIndex);
                        setSliderValues(0, maxIndex, value);
                        if (m_renderingActive && m_overlayUpdater)
                        {
                                const int frameCount = std::max(presenterFrames, availableImages);
                                const QString seriesNumber = (m_series)
                                        ? QString::fromStdString(m_series->getNumber())
                                        : QString();
                                const auto& presentation = m_state.presentation();
                                const double zoomFactor = (m_displayZoomFactor > 0.0 && std::isfinite(m_displayZoomFactor))
                                        ? m_displayZoomFactor
                                        : m_manualZoomFactor;
                                double spacingX = 0.0;
                                double spacingY = 0.0;
                                double sliceLocation = std::numeric_limits<double>::quiet_NaN();
                                if (t_image)
                                {
                                        spacingX = t_image->getPixelSpacingX();
                                        spacingY = t_image->getPixelSpacingY();
                                        sliceLocation = t_image->getSliceLocation();
                                }
                                else if (m_image)
                                {
                                        spacingX = m_image->getPixelSpacingX();
                                        spacingY = m_image->getPixelSpacingY();
                                        sliceLocation = m_image->getSliceLocation();
                                }
                                if (spacingX < 0.0)
                                {
                                        spacingX = 0.0;
                                }
                                if (spacingY < 0.0)
                                {
                                        spacingY = 0.0;
                                }
                                m_overlayUpdater->updateFrameOverlay(seriesNumber,
                                        value,
                                        frameCount,
                                        presentation.WindowCenter,
                                        presentation.WindowWidth,
                                        spacingX,
                                        spacingY,
                                        sliceLocation,
                                        zoomFactor,
                                        m_activeTool,
                                        m_imageLabel,
                                        m_renderingActive);
                        }
                        qCDebug(lcWidget2D) << "Scroll values refreshed from importer. Series index:" << t_series->getIndex()
                                            << "Image index:" << t_image->getIndex()
                                            << "Current value:" << value;
                }
        }
}

void isis::gui::Widget2D::onFramesReady(isis::core::Series* t_series, int t_appendedFrames)
{
        QMetaObject::invokeMethod(this,
                [this, t_series, t_appendedFrames]()
                {
                        handleFramesReadyOnGuiThread(t_series, t_appendedFrames);
                },
                Qt::QueuedConnection);
}

void isis::gui::Widget2D::handleFramesReadyOnGuiThread(isis::core::Series* t_series, int t_appendedFrames)
{
        if (!m_renderingActive || !m_imagePresenter)
        {
                m_pendingScrollRefresh = false;
                m_pendingSeriesRefresh = nullptr;
                m_pendingImageRefresh = nullptr;
        m_restoreFrameIndex = -1;
                m_restoreFrameIndex = -1;
                return;
        }

        if (t_series && m_series && t_series != m_series)
        {
                m_restoreFrameIndex = -1;
                return;
        }

        isis::core::Series* seriesToRefresh = m_pendingSeriesRefresh ? m_pendingSeriesRefresh : m_series;
        isis::core::Image* imageToRefresh = m_pendingImageRefresh ? m_pendingImageRefresh : m_image;

        const bool hadPendingRefresh = m_pendingScrollRefresh;
        m_pendingScrollRefresh = false;
        m_pendingSeriesRefresh = nullptr;
        m_pendingImageRefresh = nullptr;
        m_restoreFrameIndex = -1;

        if (!hadPendingRefresh && t_appendedFrames <= 0)
        {
                m_restoreFrameIndex = -1;
                return;
        }

        if (seriesToRefresh && imageToRefresh)
        {
                onRefreshScrollValues(seriesToRefresh, imageToRefresh);
        }

        if (m_restoreFrameIndex >= 0 && m_imagePresenter && m_imagePresenter->isValid())
        {
                const int maxIndex = std::max(m_imagePresenter->frameCount() - 1, 0);
                const int targetIndex = std::clamp(m_restoreFrameIndex, 0, maxIndex);
                if (m_scroll)
                {
                        const QSignalBlocker blocker(m_scroll);
                        m_scroll->setValue(targetIndex);
                }
                applyLoadedFrame(targetIndex);
                m_restoreFrameIndex = -1;
        }

        if (m_progressiveRefreshTimer && m_progressiveRefreshTimer->isActive()
                && !m_progressiveRefreshPending
                && (!m_loadController || !m_loadController->isAppendRunning()))
        {
                m_progressiveRefreshTimer->stop();
        }
}

void isis::gui::Widget2D::onSetMaximized() const
{
        if(m_tabWidget)
        {
                dynamic_cast<TabWidget*>(m_tabWidget)->onMaximize();
        }
}
//-----------------------------------------------------------------------------
void isis::gui::Widget2D::closeEvent(QCloseEvent* t_event)
{
        initData();
        initView();
        QWidget::closeEvent(t_event);
}
//-----------------------------------------------------------------------------
void isis::gui::Widget2D::onChangeImage(int t_index)
{
        if (!m_renderingActive)
        {
                qCDebug(lcWidget2D) << "Ignoring scroll request because GDCM rendering is inactive.";
                return;
        }
        applyLoadedFrame(t_index);
}
//-----------------------------------------------------------------------------
void isis::gui::Widget2D::connectScroll()
{
        if (m_scroll)
        {
                Q_UNUSED(connect(m_scroll, &QScrollBar::valueChanged,
                        this, &Widget2D::onChangeImage));
        }
}
//-----------------------------------------------------------------------------
void isis::gui::Widget2D::startLoadingAnimation()
{
        if (m_loadingAnimation)
        {
                m_loadingAnimation->hide();
                m_loadingAnimation.reset();
        }
}
//-----------------------------------------------------------------------------
void isis::gui::Widget2D::disconnectScroll() const
{
        if (m_scroll)
        {
                disconnect(m_scroll, &QScrollBar::valueChanged,
                           this, &Widget2D::onChangeImage);
        }
}
//-----------------------------------------------------------------------------
void isis::gui::Widget2D::resetScroll()
{
        if (m_scroll)
        {
		const QSignalBlocker blocker(m_scroll);
		m_scroll->setValue(0);
		m_scroll->setMaximum(0);
	}
	else
	{
		m_scroll = new QScrollBar(Qt::Vertical, this);
		setScrollStyle();
	}
}

void isis::gui::Widget2D::setScrollStyle() const
{
        m_scroll->hide();
        QFile file(scroll2DStyle);
        if (file.open(QFile::ReadOnly))
        {
                const QString styleSheet = QLatin1String(file.readAll());
                m_scroll->setStyleSheet(styleSheet);
        }
}

void isis::gui::Widget2D::resizeEvent(QResizeEvent* t_event)
{
        QWidget::resizeEvent(t_event);
        positionLoadingAnimation();
        positionOrientationLabels();
        if (m_renderingActive)
        {
                applyLoadedFrame(m_currentFrameIndex);
        }
}
//-----------------------------------------------------------------------------
bool isis::gui::Widget2D::canScrollBeRefreshed(const int& t_patientIndex, const int& t_studyIndex,
                                                    const int& t_seriesIndex) const
{
        return m_renderingActive && m_imagePresenter && m_imagePresenter->isValid()
                && t_patientIndex == m_patientIndex && t_studyIndex == m_studyIndex
                && m_seriesIndex == t_seriesIndex;
}



