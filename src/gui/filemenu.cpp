/*
 * ------------------------------------------------------------------------------------
 *  File: filemenu.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements the File menu actions for importing data and managing application commands.
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

#include "filemenu.h"
#include "guiframe.h"

isis::gui::FileMenu::FileMenu(QWidget *parent)
	: QMenu(parent)
{
	initView();
	createConnections(parent);
}

//-----------------------------------------------------------------------------
void isis::gui::FileMenu::onActionTriggered(QAction* t_action)
{
	const auto action = t_action->objectName();
	if(action == "actionOpenFile")
	{
		emit openFile();
	}
	else if(action == "actionOpenFolder")
	{
		emit openFolder();
	}
        else if(action == "actionCloseAllPatients")
        {
                emit closeAllPatients();
        }
        else if(action == "actionRestoreThumbnailsPanel")
        {
                emit restoreThumbnailsPanel();
        }
        else if(action == "actionManagePeers")
        {
                emit managePeers();
        }
        else if(action == "actionQueryRetrieve")
        {
                emit queryRetrieve();
        }
        else if(action == "actionSendToPacs")
        {
                emit sendToPacs();
        }
        else if(action == "actionDimseEcho")
        {
                emit dimseEcho();
        }
}

//-----------------------------------------------------------------------------
void isis::gui::FileMenu::initView()
{
	m_ui.setupUi(this);
}

//-----------------------------------------------------------------------------
void isis::gui::FileMenu::createConnections(QWidget* parent) const
{
	Q_UNUSED(connect(this, &QMenu::triggered,
		this, &FileMenu::onActionTriggered));
	auto* const receiver = dynamic_cast<GUIFrame*>(parent);
	Q_UNUSED(connect(this, &FileMenu::openFile,
		receiver, &GUIFrame::onOpenFile));
        Q_UNUSED(connect(this, &FileMenu::openFolder,
                receiver, &GUIFrame::onOpenFolder));
        Q_UNUSED(connect(this, &FileMenu::closeAllPatients,
                receiver, &GUIFrame::onCloseAllPatients));
        Q_UNUSED(connect(this, &FileMenu::restoreThumbnailsPanel,
                receiver, &GUIFrame::onRestoreThumbnailsPanel));
        Q_UNUSED(connect(this, &FileMenu::managePeers,
                receiver, &GUIFrame::onManagePeers));
        Q_UNUSED(connect(this, &FileMenu::queryRetrieve,
                receiver, &GUIFrame::onQueryRetrieve));
        Q_UNUSED(connect(this, &FileMenu::sendToPacs,
                receiver, &GUIFrame::onSendToPacs));
        Q_UNUSED(connect(this, &FileMenu::dimseEcho,
                receiver, &GUIFrame::onDimseEcho));
}

