/*
 * ------------------------------------------------------------------------------------
 *  File: thumbnailswidget.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements the thumbnails panel, including population and selection handling.
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

#include "thumbnailswidget.h"
#include "patienttab.h"
#include <QTabBar>

isis::gui::ThumbnailsWidget::ThumbnailsWidget(QWidget* parent)
	: QWidget(parent)
{
	initView();
	initData();
}

//-----------------------------------------------------------------------------
void isis::gui::ThumbnailsWidget::initView()
{
	m_ui.setupUi(this);
}

//-----------------------------------------------------------------------------
void isis::gui::ThumbnailsWidget::initData()
{
	m_patientsTabs = new QTabWidget(this);
	m_patientsTabs->setTabPosition(QTabWidget::North);
	m_patientsTabs->setObjectName("ThumbnailsPatients");
	if (auto* const patientsTabBar = m_patientsTabs->tabBar())
	{
		patientsTabBar->setExpanding(false);
		patientsTabBar->setElideMode(Qt::ElideRight);
	}
	layout()->addWidget(m_patientsTabs);
}

//-----------------------------------------------------------------------------
void isis::gui::ThumbnailsWidget::resetData() const
{
	while (m_patientsTabs->count())
	{
		delete m_patientsTabs->widget(0);
	}
}

//-----------------------------------------------------------------------------
void isis::gui::ThumbnailsWidget::applyDarkPalette()
{
        for (int i = 0; i < m_patientsTabs->count(); ++i)
        {
                if (auto* const patientTab = qobject_cast<PatientTab*>(m_patientsTabs->widget(i)))
                {
                        patientTab->applyDarkPalette();
                }
        }
}

//-----------------------------------------------------------------------------
void isis::gui::ThumbnailsWidget::updateSeriesProgress(core::Series* t_series,
        int frameIndex,
        int frameCount) const
{
        if (!t_series || !m_patientsTabs)
        {
                return;
        }

        for (int i = 0; i < m_patientsTabs->count(); ++i)
        {
                if (auto* const patientTab = qobject_cast<PatientTab*>(m_patientsTabs->widget(i)))
                {
                        if (patientTab->updateSeriesProgress(t_series, frameIndex, frameCount))
                        {
                                return;
                        }
                }
        }
}

//-----------------------------------------------------------------------------
void isis::gui::ThumbnailsWidget::addThumbnail(core::Patient* t_patient, core::Study* t_study,
                                                    core::Series* t_series, core::Image* t_image) const
{
        if (!tryInsertExistingItem(t_patient, t_study, t_series, t_image))
        {
                insertNewItem(t_patient, t_study, t_series, t_image);
                const_cast<ThumbnailsWidget*>(this)->applyDarkPalette();
        }
        if (auto* const thumbnailsContainer = parentWidget())
        {
                thumbnailsContainer->setVisible(true);
        }
}

//-----------------------------------------------------------------------------
bool isis::gui::ThumbnailsWidget::tryInsertExistingItem(core::Patient* t_patient, core::Study* t_study,
                                                             core::Series* t_series, core::Image* t_image) const
{
	for (int i = 0; i < m_patientsTabs->count(); ++i)
	{
		auto* const patientTab =
			dynamic_cast<PatientTab*>(m_patientsTabs->widget(i));
		if (patientTab->getPatientID().toStdString() == t_patient->getID())
		{
			auto* studyTab =
				patientTab->getStudyTab(QString(t_study->getUID().c_str()));
			if (!studyTab)
			{
				studyTab = patientTab->addNewStudy(t_study);
			}
			studyTab->insertNewSeries(t_series, t_image);
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
void isis::gui::ThumbnailsWidget::insertNewItem(core::Patient* t_patient, core::Study* t_study,
                                                     core::Series* t_series, core::Image* t_image) const
{
	auto* newPatientTab = new PatientTab(m_patientsTabs);
	newPatientTab->setPatientID(t_patient->getID().c_str());
	newPatientTab->setPatientName(QString::fromLatin1(t_patient->getName().c_str()));
	auto* newStudyTab = newPatientTab->addNewStudy(t_study);
	newStudyTab->insertNewSeries(t_series, t_image);
	m_patientsTabs->addTab(newPatientTab, newPatientTab->getPatientName());
}

