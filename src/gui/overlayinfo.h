/*
 * ------------------------------------------------------------------------------------
 *  File: overlayinfo.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the OverlayInfo data structure encapsulating key-value metadata shown in overlays.
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
#include <string>

namespace isis::gui
{
	class OverlayInfo
	{
	public:
		OverlayInfo() = default;
		~OverlayInfo() = default;

		//getters
		[[nodiscard]] std::string getTextBefore() const { return m_textBefore; }
		[[nodiscard]] std::string getTextAfter() const { return m_textAfter; }
		[[nodiscard]] unsigned int getTagKey() const { return m_tagKey; }
		[[nodiscard]] int getCorner() const { return m_corner; }

		//setters
		void setTextBefore(const std::string& t_text) { m_textBefore = t_text; }
		void setTextAfter(const std::string& t_text) { m_textAfter = t_text; }
		void setTagKey(const unsigned long& t_tagKey) { m_tagKey = t_tagKey; }
		void setCorner(const int& t_corner) { m_corner = t_corner; }

	private:
		std::string m_textAfter;
		std::string m_textBefore;
		unsigned int m_tagKey = 0;
		int m_corner = 0;
	};
}

