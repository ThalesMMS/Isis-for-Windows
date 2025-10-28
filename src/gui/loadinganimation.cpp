/*
 * ------------------------------------------------------------------------------------
 *  File: loadinganimation.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements the animated indicator displayed while data is loading.
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

#include "loadinganimation.h"
#include <QString>
#include <QLoggingCategory>
#include <QShowEvent>
#include <QHideEvent>
#include <QMoveEvent>
#include <QResizeEvent>

Q_LOGGING_CATEGORY(lcLoadingAnimation, "isis.gui.loadinganimation")

isis::gui::LoadingAnimation::LoadingAnimation(QWidget* parent)
	: QWidget(parent)
{
	setObjectName(QStringLiteral("LoadingAnimationOverlay"));
	initView();
	qCDebug(lcLoadingAnimation) << "Constructed overlay" << this
		<< "parent" << parent
		<< "flags" << Qt::hex << static_cast<quint64>(windowFlags())
		<< "isWindow" << isWindow();
}

//-----------------------------------------------------------------------------
void isis::gui::LoadingAnimation::initView()
{
	m_ui.setupUi(this);
	setAttribute(Qt::WA_StyledBackground, true);
	setFocusPolicy(Qt::NoFocus);
	setStyleSheet(QStringLiteral(
		"#LoadingAnimation {"
		"background-color: rgba(16, 16, 16, 180);"
		"border-radius: 12px;"
		"}"));
	m_ui.label->setMovie(&m_movie);
	m_movie.setFileName(":/loading");
	m_movie.start();
	logGeometry("initView");
}

//-----------------------------------------------------------------------------
void isis::gui::LoadingAnimation::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	logGeometry("showEvent");
}

//-----------------------------------------------------------------------------
void isis::gui::LoadingAnimation::hideEvent(QHideEvent* event)
{
	logGeometry("hideEvent");
	QWidget::hideEvent(event);
}

//-----------------------------------------------------------------------------
void isis::gui::LoadingAnimation::moveEvent(QMoveEvent* event)
{
	QWidget::moveEvent(event);
	logGeometry("moveEvent");
}

//-----------------------------------------------------------------------------
void isis::gui::LoadingAnimation::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	logGeometry("resizeEvent");
}

//-----------------------------------------------------------------------------
void isis::gui::LoadingAnimation::logGeometry(const char* context) const
{
	const QPoint topLeftLocal = pos();
	const QPoint topLeftGlobal = mapToGlobal(QPoint(0, 0));
	qCDebug(lcLoadingAnimation) << context
		<< "widget" << this
		<< "visible" << isVisible()
		<< "flags" << Qt::hex << static_cast<quint64>(windowFlags())
		<< "isWindow" << isWindow()
		<< "localPos" << topLeftLocal
		<< "globalPos" << topLeftGlobal
		<< "size" << size();
}
