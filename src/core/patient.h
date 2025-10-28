/*
 * ------------------------------------------------------------------------------------
 *  File: patient.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Defines the Patient entity that tracks demographics and owns the
 *      collection of associated studies.
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
#include <vector>
#include "study.h"

namespace isis::core
{
	class Patient
	{
	public:
		Patient() = default;
		~Patient() = default;

		//getters
		[[nodiscard]] std::string getID() const { return m_id; }
		[[nodiscard]] std::string getName() const { return m_name; }
		[[nodiscard]] int getAge() const { return m_age; }
		[[nodiscard]] std::string getBirthDate() const { return m_birthDate; }
		[[nodiscard]] int getIndex() const { return m_index; }
		[[nodiscard]] std::vector<std::unique_ptr<Study>>& getStudies() { return m_studies; }

		//setters
		void setID(const std::string& t_id) { m_id = t_id; }
		void setName(const std::string& t_fullName) { m_name = t_fullName; }
		void setAge(const int& t_age) { m_age = t_age; }
		void setBirthDate(const std::string& t_birthDate) { m_birthDate = t_birthDate; }
		void setIndex(const int& t_index) { m_index = t_index; }


		[[nodiscard]] Study* addStudy(std::unique_ptr<Study> t_study);

		//find
		[[nodiscard]] std::size_t findStudyIndex(Study* t_study);

	private:
		std::size_t m_index = -1;
		std::string m_id = {};
		std::string m_name = {};
		int m_age = {};
		std::string m_birthDate = {};
		std::vector<std::unique_ptr<Study>> m_studies = {};

	};
}

