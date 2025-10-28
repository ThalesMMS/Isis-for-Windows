/*
 * ------------------------------------------------------------------------------------
 *  File: widget2dinteractor.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Defines the interactive behaviour for 2D slice widgets, including mouse tools and gestures.
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

#include "widget2dinteractor.h"

#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QSize>
#include <algorithm>
#include <cmath>

#include "widget2d.h"
#include "widget2dstate.h"
#include "widget2dimagepresenter.h"

namespace isis::gui
{
        Widget2DInteractor::Widget2DInteractor(Widget2D& t_widget, Widget2DState& t_state)
                : QObject(&t_widget)
                , m_widget(t_widget)
                , m_state(t_state)
        {
        }

        bool Widget2DInteractor::handleEvent(QObject* t_watched, QEvent* t_event)
        {
                if (t_watched != m_widget.m_imageLabel || !t_event)
                {
                        return false;
                }

                const bool renderActive = m_widget.m_renderingActive
                        && m_widget.m_imagePresenter
                        && m_widget.m_imagePresenter->isValid();
                switch (t_event->type())
                {
                case QEvent::Wheel:
                {
                        if (!renderActive)
                        {
                                break;
                        }
                        auto* wheelEvent = static_cast<QWheelEvent*>(t_event);
                        const QPoint angleDelta = wheelEvent->angleDelta();
                        if (wheelEvent->modifiers().testFlag(Qt::ControlModifier))
                        {
                                double relativeStep = 0.0;
                                if (angleDelta.y() != 0)
                                {
                                        relativeStep = static_cast<double>(angleDelta.y()) / 120.0;
                                }
                                else if (angleDelta.x() != 0)
                                {
                                        relativeStep = static_cast<double>(angleDelta.x()) / 120.0;
                                }
                                if (relativeStep == 0.0)
                                {
                                        relativeStep = (angleDelta.y() > 0 || angleDelta.x() > 0)
                                                ? 1.0
                                                : ((angleDelta.y() < 0 || angleDelta.x() < 0) ? -1.0 : 0.0);
                                }
                                if (relativeStep != 0.0)
                                {
                                        constexpr double scaleStep = 0.1;
                                        m_widget.m_fitToWindowEnabled = false;
                                        m_state.setFitToWindowEnabled(false);
                                        const double newFactor = std::clamp(
                                                m_widget.m_manualZoomFactor + scaleStep * relativeStep,
                                                0.1,
                                                8.0);
                                        m_widget.m_manualZoomFactor = newFactor;
                                        m_state.setManualZoomFactor(newFactor);
                                        m_widget.refreshDisplayedFrame(true);
                                }
                        }
                        else
                        {
                                int steps = 0;
                                if (angleDelta.y() != 0)
                                {
                                        steps = angleDelta.y() / 120;
                                        if (steps == 0)
                                        {
                                                steps = angleDelta.y() > 0 ? 1 : -1;
                                        }
                                }
                                else if (angleDelta.x() != 0)
                                {
                                        steps = angleDelta.x() / 120;
                                        if (steps == 0)
                                        {
                                                steps = angleDelta.x() > 0 ? 1 : -1;
                                        }
                                }
                                if (steps != 0)
                                {
                                        m_widget.adjustFrameByStep(-steps);
                                }
                        }
                        wheelEvent->accept();
                        return true;
                }
                case QEvent::MouseButtonPress:
                {
                        if (!renderActive)
                        {
                                break;
                        }
                        auto* mouseEvent = static_cast<QMouseEvent*>(t_event);
                        if (mouseEvent->button() == Qt::LeftButton)
                        {
                                m_widget.m_lastMousePosition = mouseEvent->pos();
                                m_state.setLastMousePosition(mouseEvent->pos());
                                switch (m_widget.m_activeTool)
                                {
                                case InteractionTool::scroll:
                                        m_widget.m_scrollDragging = true;
                                        m_state.setScrollDragging(true);
                                        m_widget.m_scrollDragAccumulator = 0.0;
                                        m_state.setScrollDragAccumulator(0.0);
                                        mouseEvent->accept();
                                        return true;
                                case InteractionTool::window:
                                {
                                        m_widget.m_windowLevelDragging = true;
                                        m_state.setWindowLevelDragging(true);
                                        const auto initialState = m_widget.m_imagePresenter->initialState();
                                        auto& presentation = m_state.presentation();
                                if (presentation.WindowWidth <= 0.0)
                                {
                                        presentation.WindowWidth = initialState.WindowWidth;
                                        presentation.WindowCenter = initialState.WindowCenter;
                                }
                                m_state.setInitialWindowCenter(presentation.WindowCenter);
                                m_state.setInitialWindowWidth(std::max<double>(presentation.WindowWidth, 1.0));
                                m_state.setWindowDragStart(mouseEvent->pos());
                                mouseEvent->accept();
                                return true;
                            }
                                case InteractionTool::zoom:
                                        m_widget.m_zoomDragging = true;
                                        m_state.setZoomDragging(true);
                                        m_widget.m_fitToWindowEnabled = false;
                                        m_state.setFitToWindowEnabled(false);
                                        mouseEvent->accept();
                                        return true;
                                case InteractionTool::pan:
                                        m_widget.m_panDragging = true;
                                        m_state.setPanDragging(true);
                                        m_widget.m_fitToWindowEnabled = false;
                                        m_state.setFitToWindowEnabled(false);
                                        m_widget.setCursorForActiveTool(true);
                                        mouseEvent->accept();
                                        return true;
                                default:
                                        break;
                                }
                        }
                        if (mouseEvent->button() == Qt::MiddleButton)
                        {
                                m_widget.m_fitToWindowEnabled = false;
                                m_state.setFitToWindowEnabled(false);
                                m_widget.m_manualZoomFactor = 1.0;
                                m_state.setManualZoomFactor(1.0);
                                m_widget.resetPanOffset();
                                m_state.resetPanOffset();
                                m_widget.refreshDisplayedFrame(true);
                                mouseEvent->accept();
                                return true;
                        }
                        if (mouseEvent->button() == Qt::RightButton)
                        {
                                if (m_widget.m_activeTool == InteractionTool::pan)
                                {
                                        m_widget.setCursorForActiveTool();
                                }
                                m_widget.m_fitToWindowEnabled = false;
                                m_state.setFitToWindowEnabled(false);
                                mouseEvent->accept();
                                return true;
                        }
                        break;
                }
                case QEvent::MouseMove:
                {
                        auto* mouseEvent = static_cast<QMouseEvent*>(t_event);
                        if (!renderActive)
                        {
                                break;
                        }
                        m_widget.handleCursorHover(mouseEvent->pos());
                        if (m_widget.m_scrollDragging && m_widget.m_scroll)
                        {
                                const QPoint delta = mouseEvent->pos() - m_widget.m_lastMousePosition;
                                m_widget.m_scrollDragAccumulator += static_cast<double>(delta.y());
                                m_state.setScrollDragAccumulator(m_widget.m_scrollDragAccumulator);
                                constexpr double pixelsPerStep = 5.0;
                                const int steps = static_cast<int>(m_widget.m_scrollDragAccumulator / pixelsPerStep);
                                if (steps != 0)
                                {
                                        m_widget.adjustFrameByStep(steps);
                                        m_widget.m_scrollDragAccumulator -= static_cast<double>(steps) * pixelsPerStep;
                                        m_state.setScrollDragAccumulator(m_widget.m_scrollDragAccumulator);
                                }
                                m_widget.m_lastMousePosition = mouseEvent->pos();
                                m_state.setLastMousePosition(mouseEvent->pos());
                                mouseEvent->accept();
                                return true;
                        }
                        if (m_widget.m_windowLevelDragging && m_widget.m_activeTool == InteractionTool::window)
                        {
                                const QPoint startPos = m_state.windowDragStart();
                                const QPoint delta = mouseEvent->pos() - startPos;
                                const double baseWidth = std::max<double>(m_state.initialWindowWidth(), 1.0);
                                const QSize labelSize = (m_widget.m_imageLabel)
                                        ? m_widget.m_imageLabel->size()
                                        : QSize();
                                const double horizontalScale = (labelSize.width() > 0)
                                        ? baseWidth / static_cast<double>(labelSize.width())
                                        : baseWidth / 300.0;
                                const double verticalScale = (labelSize.height() > 0)
                                        ? baseWidth / static_cast<double>(labelSize.height())
                                        : baseWidth / 300.0;
                                double sensitivity = 1.0;
                                if (mouseEvent->modifiers().testFlag(Qt::ShiftModifier))
                                {
                                        sensitivity = 0.25;
                                }
                                else if (mouseEvent->modifiers().testFlag(Qt::AltModifier))
                                {
                                        sensitivity = 2.0;
                                }
                                const double widthChange = static_cast<double>(delta.x()) * horizontalScale * sensitivity;
                                const double centerChange = static_cast<double>(-delta.y()) * verticalScale * sensitivity;
                                auto& presentation = m_state.presentation();
                                presentation.WindowWidth = std::max<double>(1.0, baseWidth + widthChange);
                                presentation.WindowCenter = m_state.initialWindowCenter() + centerChange;
                                m_widget.applyLoadedFrame(m_widget.m_currentFrameIndex);
                                m_widget.m_lastMousePosition = mouseEvent->pos();
                                m_state.setLastMousePosition(mouseEvent->pos());
                                mouseEvent->accept();
                                return true;
                        }
                        if (m_widget.m_zoomDragging)
                        {
                                const QPoint delta = mouseEvent->pos() - m_widget.m_lastMousePosition;
                                if (delta.y() != 0)
                                {
                                        constexpr double sensitivity = 0.01;
                                        const double factor = 1.0 - sensitivity * static_cast<double>(delta.y());
                                        if (factor > 0.0)
                                        {
                                                const double newFactor = std::clamp(
                                                        m_widget.m_manualZoomFactor * factor,
                                                        0.1,
                                                        8.0);
                                                m_widget.m_manualZoomFactor = newFactor;
                                                m_state.setManualZoomFactor(newFactor);
                                                m_widget.refreshDisplayedFrame(true);
                                        }
                                }
                                m_widget.m_lastMousePosition = mouseEvent->pos();
                                m_state.setLastMousePosition(mouseEvent->pos());
                                mouseEvent->accept();
                                return true;
                        }
                        if (m_widget.m_panDragging)
                        {
                                const QPoint delta = mouseEvent->pos() - m_widget.m_lastMousePosition;
                                m_widget.m_panOffset += QPointF(delta);
                                m_state.setPanOffset(m_widget.m_panOffset);
                                m_widget.m_fitToWindowEnabled = false;
                                m_state.setFitToWindowEnabled(false);
                                m_widget.refreshDisplayedFrame(true);
                                m_widget.m_lastMousePosition = mouseEvent->pos();
                                m_state.setLastMousePosition(mouseEvent->pos());
                                mouseEvent->accept();
                                return true;
                        }
                        break;
                }
                case QEvent::MouseButtonRelease:
                {
                        auto* mouseEvent = static_cast<QMouseEvent*>(t_event);
                        if (!mouseEvent)
                        {
                                break;
                        }
                        if (mouseEvent->button() != Qt::LeftButton)
                        {
                                break;
                        }
                        bool handled = false;
                        if (m_widget.m_windowLevelDragging)
                        {
                                m_widget.m_windowLevelDragging = false;
                                m_state.setWindowLevelDragging(false);
                                handled = true;
                        }
                        if (m_widget.m_scrollDragging)
                        {
                                m_widget.m_scrollDragging = false;
                                m_state.setScrollDragging(false);
                                m_widget.m_scrollDragAccumulator = 0.0;
                                m_state.setScrollDragAccumulator(0.0);
                                handled = true;
                        }
                        if (m_widget.m_zoomDragging)
                        {
                                m_widget.m_zoomDragging = false;
                                m_state.setZoomDragging(false);
                                handled = true;
                        }
                        if (m_widget.m_panDragging)
                        {
                                m_widget.m_panDragging = false;
                                m_state.setPanDragging(false);
                                handled = true;
                                m_widget.setCursorForActiveTool();
                        }
                        if (handled)
                        {
                                mouseEvent->accept();
                                return true;
                        }
                        break;
                }
                case QEvent::MouseButtonDblClick:
                {
                        if (!renderActive)
                        {
                                break;
                        }
                        auto* mouseEvent = static_cast<QMouseEvent*>(t_event);
                        if (mouseEvent->button() == Qt::LeftButton
                                && mouseEvent->modifiers().testFlag(Qt::ControlModifier)
                                && m_widget.m_activeTool == InteractionTool::window)
                        {
                                m_widget.resetWindowLevel();
                                m_widget.m_lastMousePosition = mouseEvent->pos();
                                m_state.setLastMousePosition(mouseEvent->pos());
                                mouseEvent->accept();
                                return true;
                        }
                        if (mouseEvent->button() == Qt::MiddleButton)
                        {
                                m_widget.m_manualZoomFactor = 1.0;
                                m_state.setManualZoomFactor(1.0);
                                m_widget.resetPanOffset();
                                m_state.resetPanOffset();
                                m_widget.refreshDisplayedFrame(true);
                                mouseEvent->accept();
                                return true;
                        }
                        break;
                }
                case QEvent::Leave:
                {
                        m_widget.clearCursorInfo();
                        break;
                }
                default:
                        break;
                }
                return false;
        }
}

