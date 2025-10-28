/*
 * ------------------------------------------------------------------------------------
 *  File: patienttab.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements the PatientTab UI, wiring patient selection to study and series displays.
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

#include "patienttab.h"
#include <QTabBar>

isis::gui::PatientTab::PatientTab(QWidget* parent)
	: QTabWidget(parent)
{
	initView();
}

//-----------------------------------------------------------------------------
isis::gui::PatientTab::~PatientTab()
{
	for (int i = 0; i < count(); i++)
	{
		auto* const item = dynamic_cast<StudyList*>(widget(i));
		const auto futures = item->getFutures();
		for (auto future : futures)
		{
			future.waitForFinished();
		}
	}
}

//-----------------------------------------------------------------------------
void isis::gui::PatientTab::initView()
{
	m_ui.setupUi(this);
	setTabPosition(North);
	if (auto* const studiesTabBar = tabBar())
	{
		studiesTabBar->setExpanding(false);
		studiesTabBar->setElideMode(Qt::ElideRight);
	}
}

//-----------------------------------------------------------------------------
isis::gui::StudyList* isis::gui::PatientTab::getStudyTab(const QString& t_studyuid) const
{
	for (int i = 0; i < count(); i++)
	{
		auto* const item = dynamic_cast<StudyList*>(widget(i));
		if (item->getStudy()->getUID() == t_studyuid.toStdString())
		{
			return item;
		}
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
isis::gui::StudyList* isis::gui::PatientTab::addNewStudy(core::Study* t_study)
{
	auto* list = new StudyList(this);
	list->setViewMode(QListWidget::IconMode);
	list->setIconSize(QSize(140, 140));
	list->setPatient(t_study->getParentObject());
	list->setStudy(t_study);
        list->applyDarkPalette();
	addTab(list, QString::fromLatin1(t_study->getDescription().c_str()));
	return list;
}

//-----------------------------------------------------------------------------
bool isis::gui::PatientTab::updateSeriesProgress(core::Series* t_series, int frameIndex, int frameCount)
{
        if (!t_series)
        {
                return false;
        }

        for (int i = 0; i < count(); ++i)
        {
                auto* const list = qobject_cast<StudyList*>(widget(i));
                if (!list)
                {
                        continue;
                }
                if (list->updateSeriesProgress(t_series, frameIndex, frameCount))
                {
                        return true;
                }
        }
        return false;
}

//-----------------------------------------------------------------------------
void isis::gui::PatientTab::applyDarkPalette()
{
        for (int i = 0; i < count(); ++i)
        {
                if (auto* const list = qobject_cast<StudyList*>(widget(i)))
                {
                        list->applyDarkPalette();
                }
        }
}

