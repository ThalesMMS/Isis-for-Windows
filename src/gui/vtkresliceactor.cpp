/*
 * ------------------------------------------------------------------------------------
 *  File: vtkresliceactor.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements the VTK actor setup for displaying resliced image textures.
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

#include "vtkresliceactor.h"
#include <vtkImageSlice.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkProperty.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkUnsignedCharArray.h>

vtkStandardNewMacro(isis::gui::vtkResliceActor);

void isis::gui::vtkResliceActor::createActor()
{
	m_appender = vtkSmartPointer<vtkAppendPolyData>::New();
	for (auto i = 0; i < 2; ++i)
	{
		m_cursorLines[i] = vtkSmartPointer<vtkLineSource>::New();
		m_appender->AddInputConnection(m_cursorLines[i]->GetOutputPort());
	}
	m_mapper =
		vtkSmartPointer<vtkPolyDataMapper>::New();
	m_mapper->SetInputConnection(m_appender->GetOutputPort());
	m_mapper->ScalarVisibilityOn();
	m_mapper->SetScalarModeToUsePointFieldData();
	m_mapper->SelectColorArray("Colors");
	m_actor =
		vtkSmartPointer<vtkActor>::New();
	m_actor->SetMapper(m_mapper);
	m_actor->GetProperty()->SetInterpolationToGouraud();
}

//-----------------------------------------------------------------------------
void isis::gui::vtkResliceActor::reset() const
{
	auto* const orientation = m_actor->GetOrientation();
	m_actor->RotateZ(-orientation[2]);
	m_actor->SetPosition(0, 0, 0);
}

//-----------------------------------------------------------------------------
void isis::gui::vtkResliceActor::setDisplaySize(const double* t_size)
{
	m_windowSize[0] = t_size[0];
	m_windowSize[1] = t_size[1];
	m_windowSize[2] = t_size[2];
}

//-----------------------------------------------------------------------------
void isis::gui::vtkResliceActor::setDisplayOriginPoint(const double* t_point)
{
	m_windowOrigin[0] = t_point[0];
	m_windowOrigin[1] = t_point[1];
	m_windowOrigin[2] = t_point[2];
}

//-----------------------------------------------------------------------------
void isis::gui::vtkResliceActor::setCenterPosition(const double* t_center)
{
	m_centerPointDisplayPosition[0] = t_center[0];
	m_centerPointDisplayPosition[1] = t_center[1];
	m_centerPointDisplayPosition[2] = t_center[2];
}

//-----------------------------------------------------------------------------
void isis::gui::vtkResliceActor::update()
{
	if (m_start == 0)
	{
		m_cursorLines[0]->SetPoint1(m_windowOrigin[0],
			m_centerPointDisplayPosition[1], 0.01);
		m_cursorLines[0]->SetPoint2(m_windowSize[0],
			m_centerPointDisplayPosition[1], 0.01);
		m_cursorLines[0]->Update();
		m_cursorLines[0]->GetOutput()->GetPointData()->AddArray(m_colors[1]);
		m_cursorLines[1]->SetPoint1(m_centerPointDisplayPosition[0],
			m_windowOrigin[1], 0.01);
		m_cursorLines[1]->SetPoint2(m_centerPointDisplayPosition[0],
			m_windowSize[1], 0.01);
		m_cursorLines[1]->Update();
		m_cursorLines[1]->GetOutput()->GetPointData()->AddArray(m_colors[0]);
		m_actor->SetScale(5);
		m_start = 1;
	}
	else
	{
		m_actor->SetPosition(
			m_centerPointDisplayPosition[0],
			m_centerPointDisplayPosition[1],
			0.01);
	}

	m_displaySegments[0] = {
		m_windowOrigin[0],
		m_centerPointDisplayPosition[1],
		m_windowSize[0],
		m_centerPointDisplayPosition[1]
	};
	m_displaySegments[1] = {
		m_centerPointDisplayPosition[0],
		m_windowOrigin[1],
		m_centerPointDisplayPosition[0],
		m_windowSize[1]
	};
	m_segmentsValid = true;
}

//-----------------------------------------------------------------------------
void isis::gui::vtkResliceActor::createColors(double* t_color1, double* t_color2)
{
	m_colors[0] =
		vtkSmartPointer<vtkUnsignedCharArray>::New();
	m_colors[0]->SetName("Colors");
	m_colors[0]->SetNumberOfComponents(3);
	m_colors[0]->SetNumberOfTuples(100);
	for (auto j = 0; j < 100; j++)
	{
		m_colors[0]->InsertTuple3(j, t_color1[0],
		                          t_color1[1], t_color1[2]);
	}
	m_colors[1] =
		vtkSmartPointer<vtkUnsignedCharArray>::New();
	m_colors[1]->SetName("Colors");
	m_colors[1]->SetNumberOfComponents(3);
	m_colors[1]->SetNumberOfTuples(100);
	for (auto j = 0; j < 100; j++)
	{
		m_colors[1]->InsertTuple3(j, t_color2[0],
		                          t_color2[1], t_color2[2]);
	}
}

