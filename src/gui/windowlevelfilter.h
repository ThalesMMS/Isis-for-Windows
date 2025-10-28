/*
 * ------------------------------------------------------------------------------------
 *  File: windowlevelfilter.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the WindowLevelFilter utility that controls brightness and contrast of slices.
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

#include <vtkSmartPointer.h>

class vtkImageMapToWindowLevelColors;
class vtkWindowLevelLookupTable;

namespace isis::gui
{
	class WindowLevelFilter
	{
	public:
		WindowLevelFilter();
		~WindowLevelFilter();

		[[nodiscard]] vtkImageMapToWindowLevelColors* getWindowLevelColors() const { return m_windowLevelColors; }
		[[nodiscard]] bool getAreColorsInverted() const { return m_invert; }

		void setWindowLevelColors(vtkImageMapToWindowLevelColors* t_filter);
		void setAreColorsInverted(bool t_flag);
		void setWindowWidthCenter(int t_width, int t_center);

	private:
		vtkImageMapToWindowLevelColors* m_windowLevelColors = nullptr;
		vtkSmartPointer<vtkWindowLevelLookupTable> m_lookupTable = {};
		bool m_invert = false;
	};
}

