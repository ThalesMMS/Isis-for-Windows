/*
 * ------------------------------------------------------------------------------------
 *  File: vtkreslicetranslator.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements coordinate translation utilities for positioning reslice widgets.
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

#include "vtkreslicetranslator.h"
#include <vtkNew.h>
#include <vtkTransform.h>
#include "vtkMatrix4x4.h"

void isis::gui::vtkResliceTranslator::movePlaneX(vtkMatrix4x4* t_sourceMatrix,
                                                      vtkMatrix4x4* t_destinationMatrix1,
                                                      vtkMatrix4x4* t_destinationMatrix2, double* t_point)
{
	vtkNew<vtkTransform> transform;
	transform->SetMatrix(t_sourceMatrix);
	transform->Translate(t_point[0], t_point[1], 0);
	double center[3] =
	{
		transform->GetMatrix()->GetElement(0, 3),
		transform->GetMatrix()->GetElement(1, 3),
		transform->GetMatrix()->GetElement(2, 3)
	};
	t_destinationMatrix1->SetElement(0, 3, center[0]);
	t_destinationMatrix1->SetElement(1, 3, center[1]);
	t_destinationMatrix1->SetElement(2, 3, center[2]);
	t_destinationMatrix2->SetElement(0, 3, center[0]);
	t_destinationMatrix2->SetElement(1, 3, center[1]);
	t_destinationMatrix2->SetElement(2, 3, center[2]);
}

