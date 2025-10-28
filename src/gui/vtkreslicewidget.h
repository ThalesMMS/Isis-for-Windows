/*
 * ------------------------------------------------------------------------------------
 *  File: vtkreslicewidget.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the vtkResliceWidget wrapper integrating VTK reslice widgets into the GUI.
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

#include <vtkAbstractWidget.h>
#include <vtkSmartPointer.h>
#include <vtkImageResliceToColors.h>
#include <vtkRenderWindow.h>
#include "vtkresliceplanecursorwidget.h"
#include <vtkImageActor.h>

namespace isis::gui
{
	class vtkResliceCallback;

	class vtkResliceWidget final : public vtkAbstractWidget
	{
	public:
		static vtkResliceWidget* New();
		vtkTypeMacro(vtkResliceWidget, vtkAbstractWidget);
		vtkResliceWidget() = default;
		~vtkResliceWidget() = default;

		//getters
		[[nodiscard]] vtkRenderWindow* getTestRenderWindow() const { return m_testRenderWindow; }
		[[nodiscard]] int getIsCameraCentered() const { return m_isCameraCentered; }

		//setters
		void SetEnabled(int) override;
		void setImageReslicers(
			const vtkSmartPointer<vtkImageResliceToColors>& m_firstReslice,
			const vtkSmartPointer<vtkImageResliceToColors>& m_secondReslice,
			const vtkSmartPointer<vtkImageResliceToColors>& m_thirdReslice);
		vtkSmartPointer<vtkImageResliceToColors>* getImageReslicers() { return m_imageReslice; }
		void setRenderWindows(vtkSmartPointer<vtkRenderWindow>* t_windows);
		void refreshWindows(int t_windowNumber);
		void setVisible(bool);
		void setCameraCentered(int t_centered);
		void setHighQuality(int t_highQuality, int t_plane);

		void CreateDefaultRepresentation() override;
		void centerImageActors(int t_excludedCursor);
		void resetResliceCursor();
		[[nodiscard]] vtkReslicePlaneCursorWidget* getCursorWidget(int t_index) const
		{
			return (t_index >= 0 && t_index < 3) ? m_cursorWidget[t_index] : nullptr;
		}

		enum widgetState { start = 0, rotate, translate };

	private:
		vtkSmartPointer<vtkImageResliceToColors> m_imageReslice[3] = {};
		vtkSmartPointer<vtkRenderWindow> m_windows[3] = {};
		vtkSmartPointer<vtkReslicePlaneCursorWidget> m_cursorWidget[3] = {};
		vtkSmartPointer<vtkResliceCallback> m_cbk[3] = {};
		vtkRenderWindow* m_testRenderWindow = nullptr;
		int m_isCameraCentered = 0;

		void resetCamera(vtkRenderWindow* t_window);
		double* getImageActorCenterPosition(vtkRenderWindow* t_window);
		void setQualityToHigh(int t_windowNumber, vtkImageActor* t_actor);
		void setQualityToLow(int t_windowNumber, vtkImageActor* t_actor);

	protected:
		friend class vtkResliceCallback;
	};
}

