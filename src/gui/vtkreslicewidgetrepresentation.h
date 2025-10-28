/*
 * ------------------------------------------------------------------------------------
 *  File: vtkreslicewidgetrepresentation.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares VTK widget representation classes for drawing interactive reslice planes.
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
#include <vtkPointHandleRepresentation3D.h>
#include <vtkSmartPointer.h>
#include <vtkWidgetRepresentation.h>
#include "vtkresliceactor.h"

namespace isis::gui
{
	class vtkResliceWidgetRepresentation final : public vtkWidgetRepresentation
	{
	public:
		static vtkResliceWidgetRepresentation* New();
		vtkTypeMacro(vtkResliceWidgetRepresentation, vtkWidgetRepresentation);
		vtkResliceWidgetRepresentation();
		~vtkResliceWidgetRepresentation() = default;
		
		void instantiateHandleRepresentation();
		void StartWidgetInteraction(double eventPos[2]) override {};
		void WidgetInteraction(double newEventPos[2]) override {};
		void BuildRepresentation() override;
		void ReleaseGraphicsResources(vtkWindow* w) override;
		int RenderOverlay(vtkViewport* viewport) override;
		int RenderOpaqueGeometry(vtkViewport* viewport) override;
		int HasTranslucentPolygonalGeometry() override;

		void rotate(double t_angle);
		void setPlane(int t_plane);

		void setCursorPosition(double* t_position);
		void SetVisibility(int _arg) override;
		int GetVisibility() override;
		int ComputeInteractionState(int X, int Y, int modify) override;
		bool currentDisplaySegments(std::array<std::array<double, 4>, 2>& t_segments) const;

		[[nodiscard]] vtkSmartPointer<vtkResliceActor> getResliceActor() const { return m_cursorActor; }
		[[nodiscard]] vtkSmartPointer<vtkPointHandleRepresentation3D> getCenterMovementRepresentation() const {
			return m_centerMovementPointRepresentation;
		}
		enum InteractionState { outside = 0, mprCursor, mipCursor, handleCursor };

	private:
		vtkSmartPointer<vtkPointHandleRepresentation3D> m_handleRepresentation = {};
		vtkSmartPointer<vtkPointHandleRepresentation3D> m_centerMovementPointRepresentation = {};
		vtkSmartPointer<vtkResliceActor> m_cursorActor;
		bool m_cursorVisibility = false;
		double m_rotationAngle = 0;
		int m_plane = -1;

		void initializeRepresentation();
		void createCursor();
	};
}

