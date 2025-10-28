/*
 * ------------------------------------------------------------------------------------
 *  File: corerepository.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements CoreRepository operations that maintain the hierarchical
 *      patient, study, series, and image collections.
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

#include "corerepository.h"
#include <algorithm>


void isis::core::CoreRepository::addPatient(std::unique_ptr<Patient> t_patient)
{
	auto index = findPatient(t_patient.get());
	if (index == m_patients.size())
	{
		m_patients.emplace_back(std::move(t_patient));
		index = m_patients.size() - 1;
	}
	m_lastPatient = m_patients.at(index).get();
	m_lastPatient->setIndex(index);
}

//-----------------------------------------------------------------------------
void isis::core::CoreRepository::addStudy(std::unique_ptr<Study> t_study)
{
	t_study->setParentObject(m_lastPatient);
	m_lastStudy = m_lastPatient->addStudy(std::move(t_study));
}

//-----------------------------------------------------------------------------
void isis::core::CoreRepository::addSeries(std::unique_ptr<Series> t_series)
{
	if (!t_series)
	{
		m_lastSeries = nullptr;
		m_lastImage = nullptr;
		m_newSeries = false;
		m_newImage = false;
		return;
	}
	t_series->setParentObject(m_lastStudy);
	m_lastSeries = m_lastStudy->addSeries(std::move(t_series), m_newSeries);
}

//-----------------------------------------------------------------------------
void isis::core::CoreRepository::addImage(std::unique_ptr<Image> t_image)
{
	if (!t_image)
	{
		m_lastImage = nullptr;
		m_newImage = false;
		return;
	}
	t_image->setParentObject(m_lastSeries);
	m_lastImage = t_image->getIsMultiFrame()
		? m_lastSeries->addMultiFrameImage(std::move(t_image), m_newImage)
		: m_lastSeries->addSingleFrameImage(std::move(t_image), m_newImage);
}

//-----------------------------------------------------------------------------
void isis::core::CoreRepository::deletePatient(Patient* t_patient)
{
	const auto index = findPatient(t_patient);
	if (index != m_patients.size())
	{
		m_patients.erase(m_patients.begin() + index);
	}
}

//-----------------------------------------------------------------------------
void isis::core::CoreRepository::resetLastPatientData()
{
	m_lastPatient = nullptr;
	m_lastStudy = nullptr;
	m_lastSeries = nullptr;
	m_lastImage = nullptr;
	m_newSeries = false;
	m_newImage = false;
}

//-----------------------------------------------------------------------------
std::size_t isis::core::CoreRepository::findPatient(Patient* t_patient)
{
	const auto it = std::find_if(m_patients.begin(),
		m_patients.end(), [&t_patient](const std::unique_ptr<Patient>& patient)
	{
		return t_patient->getID() == patient->getID();
	});
	return  std::distance(m_patients.begin(), it);
}


