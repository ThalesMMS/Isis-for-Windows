/*
 * ------------------------------------------------------------------------------------
 *  File: study.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements Study collection management for creating and locating
 *      Series instances within a patient.
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

#include "study.h"
#include <algorithm>

isis::core::Series* isis::core::Study::addSeries(std::unique_ptr<Series> t_series, bool& t_newSeries)
{
	auto index = findSeriesIndex(t_series.get());
	t_newSeries = false;
	if (index == m_series.size())
	{
		m_series.emplace_back(std::move(t_series));
		index = m_series.size() - 1;
		t_newSeries = true;
	}
	m_series.at(index)->setIndex(index);
	return m_series.at(index).get();
}

//-----------------------------------------------------------------------------
std::size_t isis::core::Study::findSeriesIndex(Series* t_series)
{
	const auto it = std::find_if(m_series.begin(),
		m_series.end(), [&t_series](const std::unique_ptr<Series>& series)
		{
			return t_series->getUID() == series->getUID();
		});
	return std::distance(m_series.begin(), it);
}

