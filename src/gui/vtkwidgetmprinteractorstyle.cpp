/*
 * ------------------------------------------------------------------------------------
 *  File: vtkwidgetmprinteractorstyle.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements interaction behaviour for multiplanar reconstruction VTK render windows.
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

#include "vtkwidgetmprinteractorstyle.h"
#include <vtkCallbackCommand.h>
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>
#include <vtkRenderWindowInteractor.h>
#include "vtkwidgetmpr.h"

vtkStandardNewMacro(isis::gui::vtkWidgetMPRInteractorStyle);

void isis::gui::vtkWidgetMPRInteractorStyle::rescaleAxisActor()
{
	auto* const bounds =
		m_imageReslice->GetOutput()->GetBounds();
	vtkNew<vtkCoordinate> newCoord;
	newCoord->SetCoordinateSystemToWorld();
	newCoord->SetValue(0, bounds[2], 0);
	const auto yMin =
		newCoord->GetComputedDisplayValue(GetCurrentRenderer())[1];
	newCoord->SetValue(0, bounds[3], 0);
	const auto yMax =
		newCoord->GetComputedDisplayValue(GetCurrentRenderer())[1];
	m_axisActor->SetPoint1(1, yMin);
	m_axisActor->SetPoint2(1, yMax);
	Interactor->Render();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetMPRInteractorStyle::moveToSlice(const int& t_number)
{
	moveSlice(t_number - CurrentImageNumber);
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetMPRInteractorStyle::OnMouseMove()
{
	const auto x =
		Interactor->GetEventPosition()[0] - Interactor->GetLastEventPosition()[0];
	const auto y =
		Interactor->GetEventPosition()[1] - Interactor->GetLastEventPosition()[1];
	switch (State)
	{
	case VTKIS_DOLLY:
		rescaleAxisActor();
		break;
	case VTKIS_WINDOW_LEVEL:
		m_widget->changeWindowLevel(Interactor->GetRenderWindow(), x, y);
		return;
	default:
		break;
	}
	vtkInteractorStyleImage::OnMouseMove();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetMPRInteractorStyle::OnMouseWheelForward()
{
	moveSlice(1);
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetMPRInteractorStyle::OnMouseWheelBackward()
{
	moveSlice(-1);
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetMPRInteractorStyle::OnLeftButtonDown()
{
	startAction(transformationType::windowLevel);
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetMPRInteractorStyle::OnLeftButtonUp()
{
	auto* const windows =
		m_widget->getRenderWindows();
	for (auto i = 0; i < 3; ++i)
	{
		if (windows[i] != Interactor->GetRenderWindow())
		{
			m_widget->changeWindowLevel(windows[i],
			                            0, 0);
		}
	}
	StopState();
	vtkInteractorStyleImage::OnLeftButtonUp();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetMPRInteractorStyle::OnRightButtonDown()
{
	startAction(transformationType::zoom);
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetMPRInteractorStyle::OnRightButtonUp()
{
	StopState();
	vtkInteractorStyleImage::OnRightButtonUp();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetMPRInteractorStyle::OnMiddleButtonDown()
{
	startAction(transformationType::pan);
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetMPRInteractorStyle::OnMiddleButtonUp()
{
	StopState();
	vtkInteractorStyleImage::OnMiddleButtonUp();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetMPRInteractorStyle::startAction(const transformationType& t_action)
{
	const int x = Interactor->GetEventPosition()[0];
	const int y = Interactor->GetEventPosition()[1];
	FindPokedRenderer(x, y);
	if (!CurrentRenderer)
	{
		return;
	}
	GrabFocus(EventCallbackCommand);
	switch (t_action)
	{
	case transformationType::zoom:
		StartDolly();
		break;
	case transformationType::pan:
		StartPan();
		break;
	case transformationType::windowLevel:
		StartWindowLevel();
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
void isis::gui::vtkWidgetMPRInteractorStyle::moveSlice(const int& t_delta)
{
	const int nextSlice =
		CurrentImageNumber + t_delta;
	if (nextSlice > m_max || nextSlice < m_min)
	{
		return;
	}
	const double sliceSpacing =
		m_imageReslice->GetOutput()->GetSpacing()[2];
	vtkMatrix4x4* matrix =
		m_imageReslice->GetResliceAxes();
	double point[4];
	double center[4];
	point[0] = 0.0;
	point[1] = 0.0;
	point[2] = sliceSpacing * t_delta;
	point[3] = 1.0;
	matrix->MultiplyPoint(point, center);
	matrix->SetElement(0, 3, center[0]);
	matrix->SetElement(1, 3, center[1]);
	matrix->SetElement(2, 3, center[2]);
	SetCurrentImageNumber(nextSlice);
	m_imageReslice->Update();
	Interactor->Render();
}

