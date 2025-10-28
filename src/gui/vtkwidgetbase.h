/*
 * ------------------------------------------------------------------------------------
 *  File: vtkwidgetbase.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the vtkWidgetBase abstract class that manages render windows and associated data.
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
#include <series.h>
#include <vtkRenderWindow.h>

class vtkRenderWindowInteractor;

namespace isis::gui
{
	class vtkWidgetBase
	{
	public:
		vtkWidgetBase() = default;
		virtual ~vtkWidgetBase() = default;

		//getters
		[[nodiscard]] vtkSmartPointer<vtkRenderWindow>* getRenderWindows() { return m_renderWindows; }
		[[nodiscard]] vtkRenderWindow* getActiveRenderWindow() const { return m_activeRenderWindow; }
		[[nodiscard]] core::Image* getImage() const { return m_image; }
		[[nodiscard]] core::Series* getSeries() const { return m_series; }

		//setters
		void setImage(core::Image* t_image) { m_image = t_image; }
		void setSeries(core::Series* t_series) { m_series = t_series; }
		void setRenderWindow(const vtkSmartPointer<vtkRenderWindow>& t_renderWindow)
		{
			m_renderWindows[0] = t_renderWindow;
			onRenderWindowAssigned(m_renderWindows[0]);
		}
		void setRenderWindowsMPR(vtkRenderWindow* t_sagittal,
			vtkRenderWindow* t_coronal,
			vtkRenderWindow* t_axial);
		virtual void setInteractor(vtkRenderWindowInteractor* t_interactor) = 0;

		virtual void render() = 0;
		
	protected:
		vtkSmartPointer<vtkRenderWindow> m_renderWindows[3] = {};
		vtkRenderWindow* m_activeRenderWindow = nullptr;
		core::Series* m_series = {};
		core::Image* m_image = {};

		virtual void onRenderWindowAssigned(const vtkSmartPointer<vtkRenderWindow>&) {}
	};
}

