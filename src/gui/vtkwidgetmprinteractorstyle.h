/*
 * ------------------------------------------------------------------------------------
 *  File: vtkwidgetmprinteractorstyle.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares a custom VTK interactor style tailored to multiplanar reconstruction views.
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

#include <vtkAxisActor2D.h>
#include <vtkImageResliceToColors.h>
#include <vtkInteractorStyleImage.h>
#include "utils.h"

namespace isis::gui
{
	class vtkWidgetMPR;

	class vtkWidgetMPRInteractorStyle final : public vtkInteractorStyleImage
	{
	public:
		static vtkWidgetMPRInteractorStyle* New();
		vtkTypeMacro(vtkWidgetMPRInteractorStyle, vtkInteractorStyleImage);
		vtkWidgetMPRInteractorStyle() = default;
		~vtkWidgetMPRInteractorStyle() = default;

		//getters
		[[nodiscard]] int getMax() const { return m_max; }
		[[nodiscard]] int getMin() const { return m_min; }
		[[nodiscard]] vtkWidgetMPR* getWidget() const { return m_widget; }
		[[nodiscard]] vtkAxisActor2D* getAxisActor2D() const { return m_axisActor; }
		[[nodiscard]] vtkImageResliceToColors* getImageReslice() const { return m_imageReslice; }

		//setters
		void setMinMax(const int& t_min, const int& t_max) { m_min = t_min; m_max = t_max; }
		void setWidget(vtkWidgetMPR* t_widget) { m_widget = t_widget; }
		void setAxisActor2D(vtkAxisActor2D* t_actor) { m_axisActor = t_actor; }
		void setImageReslice(vtkImageResliceToColors* t_imageReslice) { m_imageReslice = t_imageReslice; }
		
		void rescaleAxisActor();
		void moveToSlice(const int& t_number);

	protected:
		void OnMouseMove() override;
		void OnMouseWheelForward() override;
		void OnMouseWheelBackward() override;
		void OnLeftButtonDown() override;
		void OnLeftButtonUp() override;
		void OnRightButtonDown() override;
		void OnRightButtonUp() override;
		void OnMiddleButtonDown() override;
		void OnMiddleButtonUp() override;

	private:
		vtkWidgetMPR* m_widget = {};
		vtkAxisActor2D* m_axisActor = {};
		vtkImageResliceToColors* m_imageReslice = {};
		int m_max = 0;
		int m_min = 0;

		void startAction(const transformationType& t_action);
		void moveSlice(const int& t_delta);
	};
}

