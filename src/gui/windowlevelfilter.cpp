/*
 * ------------------------------------------------------------------------------------
 *  File: windowlevelfilter.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements window/level adjustments applied to VTK image actors based on viewer settings.
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

#include "windowlevelfilter.h"

#include <vtkImageMapToWindowLevelColors.h>
#include <vtkWindowLevelLookupTable.h>

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcWindowLevelFilter, "isis.gui.windowlevelfilter")

isis::gui::WindowLevelFilter::WindowLevelFilter()
{
	m_lookupTable = vtkSmartPointer<vtkWindowLevelLookupTable>::New();
	if (m_lookupTable)
	{
		m_lookupTable->SetWindow(1.0);
		m_lookupTable->SetLevel(0.0);
		m_lookupTable->SetInverseVideo(false);
		m_lookupTable->Build();
		m_lookupTable->BuildSpecialColors();
	}
}

isis::gui::WindowLevelFilter::~WindowLevelFilter() = default;

void isis::gui::WindowLevelFilter::setWindowLevelColors(vtkImageMapToWindowLevelColors* t_filter)
{
	m_windowLevelColors = t_filter;
	if (m_windowLevelColors && m_lookupTable)
	{
		m_windowLevelColors->SetLookupTable(m_lookupTable);
	}
}

void isis::gui::WindowLevelFilter::setAreColorsInverted(const bool t_flag)
{
	m_invert = t_flag;
	if (m_lookupTable)
	{
		m_lookupTable->SetInverseVideo(m_invert);
		m_lookupTable->Build();
	}
	if (m_windowLevelColors)
	{
		m_windowLevelColors->Modified();
	}
}

void isis::gui::WindowLevelFilter::setWindowWidthCenter(int t_width, int t_center)
{
	if (!m_windowLevelColors)
	{
		qWarning()
			<< "[WindowLevelFilter] Missing window-level filter while setting WL."
			<< "Requested width:" << t_width
			<< "center:" << t_center;
		return;
	}

	const int safeWidth = (t_width == 0) ? 1 : t_width;

	if (!m_lookupTable)
	{
		m_lookupTable = vtkSmartPointer<vtkWindowLevelLookupTable>::New();
	}
	if (!m_lookupTable)
	{
		qWarning()
			<< "[WindowLevelFilter] Failed to allocate lookup table."
			<< "Requested width:" << safeWidth
			<< "center:" << t_center;
		return;
	}

	m_lookupTable->SetInverseVideo(m_invert);
	m_lookupTable->SetWindow(static_cast<double>(safeWidth));
	m_lookupTable->SetLevel(static_cast<double>(t_center));
	m_lookupTable->Build();
	m_lookupTable->BuildSpecialColors();

	m_windowLevelColors->SetWindow(static_cast<double>(safeWidth));
	m_windowLevelColors->SetLevel(static_cast<double>(t_center));
	m_windowLevelColors->SetLookupTable(m_lookupTable);
	m_windowLevelColors->Update();

	qInfo()
		<< "[WindowLevelFilter] Applied window/level."
		<< "Width:" << safeWidth
		<< "Center:" << t_center
		<< "Invert:" << m_invert;
}

