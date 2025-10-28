/*
 * ------------------------------------------------------------------------------------
 *  File: vtkreslicecallback.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares observer callbacks used to react to reslice widget interaction events.
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

#include <vtkCommand.h>
#include "vtkreslicewidget.h"

namespace isis::gui
{
	class vtkResliceCallback final : public vtkCommand
	{
	public:
		static vtkResliceCallback* New();
		vtkTypeMacro(vtkResliceCallback, vtkCommand);
		vtkResliceCallback() = default;
		~vtkResliceCallback() = default;


		//getters
		[[nodiscard]] vtkResliceWidget* getWidget() const { return m_resliceWidget; }
		[[nodiscard]] vtkRenderWindow* getRenderWindow() const { return m_window; }
		[[nodiscard]] int getHandleNumber() const { return m_handleNumber; }

		//setters
		void setWidget(vtkResliceWidget* t_widget) { m_resliceWidget = t_widget; }
		void setRenderWindow(vtkRenderWindow* t_window) { m_window = t_window; }
		void setHandleNumber(const int& t_nr) { m_handleNumber = t_nr; }

		void Execute(vtkObject* caller, unsigned long eventId, void* callData) override;

	private:
		vtkResliceWidget* m_resliceWidget = {};
		vtkRenderWindow* m_window = {};
		int m_handleNumber = {};

		void setCursorPositiontoDefault() const;
		void rotateCursor(double t_angle) const;
		void moveCursor(double* t_position) const;
		void changeMatrixCenterPosition(
			vtkMatrix4x4* t_sourceMatrix, vtkMatrix4x4* t_destinationMatrix);
	};
}

