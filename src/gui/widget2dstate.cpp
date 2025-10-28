/*
 * ------------------------------------------------------------------------------------
 *  File: widget2dstate.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements state tracking for 2D widgets, covering window/level and orientation data.
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

#include "widget2dstate.h"

namespace isis::gui
{
        Widget2DState::Widget2DState()
        {
        }

        void Widget2DState::resetInteractiveState()
        {
                m_fitToWindowEnabled = false;
                m_manualZoomFactor = 1.0;
                m_displayZoomFactor = 1.0;
                m_windowLevelDragging = false;
                m_scrollDragging = false;
                m_zoomDragging = false;
                m_panDragging = false;
                m_scrollDragAccumulator = 0.0;
                m_windowDragStart = {};
                resetPanOffset();
        }

        void Widget2DState::resetForNewSeries(const Widget2dPresentationState& t_initialState)
        {
                m_presentation = t_initialState;
                resetInteractiveState();
                m_initialWindowCenter = m_presentation.WindowCenter;
                m_initialWindowWidth = (m_presentation.WindowWidth > 0.0)
                        ? m_presentation.WindowWidth
                        : 1.0;
        }

        const Widget2dPresentationState& Widget2DState::presentation() const
        {
                return m_presentation;
        }

        Widget2dPresentationState& Widget2DState::presentation()
        {
                return m_presentation;
        }

        void Widget2DState::setPresentation(const Widget2dPresentationState& t_state)
        {
                m_presentation = t_state;
        }

        void Widget2DState::setFitToWindowEnabled(bool t_enabled)
        {
                m_fitToWindowEnabled = t_enabled;
        }

        bool Widget2DState::isFitToWindowEnabled() const
        {
                return m_fitToWindowEnabled;
        }

        void Widget2DState::setManualZoomFactor(double t_factor)
        {
                m_manualZoomFactor = t_factor;
        }

        double Widget2DState::manualZoomFactor() const
        {
                return m_manualZoomFactor;
        }

        void Widget2DState::setDisplayZoomFactor(double t_factor)
        {
                m_displayZoomFactor = t_factor;
        }

        double Widget2DState::displayZoomFactor() const
        {
                return m_displayZoomFactor;
        }

        void Widget2DState::setPanOffset(const QPointF& t_offset)
        {
                m_panOffset = t_offset;
        }

        const QPointF& Widget2DState::panOffset() const
        {
                return m_panOffset;
        }

        void Widget2DState::resetPanOffset()
        {
                m_panOffset = {};
        }

        void Widget2DState::setWindowLevelDragging(bool t_dragging)
        {
                m_windowLevelDragging = t_dragging;
        }

        bool Widget2DState::isWindowLevelDragging() const
        {
                return m_windowLevelDragging;
        }

        void Widget2DState::setScrollDragging(bool t_dragging)
        {
                m_scrollDragging = t_dragging;
        }

        bool Widget2DState::isScrollDragging() const
        {
                return m_scrollDragging;
        }

        void Widget2DState::setZoomDragging(bool t_dragging)
        {
                m_zoomDragging = t_dragging;
        }

        bool Widget2DState::isZoomDragging() const
        {
                return m_zoomDragging;
        }

        void Widget2DState::setPanDragging(bool t_dragging)
        {
                m_panDragging = t_dragging;
        }

        bool Widget2DState::isPanDragging() const
        {
                return m_panDragging;
        }

        void Widget2DState::setLastMousePosition(const QPoint& t_position)
        {
                m_lastMousePosition = t_position;
        }

        const QPoint& Widget2DState::lastMousePosition() const
        {
                return m_lastMousePosition;
        }

        void Widget2DState::setWindowDragStart(const QPoint& t_position)
        {
                m_windowDragStart = t_position;
        }

        const QPoint& Widget2DState::windowDragStart() const
        {
                return m_windowDragStart;
        }

        void Widget2DState::setScrollDragAccumulator(double t_value)
        {
                m_scrollDragAccumulator = t_value;
        }

        double Widget2DState::scrollDragAccumulator() const
        {
                return m_scrollDragAccumulator;
        }

        void Widget2DState::setInitialWindowCenter(double t_value)
        {
                m_initialWindowCenter = t_value;
        }

        double Widget2DState::initialWindowCenter() const
        {
                return m_initialWindowCenter;
        }

        void Widget2DState::setInitialWindowWidth(double t_value)
        {
                m_initialWindowWidth = t_value;
        }

        double Widget2DState::initialWindowWidth() const
        {
                return m_initialWindowWidth;
        }
}


