/*
 * ------------------------------------------------------------------------------------
 *  File: vtkwidget2d.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the vtkWidget2D class encapsulating render window and mapper setup for 2D slices.
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

#include "vtkwidgetbase.h"
#include "vtkwidgetdicom.h"
#include "vtkwidgetoverlay.h"
#include "widget2d.h"

#include <memory>

#include "dicomvolume.h"
#include <string>

enum class transformationType;

namespace isis::gui
{
        class vtkWidget2D final : public vtkWidgetBase
        {
        public:
                vtkWidget2D();
                ~vtkWidget2D();

                //getters
                [[nodiscard]] vtkSmartPointer<vtkWidgetDICOM> getDCMWidget() const { return m_dcmWidget; }
                [[nodiscard]] vtkRenderWindowInteractor* getInteractor() const { return m_interactor; }
                [[nodiscard]] InteractionTool activeTool() const { return m_activeTool; }

                //setters
                void setVolume(const std::shared_ptr<core::DicomVolume>& t_volume) { m_volume = t_volume; }
                void setInteractor(vtkRenderWindowInteractor* t_interactor) override;
                void setActiveTool(InteractionTool t_tool);
                void publishBridgeProperty(Widget2D* t_widget);
                static vtkWidget2D* findForContext(const core::Series* t_series, const core::Image* t_image);

                void initImageReader();
                void render() override;
                void applyTransformation(const transformationType& t_type);
                void updateOverlayZoomFactor();
                void updateOverlayHUValue(const int& x, const int& y);
                void updateOvelayImageNumber(const int& t_current, const int& t_max, const int& t_numberOfSeries);
                void updateOverlayWindowLevelApply(const int& t_window, const int& t_level, const bool& t_apply);
                void resetOverlay();
                void updateOverlayTool(const std::string& t_toolLabel);

        private:
                std::shared_ptr<core::DicomVolume> m_volume = {};
                vtkSmartPointer<vtkWidgetDICOM> m_dcmWidget = {};
                vtkRenderWindowInteractor* m_interactor = nullptr;
                vtkSmartPointer<vtkRenderer> m_vtkWidgetOverlayRenderer = {};
                std::unique_ptr<vtkWidgetOverlay> m_widgetOverlay = {};
                bool m_colorsInverted = false;
                InteractionTool m_activeTool;
                QPointer<Widget2D> m_bridgeTarget = {};

                void initRenderingLayers();
                /**
                 * Initializes the vtkWidgetDICOM instance with image data.
                 * @return False when no renderable data could be configured for rendering.
                 */
                bool initWidgetDICOM();
                void createvtkWidgetOverlay();
                void invertColors();
                void fitImage() const;
                void refreshOverlayInCorner(const int& t_corner);
                void updateOverlayMouseCoordinates(const int& x, const int& y) const;
                [[nodiscard]] std::string computeHUValueInPixel(const int& t_pixel) const;
                [[nodiscard]] double computeScale() const;
        };
}

