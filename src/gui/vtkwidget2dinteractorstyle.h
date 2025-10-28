/*
 * ------------------------------------------------------------------------------------
 *  File: vtkwidget2dinteractorstyle.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the interactor style used to control 2D slice widgets.
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

#include <vtkInteractorStyleImage.h>
#include "vtkwidget2d.h"

namespace isis::gui
{
	class vtkWidget2DInteractorStyle final : public vtkInteractorStyleImage
	{
	public:
		static vtkWidget2DInteractorStyle* New();
		vtkTypeMacro(vtkWidget2DInteractorStyle, vtkInteractorStyleImage);

		vtkWidget2DInteractorStyle() = default;
		~vtkWidget2DInteractorStyle() = default;

		//getters
		[[nodiscard]] int getCurrentImageIndex() const { return  m_currentImageIndex; }

		//setters
		void setWidget(vtkWidget2D* t_widget) { m_widget2D = t_widget; }
		void setSeries(core::Series* t_series) { m_series = t_series; }
		void setImage(core::Image* t_image) { m_image = t_image; }

		void changeImage(int& t_index);
		void updateOvelayImageNumber(const int& t_index) const;

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
		void Dolly() override;

	private:
                vtkWidget2D* m_widget2D = {};
                core::Series* m_series = {};
                core::Image* m_image = {};
                int m_currentImageIndex = {};
                bool m_leftScrolling = false;
                double m_scrollDragAccumulator = 0.0;

                void refreshImage() const;
                void updateOverlayWindowLevelApply() const;
                void updateOverlayHUValue() const;
                [[nodiscard]] core::Image* getNextImage() const;
		[[nodiscard]] core::Image* getPreviousImage() const;
	};
}

