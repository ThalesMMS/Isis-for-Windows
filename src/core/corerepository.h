/*
 * ------------------------------------------------------------------------------------
 *  File: corerepository.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the CoreRepository container that stores patients, studies,
 *      series, and images with convenience accessors for the UI layer.
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

#include <memory>
#include "patient.h"
#include <vector>

namespace isis::core
{
	class CoreRepository
	{
	public:
		CoreRepository() = default;
		~CoreRepository() = default;

		//getters
		[[nodiscard]] std::vector<std::unique_ptr<Patient>>& getPatients() { return m_patients; }
		[[nodiscard]] Patient* getLastPatient() const { return m_lastPatient; }
		[[nodiscard]] Study* getLastStudy() const { return m_lastStudy; }
		[[nodiscard]] Series* getLastSeries() const { return m_lastSeries; }
		[[nodiscard]] Image* getLastImage()  const { return m_lastImage; }
		[[nodiscard]] bool newSeriesAdded() const { return m_newSeries; }
		[[nodiscard]] bool newImageAdded() const { return m_newImage; }

		//add
		void addPatient(std::unique_ptr<Patient> t_patient);
		void addStudy(std::unique_ptr<Study> t_study);
		void addSeries(std::unique_ptr<Series> t_series);
		void addImage(std::unique_ptr<Image> t_image);

		//delete
		void deletePatient(Patient* t_patient);
		void deleteAllPatients() { m_patients.clear(); }
		void resetLastPatientData();

		//find
		[[nodiscard]] std::size_t findPatient(Patient* t_patient);

	private:
		std::vector<std::unique_ptr<Patient>> m_patients = {};
		Patient* m_lastPatient = {};
		Study* m_lastStudy = {};
		Series* m_lastSeries = {};
		Image* m_lastImage = {};
		bool m_newSeries = false;
		bool m_newImage = false;
	};
}

