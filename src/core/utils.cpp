/*
 * ------------------------------------------------------------------------------------
 *  File: utils.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements formatting helpers that normalize DICOM tag values for
 *      display within the viewer.
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

#include "utils.h"

void isis::core::Utils::processTagFormat(const DicomTag& t_tag, std::string& t_value)
{
	if (t_value.empty())
	{
		return;
	}
	switch (t_tag.packed())
	{
	case 524320: //study date
		t_value = getDateFormat(t_value);
		break;
	case 524338: //study time
		t_value = getTimeFormat(t_value);
		break;
	case 1048624: //birth date
		t_value = getDateFormat(t_value);
		break;
	case 1048640: //sex
		t_value = "[" + t_value + "]";
		break;
	case 1052688: //age
		if (t_value.length() < 3)
		{
			t_value = "Age: " + t_value;
			break;
		}
		if (t_value.at(3) == 'Y')
		{
			if (t_value.at(0) == '0')
			{
				t_value = "Age: " + t_value.substr(1, 2);
			}
			else t_value = "Age: " + t_value.substr(0, 3);
		}
		else
		{
			t_value = "Age: " + t_value.substr(1, 2) + " months";
		}
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
std::string isis::core::Utils::getDateFormat(const std::string& t_date)
{
	return t_date.substr(6, 2) + "/" +
		t_date.substr(4, 2) + "/" +
		t_date.substr(0, 4);
}

//-----------------------------------------------------------------------------
std::string isis::core::Utils::getTimeFormat(const std::string& t_time)
{
	return t_time.substr(0, 2) + ":" +
		t_time.substr(2, 2) + ":" +
		t_time.substr(4, 2);
}

