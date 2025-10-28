/*
 * ------------------------------------------------------------------------------------
 *  File: layoutmenu.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Creates the GUI layout menu and wires its actions to the widgets controller.
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

#include "layoutmenu.h"

#include "guiframe.h"


isis::gui::LayoutMenu::LayoutMenu(QWidget* parent)
	: QMenu(parent)
{
	initView();
	createConnections(parent);
}

//-----------------------------------------------------------------------------
void isis::gui::LayoutMenu::initView()
{
        m_ui.setupUi(this);
        setTitle("Layouts");
        if (m_ui.actionOne)
        {
                setActiveAction(m_ui.actionOne);
                if (m_ui.actionOne->isCheckable())
                {
                        m_ui.actionOne->setChecked(true);
                }
        }
}

//-----------------------------------------------------------------------------
void isis::gui::LayoutMenu::createConnections(QWidget* parent) const
{
	Q_UNUSED(connect(this, &QMenu::triggered,
		this, &LayoutMenu::onActionTriggered));
	auto* const receiver = dynamic_cast<GUIFrame*>(parent);
	Q_UNUSED(connect(this, &LayoutMenu::changeLayout,
		receiver, &GUIFrame::onChangeLayout));
}

//-----------------------------------------------------------------------------s
void isis::gui::LayoutMenu::onActionTriggered(QAction* t_action)
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	const auto action = t_action->objectName();
	if (action == "actionOne")
	{
		emit changeLayout(WidgetsContainer::layouts::one);
	}
	else if (action == "actionTwoInRowOneBottom")
	{
		emit changeLayout(WidgetsContainer::layouts::twoRowOneBottom);
	}
	else if (action == "actionTwoInColumnOneRight")
	{
		emit changeLayout(WidgetsContainer::layouts::twoColumnOneRight);
	}
	else if (action == "actionThreeInRowOneBottom")
	{
		emit changeLayout(WidgetsContainer::layouts::threeRowOneBottom);
	}
	else if (action == "actionThreeInColumnOneRight")
	{
		emit changeLayout(WidgetsContainer::layouts::threeColumnOneRight);
	}
}

