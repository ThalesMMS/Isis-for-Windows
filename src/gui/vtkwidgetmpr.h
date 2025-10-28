/*
 * ------------------------------------------------------------------------------------
 *  File: vtkwidgetmpr.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the vtkWidgetMPR class used to display synchronized axial, sagittal, and coronal views.
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

#include <array>
#include <vtkAxisActor2D.h>
#include <QString>
#include "vtkwidgetbase.h"
#include "mprmaker.h"
#include "vtkwidgetoverlay.h"
#include "vtkreslicewidget.h"
#include "vtkwidgetmprcallback.h"

namespace isis::gui
{
	class vtkWidgetMPR final : public vtkWidgetBase
	{
	public:
		vtkWidgetMPR() { initializeWidget(); }
		~vtkWidgetMPR();

		//getters
		[[nodiscard]] int getNumberOfRenderWindow(vtkRenderWindow* t_window) const;
                [[nodiscard]] bool hasValidVolume();
                [[nodiscard]] QString lastFailureMessage() const { return m_mprMaker ? m_mprMaker->lastFailureMessage() : QString(); }
                [[nodiscard]] QString lastWarningMessage() const { return m_mprMaker ? m_mprMaker->lastWarningMessage() : QString(); }

		//setters
		void setInteractor([[maybe_unused]] vtkRenderWindowInteractor* t_interactor) override {}
		void setActiveRenderWindow(vtkRenderWindow* t_window) { m_activeRenderWindow = t_window; }
		void setWindowLevel(const int& t_window, const int& t_level);
		void setCameraCentered(const int& t_centered) const;
		void setShowCursor(const bool& t_flag) const { m_resliceWidget->setVisible(t_flag); }
		
                void render() override;
                void resetResliceWidget();
                void changeWindowLevel(vtkRenderWindow* t_renderWindow, const int& t_window,
                        const int& t_level, bool t_append = true);
                void create3DMatrix() const;

                void setVolumeForTesting(const std::shared_ptr<VtkDicomVolume>& volume);
		bool crosshairSegmentsForView(int t_viewIndex,
			std::array<std::array<double, 4>, 2>& t_segments) const;

	private:
		vtkSmartPointer<vtkWidgetMPRCallback> m_callback = {};
		vtkSmartPointer<vtkResliceWidget> m_resliceWidget = {};
		std::unique_ptr<MPRMaker> m_mprMaker = {};
		std::unique_ptr<vtkWidgetOverlay> m_widgetOverlay[3] = {};
		unsigned int m_callbackTags[3] = {};

		void initializeWidget();
		void createResliceWidget();
		void createVTKkWidgetOverlay(vtkRenderWindow* t_window, int& t_windowNumber);
		void refreshOverlayInCorner(vtkRenderWindow* t_window, int t_windowNumber, int t_corner);
		[[nodiscard]] vtkAxisActor2D* createScaleActor(vtkRenderWindow* t_window);
	};
}

