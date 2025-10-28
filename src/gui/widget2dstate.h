/*
 * ------------------------------------------------------------------------------------
 *  File: widget2dstate.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the Widget2DState structure capturing persistent state for 2D render widgets.
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

#pragma once

#include <QPoint>
#include <QPointF>

#include "widget2dpresentationstate.h"

namespace isis::gui
{
        class Widget2DState
        {
        public:
                Widget2DState();

                void resetInteractiveState();
                void resetForNewSeries(const Widget2dPresentationState& t_initialState);

                [[nodiscard]] const Widget2dPresentationState& presentation() const;
                [[nodiscard]] Widget2dPresentationState& presentation();

                void setPresentation(const Widget2dPresentationState& t_state);

                void setFitToWindowEnabled(bool t_enabled);
                [[nodiscard]] bool isFitToWindowEnabled() const;

                void setManualZoomFactor(double t_factor);
                [[nodiscard]] double manualZoomFactor() const;

                void setDisplayZoomFactor(double t_factor);
                [[nodiscard]] double displayZoomFactor() const;

                void setPanOffset(const QPointF& t_offset);
                [[nodiscard]] const QPointF& panOffset() const;
                void resetPanOffset();

                void setWindowLevelDragging(bool t_dragging);
                [[nodiscard]] bool isWindowLevelDragging() const;

                void setScrollDragging(bool t_dragging);
                [[nodiscard]] bool isScrollDragging() const;

                void setZoomDragging(bool t_dragging);
                [[nodiscard]] bool isZoomDragging() const;

                void setPanDragging(bool t_dragging);
                [[nodiscard]] bool isPanDragging() const;

                void setLastMousePosition(const QPoint& t_position);
                [[nodiscard]] const QPoint& lastMousePosition() const;

                void setWindowDragStart(const QPoint& t_position);
                [[nodiscard]] const QPoint& windowDragStart() const;

                void setScrollDragAccumulator(double t_value);
                [[nodiscard]] double scrollDragAccumulator() const;

                void setInitialWindowCenter(double t_value);
                [[nodiscard]] double initialWindowCenter() const;

                void setInitialWindowWidth(double t_value);
                [[nodiscard]] double initialWindowWidth() const;

        private:
                Widget2dPresentationState m_presentation = {};
                bool m_fitToWindowEnabled = false;
                double m_manualZoomFactor = 1.0;
                double m_displayZoomFactor = 1.0;
                QPointF m_panOffset = {};
                bool m_windowLevelDragging = false;
                bool m_scrollDragging = false;
                bool m_zoomDragging = false;
                bool m_panDragging = false;
                QPoint m_lastMousePosition = {};
                QPoint m_windowDragStart = {};
                double m_scrollDragAccumulator = 0.0;
                double m_initialWindowCenter = 0.0;
                double m_initialWindowWidth = 1.0;
        };
}


