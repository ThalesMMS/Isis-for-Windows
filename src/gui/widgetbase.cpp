/*
 * ------------------------------------------------------------------------------------
 *  File: widgetbase.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Provides base functionality for VTK-backed GUI widgets, including setup hooks and cleanup.
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

#include "widgetbase.h"
#include <QFocusEvent>
#include <QThread>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcWidgetBase, "isis.gui.widgetbase")

isis::gui::WidgetBase::WidgetBase(QWidget* t_parent)
	: QWidget(t_parent), m_tabWidget(t_parent)
{
	qCDebug(lcWidgetBase) << "Constructed" << metaObject()->className()
		<< "widget" << this
		<< "parent" << t_parent;
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetBase::stopLoadingAnimation()
{
	if(m_loadingAnimation)
	{
		qCDebug(lcWidgetBase) << "stopLoadingAnimation"
			<< "widget" << metaObject()->className()
			<< "overlay" << m_loadingAnimation.get();
		m_loadingAnimation->hide();
		m_loadingAnimation->close();
		m_loadingAnimation.reset();
	}
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetBase::setIndexes(const int& t_patientIndex, const int& t_studyIndex,
                                            const int& t_seriesIndex, const int& t_imageIndex)
{
	m_patientIndex = t_patientIndex;
	m_studyIndex = t_studyIndex;
	m_seriesIndex = t_seriesIndex;
	m_imageIndex = t_imageIndex;
	qCDebug(lcWidgetBase) << "setIndexes"
		<< "widget" << metaObject()->className()
		<< "patient" << m_patientIndex
		<< "study" << m_studyIndex
		<< "series" << m_seriesIndex
		<< "image" << m_imageIndex;
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetBase::focusInEvent(QFocusEvent* event)
{
	qCDebug(lcWidgetBase) << "focusInEvent"
		<< "widget" << metaObject()->className()
		<< "reason" << event->reason();
	m_tabWidget->setFocus(event->reason());
	QWidget::focusInEvent(event);
}
