/*
 * ------------------------------------------------------------------------------------
 *  File: vtkresliceactor.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the vtkResliceActor class that renders resliced image data in VTK scenes.
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
#include <vtkActor.h>
#include <vtkActor2D.h>
#include <vtkAppendPolyData.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkTransformFilter.h>

namespace isis::gui
{
	class vtkResliceActor final : public vtkObject
	{
	public:
		static vtkResliceActor* New();
		vtkTypeMacro(vtkResliceActor, vtkObject);
		vtkResliceActor() { createActor(); }
		~vtkResliceActor() = default;

		//getters
		[[nodiscard]] vtkActor* getActor() const { return m_actor; }
		[[nodiscard]] vtkMTimeType getLineTime() const { return m_cursorLines[0]->GetMTime(); }

		//setters
		void setCameraDistance(const double t_distance) { m_cameraDistance = t_distance; }
		void setDisplaySize(const double* t_size);
		void setDisplayOriginPoint(const double* t_point);
		void setCenterPosition(const double* t_center);

		void createActor();
		void update();
		void reset() const;
		void createColors(double* t_color1, double* t_color2);
		[[nodiscard]] bool hasDisplaySegments() const { return m_segmentsValid; }
		[[nodiscard]] std::array<std::array<double, 4>, 2> displaySegments() const { return m_displaySegments; }

	private:
		vtkSmartPointer<vtkAppendPolyData> m_appender = {};
		vtkSmartPointer<vtkActor> m_actor = {};
		vtkSmartPointer<vtkTransformFilter> m_filter = {};
		vtkSmartPointer<vtkLineSource> m_cursorLines[4] = {};
		vtkSmartPointer<vtkUnsignedCharArray> m_colors[3] = {};
		vtkSmartPointer<vtkPolyDataMapper> m_mapper = {};
		double m_windowSize[3] = {};
		double m_windowOrigin[3] = {};
		double m_centerPointDisplayPosition[3] = {};
		double m_cameraDistance = 0;
		int m_start = 0;
		std::array<std::array<double, 4>, 2> m_displaySegments = {};
		bool m_segmentsValid = false;
	};
}
