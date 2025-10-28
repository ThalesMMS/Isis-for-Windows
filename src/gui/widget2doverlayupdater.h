/*
 * ------------------------------------------------------------------------------------
 *  File: widget2doverlayupdater.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares a helper responsible for configuring and updating the 2D overlay widget.
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

#include <QPointer>
#include <QString>

#include "dcmtkoverlaywidget.h"

class QLabel;

namespace isis::core
{
        class Series;
}

namespace isis::gui
{
        enum class InteractionTool;

        class Widget2DOverlayUpdater
        {
        public:
                Widget2DOverlayUpdater() = default;
                ~Widget2DOverlayUpdater() = default;

                void ensureOverlayWidget(QLabel* t_parentLabel);
                void hideOverlay();
                void updateToolOverlay(InteractionTool t_tool, bool t_renderingActive, QLabel* t_parentLabel);
                void updateMetadata(isis::core::Series* t_series, QLabel* t_parentLabel);
                void updateFrameOverlay(const QString& t_seriesNumber,
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
                        bool t_renderingActive);

                [[nodiscard]] DcmtkOverlayWidget* overlayWidget() const { return m_overlayWidget; }

        private:
                [[nodiscard]] QString toolDisplayName(InteractionTool t_tool) const;

                QPointer<DcmtkOverlayWidget> m_overlayWidget = {};
        };
}
