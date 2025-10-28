/*
 * ------------------------------------------------------------------------------------
 *  File: vtkreslicewidgetrepresentation.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements the VTK reslice widget representation that renders plane outlines and glyphs.
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

#include "vtkreslicewidgetrepresentation.h"
#include <vtkCamera.h>
#include <vtkCellPicker.h>
#include <vtkCoordinate.h>
#include <vtkEllipseArcSource.h>
#include <vtkHandleRepresentation.h>
#include <vtkObjectFactory.h>
#include <vtkPointHandleRepresentation3D.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

vtkStandardNewMacro(isis::gui::vtkResliceWidgetRepresentation);

isis::gui::vtkResliceWidgetRepresentation::vtkResliceWidgetRepresentation()
{
	initializeRepresentation();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkResliceWidgetRepresentation::initializeRepresentation()
{
	m_handleRepresentation = vtkSmartPointer<vtkPointHandleRepresentation3D>::New();
	m_handleRepresentation->SetSmoothMotion(1);
	m_handleRepresentation->SetHandleSize(0);
	m_handleRepresentation->SetTolerance(15);
	m_handleRepresentation->AllOff();
	m_handleRepresentation->TranslationModeOn();
	createCursor();
	instantiateHandleRepresentation();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkResliceWidgetRepresentation::createCursor()
{
	m_cursorActor = vtkSmartPointer<vtkResliceActor>::New();
	m_cursorVisibility = true;
}

//-----------------------------------------------------------------------------
void isis::gui::vtkResliceWidgetRepresentation::SetVisibility(const int _arg)
{
	m_cursorVisibility = static_cast<bool>(_arg);
}

//-----------------------------------------------------------------------------
int isis::gui::vtkResliceWidgetRepresentation::GetVisibility()
{
	return static_cast<int>(m_cursorVisibility);
}

//-----------------------------------------------------------------------------
int isis::gui::vtkResliceWidgetRepresentation::ComputeInteractionState(
	const int X, const int Y, int modify)
{
	if (m_centerMovementPointRepresentation->ComputeInteractionState(X, Y)
		!= vtkHandleRepresentation::Outside)
	{
		return handleCursor;
	};
	vtkNew<vtkCellPicker> picker;
	picker->SetTolerance(0.01);
	picker->InitializePickList();
	picker->PickFromListOn();
	picker->AddPickList(m_cursorActor->getActor());
	if (picker->Pick(X, Y, 0, Renderer))
	{
		return mprCursor;
	}
	return outside;
}

//-----------------------------------------------------------------------------
void isis::gui::vtkResliceWidgetRepresentation::instantiateHandleRepresentation()
{
	if (!m_centerMovementPointRepresentation)
	{
		m_centerMovementPointRepresentation =
			m_handleRepresentation->NewInstance();
		m_centerMovementPointRepresentation->
			ShallowCopy(m_handleRepresentation);
	}
}

//-----------------------------------------------------------------------------
void isis::gui::vtkResliceWidgetRepresentation::BuildRepresentation()
{
	if (GetMTime() > BuildTime ||
		getCenterMovementRepresentation()->GetMTime() > BuildTime ||
		Renderer && Renderer->GetVTKWindow() &&
		Renderer->GetVTKWindow()->GetMTime() > BuildTime)
	{
		double centerPos[3];
		m_centerMovementPointRepresentation->GetWorldPosition(centerPos);
		Renderer->GetVTKWindow()->GetSize();
		m_cursorActor->setCameraDistance(
			Renderer->GetActiveCamera()->GetDistance());
		vtkNew<vtkCoordinate> coord;
		coord->SetCoordinateSystemToDisplay();
		coord->SetValue(
			Renderer->GetVTKWindow()->GetSize()[0],
			Renderer->GetVTKWindow()->GetSize()[1], 0);
		double* size = coord->GetComputedWorldValue(Renderer);
		m_cursorActor->setCenterPosition(centerPos);
		m_cursorActor->setDisplaySize(size);
		coord->SetValue(0, 0);
		double* origin = coord->GetComputedWorldValue(Renderer);
		m_cursorActor->setDisplayOriginPoint(origin);
		m_cursorActor->update();
		BuildTime.Modified();
	}
}

//-----------------------------------------------------------------------------
void isis::gui::vtkResliceWidgetRepresentation::ReleaseGraphicsResources(
	vtkWindow* w)
{
	if (m_cursorActor && m_cursorVisibility)
	{
		m_cursorActor->getActor()->ReleaseGraphicsResources(w);
	}
}

//-----------------------------------------------------------------------------
int isis::gui::vtkResliceWidgetRepresentation::RenderOverlay(
	vtkViewport* viewport)
{
	int count = 0;
	if (m_cursorActor->getActor() && m_cursorVisibility)
	{
		count += m_cursorActor->getActor()->RenderOverlay(viewport);
	}
	return count;
}

//-----------------------------------------------------------------------------
int isis::gui::vtkResliceWidgetRepresentation::RenderOpaqueGeometry(
	vtkViewport* viewport)
{
	if (m_cursorActor->getActor() &&
		m_cursorActor->getActor()->GetVisibility() &&
		m_cursorVisibility)
	{
		m_cursorActor->getActor()->RenderOpaqueGeometry(viewport);
	}
	else
	{
		return 0;
	}
	return 1;
}

//-----------------------------------------------------------------------------
int isis::gui::vtkResliceWidgetRepresentation::HasTranslucentPolygonalGeometry()
{
	BuildRepresentation();
	return m_cursorActor->getActor()
		       ? m_cursorActor->getActor()->HasTranslucentPolygonalGeometry()
		       : 0;
}

bool isis::gui::vtkResliceWidgetRepresentation::currentDisplaySegments(
	std::array<std::array<double, 4>, 2>& t_segments) const
{
	if (!m_cursorVisibility || !m_cursorActor || !m_cursorActor->hasDisplaySegments())
	{
		return false;
	}
	t_segments = m_cursorActor->displaySegments();
	return true;
}

//-----------------------------------------------------------------------------
void isis::gui::vtkResliceWidgetRepresentation::rotate(const double t_angle)
{
	m_rotationAngle += t_angle;
	m_cursorActor->getActor()->RotateZ(vtkMath::DegreesFromRadians(t_angle));
}

//-----------------------------------------------------------------------------
void isis::gui::vtkResliceWidgetRepresentation::setPlane(const int t_plane)
{
	m_plane = t_plane;
	double verticalColor[3] = {3, 218, 198};
	double horizontalColor[3] = {3, 218, 198};
	if (m_cursorActor)
	{
		m_cursorActor->createColors(verticalColor, horizontalColor);
	}
}

//-----------------------------------------------------------------------------
void isis::gui::vtkResliceWidgetRepresentation::setCursorPosition(double* t_position)
{
	m_centerMovementPointRepresentation->SetWorldPosition(t_position);
	BuildRepresentation();
}

