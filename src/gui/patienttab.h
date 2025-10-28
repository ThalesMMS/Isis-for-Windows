/*
 * ------------------------------------------------------------------------------------
 *  File: patienttab.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the PatientTab widget presenting patients and their associated studies.
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

#include <QTabWidget>
#include "studylist.h"
#include "ui_patienttab.h"

namespace isis::core
{
	class Study;
        class Series;
}

namespace isis::gui
{
	class PatientTab final : public QTabWidget
	{
	Q_OBJECT

	public:
		explicit PatientTab(QWidget* parent = Q_NULLPTR);
		~PatientTab();

		//getters
		[[nodiscard]] QString getPatientName() const { return m_patientName; }
		[[nodiscard]] QString getPatientID() const { return m_patientid; }
		[[nodiscard]] StudyList* getStudyTab (const QString& t_studyuid) const;

		//setters
		void setPatientName(const QString& t_patientName) { m_patientName = t_patientName; }
		void setPatientID(const QString& t_patientid) { m_patientid = t_patientid; }

		StudyList* addNewStudy(core::Study* t_study);
                bool updateSeriesProgress(core::Series* t_series, int frameIndex, int frameCount);
                void applyDarkPalette();


	private:
		Ui::PatientTab m_ui = {};
		QString m_patientName = {};
		QString m_patientid = {};

		void initView();
	};
}

