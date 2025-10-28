/*
 * ------------------------------------------------------------------------------------
 *  File: vtkboxwidget3dcallbackcpp.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements callback logic for the 3D VTK box widget controlling cropping volumes.
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

#include <vtkAbstractVolumeMapper.h>
#include <vtkBoxRepresentation.h>
#include <vtkBoxWidget2.h>
#include <vtkObjectFactory.h>
#include "vtkboxwidget3dcallback.h"

vtkStandardNewMacro(isis::gui::vtkBoxWidget3DCallback);

void isis::gui::vtkBoxWidget3DCallback::Execute(vtkObject* caller, [[maybe_unused]] unsigned long eventId, [[maybe_unused]] void* callData)
{
	auto* const boxWidget = 
		vtkBoxWidget2::SafeDownCast(caller);
	auto* const  boxRepresentation =
		vtkBoxRepresentation::SafeDownCast(boxWidget->GetRepresentation());
	boxRepresentation->SetInsideOut(1);
	boxRepresentation->GetPlanes(m_planes);
	m_volume->GetMapper()->SetClippingPlanes(m_planes);
}

