/*
 * ------------------------------------------------------------------------------------
 *  File: patient.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements Patient helpers for adding studies and resolving their
 *      positions within the patient record.
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

#include "patient.h"
#include <algorithm>

isis::core::Study* isis::core::Patient::addStudy(std::unique_ptr<Study> t_study)
{
	auto index = findStudyIndex(t_study.get());
	if(index == m_studies.size())
	{
		m_studies.emplace_back(std::move(t_study));
		index = m_studies.size() - 1;
	}
	m_studies.at(index)->setIndex(index);
	return m_studies.at(index).get();
}

//-----------------------------------------------------------------------------
std::size_t isis::core::Patient::findStudyIndex(Study* t_study)
{
	const auto it = std::find_if(m_studies.begin(),
		m_studies.end(), [&t_study](const std::unique_ptr<Study>& study)
	{
		return t_study->getUID() == study->getUID();
	});
	return  std::distance(m_studies.begin(), it);
}

