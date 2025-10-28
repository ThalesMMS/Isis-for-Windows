/*
 * ------------------------------------------------------------------------------------
 *  File: study.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Defines the Study aggregate that groups Series objects and captures
 *      identifying metadata from a DICOM study.
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
#include "series.h"

namespace isis::core
{
	class Patient;

	class Study
	{
	public:
		Study() = default;
		~Study() = default;

		//getters
		[[nodiscard]] Patient* getParentObject() const { return m_parent; }
		[[nodiscard]] std::string getUID() const { return m_uid; }
		[[nodiscard]] std::string getID() const { return m_id; }
		[[nodiscard]] std::string getDescription() const { return m_desctiption; }
		[[nodiscard]] std::string getDate() const { return m_date; }
		std::vector<std::unique_ptr<Series>>& getSeries() { return m_series; }
		[[nodiscard]] int getIndex() const { return m_index; }

		//setters
		void setParentObject(Patient* t_parent) { m_parent = t_parent; }
		void setUID(const std::string& t_uid) { m_uid = t_uid; }
		void setID(const std::string& t_id) { m_id = t_id; }
		void setDescription(const std::string& t_description) { m_desctiption = t_description; }
		void setDate(const std::string& t_date) { m_date = t_date; }
		void setIndex(const int& t_index) { m_index = t_index; }

		[[nodiscard]] Series* addSeries(std::unique_ptr<Series> t_series, bool& t_newSeries);

		//find
		[[nodiscard]] std::size_t findSeriesIndex(Series* t_series);


	private:
		std::size_t m_index = -1;
		Patient* m_parent = {};
		std::string m_uid = {};
		std::string m_id = {};
		std::string m_desctiption = {};
		std::string m_date = {};
		std::vector<std::unique_ptr<Series>> m_series = {};
	};
}

