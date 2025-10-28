/*
 * ------------------------------------------------------------------------------------
 *  File: vtkwidget3dinteractorstyle.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the custom interactor style supporting 3D volume navigation controls.
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

#include <vtkInteractorStyleTrackballCamera.h>
#include "transferfunction.h"

namespace isis::gui
{
	class vtkWidget3D;

	class vtkWidget3DInteractorStyle final : public vtkInteractorStyleTrackballCamera
	{
	public:
		static vtkWidget3DInteractorStyle* New();
		vtkTypeMacro(vtkWidget3DInteractorStyle, vtkInteractorStyleTrackballCamera);
		vtkWidget3DInteractorStyle() = default;
		~vtkWidget3DInteractorStyle() = default;

		//getters
		[[nodiscard]] vtkWidget3D* getWidget() const { return  m_widget3D; }
		[[nodiscard]] TransferFunction* getTransferFunction() const { return m_transferFunction; }

		//setters
		void setWidget(vtkWidget3D* t_widget) { m_widget3D = t_widget; }
		void setTransferFunction(TransferFunction* t_function) { m_transferFunction = t_function; }

	protected:
		void OnMouseMove() override;

	private:
		vtkWidget3D* m_widget3D = {};
		TransferFunction* m_transferFunction = {};
	};
}

