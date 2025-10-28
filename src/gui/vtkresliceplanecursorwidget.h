/*
 * ------------------------------------------------------------------------------------
 *  File: vtkresliceplanecursorwidget.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the vtkReslicePlaneCursorWidget used to track intersection lines between slices.
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
#include <vtkHandleWidget.h>

class vtkReslicePlaneCursorCallback;

namespace isis::gui
{
	class vtkReslicePlaneCursorWidget final : public vtkAbstractWidget
	{
	public:

		static vtkReslicePlaneCursorWidget* New();
	vtkTypeMacro(vtkReslicePlaneCursorWidget, vtkAbstractWidget);

		vtkReslicePlaneCursorWidget();
		~vtkReslicePlaneCursorWidget();

		void CreateDefaultRepresentation() override;
		void SetEnabled(int) override;
		void setPlane(int t_plane);
		void SetCursor(int) override;
		void setCursorPosition(double* t_position) const;
		void setCursorCenterPosition(const double* t_position);
		double* getCursorCenterPosition();


	private:
		vtkSmartPointer<vtkHandleWidget> m_centerMovementWidget = {};
		vtkSmartPointer<vtkReslicePlaneCursorCallback> m_centerMovementCallback = {};

		void initializeWidget();

		enum widgetState { start = 0, rotate, translate, thickness_horizontal, thickness_vertical };

	protected:
		int m_state = 0;
		int m_plane = -1;
		double lastCursorPos[3]{};
		double m_cursorCenterPosition[3]{};

		void startWidgetInteraction(int handleNum);
		void widgetInteraction(int handleNum);
		void endWidgetInteraction(int handleNum);

		static void leftMouseDownAction(vtkAbstractWidget* w);
		static void moveMouse(vtkAbstractWidget* w);
		static void leftMouseUpAction(vtkAbstractWidget* w);

		void rotateCursor(double t_angle) const;

		friend class vtkReslicePlaneCursorCallback;
	};
}

