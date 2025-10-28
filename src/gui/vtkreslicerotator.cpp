/*
 * ------------------------------------------------------------------------------------
 *  File: vtkreslicerotator.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements matrix computations used to rotate VTK reslice planes.
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

#include "vtkreslicerotator.h"
#include <vtkNew.h>
#include <vtkTransform.h>

void isis::gui::vtkResliceRotator::rotatePlane(vtkMatrix4x4* t_sourceMatrix,
                                                    vtkMatrix4x4* t_destinationMatrix, const double t_angle)
{
	vtkNew<vtkTransform> transform2;
	vtkNew<vtkTransform> destinationPlaneTransform;
	destinationPlaneTransform->SetMatrix(t_destinationMatrix);
	double defaultMatrix[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};
	double* destinationPlaneOrientation = destinationPlaneTransform->GetOrientation();
	const double xRotation = destinationPlaneOrientation[0];
	const double yRotation = destinationPlaneOrientation[1];
	const double zRotation = destinationPlaneOrientation[2];
	const double xCenter = t_destinationMatrix->GetElement(0, 3);
	const double yCenter = t_destinationMatrix->GetElement(1, 3);
	const double zCenter = t_destinationMatrix->GetElement(2, 3);
	transform2->SetMatrix(defaultMatrix);
	transform2->Translate(xCenter, yCenter, zCenter);
	transform2->RotateWXYZ(t_angle,
	                       t_sourceMatrix->GetElement(0, 2),
	                       t_sourceMatrix->GetElement(1, 2),
	                       t_sourceMatrix->GetElement(2, 2));
	transform2->RotateZ(zRotation);
	transform2->RotateX(xRotation);
	transform2->RotateY(yRotation);
	t_destinationMatrix->DeepCopy(transform2->GetMatrix());
}

