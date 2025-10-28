/*
 * ------------------------------------------------------------------------------------
 *  File: corneroverlay.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the CornerOverlay class responsible for drawing overlay text actors.
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

#include <map>
#include <string>
#include <vtkNew.h>
#include <vtkOpenGLTextActor.h>

namespace isis::gui
{
	class CornerOverlay
	{
	public:
		CornerOverlay() = default;
		~CornerOverlay() = default;

		//getters
		[[nodiscard]] int getNumberOfCorner() const { return m_numberOfCorner; }
		[[nodiscard]] vtkOpenGLTextActor* getTextActor();
		[[nodiscard]] std::string getOverlayFromInfo();

		//setters
		void setNumberOfCorner(const int& t_nr) { m_numberOfCorner = t_nr; }
		void setOverlayInfo(const std::string& t_key, const std::string& t_value) { m_overlaysInfo[t_key] = t_value; }
		void setTextActorProperty(vtkTextProperty* t_property) const { m_textActor->SetTextProperty(t_property); }

		void initTextActor() { m_textActor->SetInput(getOverlayFromInfo().c_str()); }
		void clearOverlaysInfo() { m_overlaysInfo.clear(); }

	private:
		std::map<std::string, std::string> m_overlaysInfo = {};
		int m_numberOfCorner = -1;
		vtkNew<vtkOpenGLTextActor> m_textActor;
	};
}

