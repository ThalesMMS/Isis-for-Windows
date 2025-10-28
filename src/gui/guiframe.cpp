/*
 * ------------------------------------------------------------------------------------
 *  File: guiframe.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Provides helper functions for initializing and wiring top-level GUI components.
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

#include "guiframe.h"
#include <QtGlobal>
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <QMessageBox>

#include "gui.h"
#include "utils.h"

isis::gui::GUIFrame::GUIFrame(QWidget* parent) :
	QMainWindow(parent)
{
	initView();
}

//-----------------------------------------------------------------------------
void isis::gui::GUIFrame::setContent(QWidget* t_widget)
{
	m_childWidget = t_widget;
	m_ui.widgetContent->layout()->addWidget(t_widget);
}

//-----------------------------------------------------------------------------
void isis::gui::GUIFrame::onChangeLayout(const WidgetsContainer::layouts& t_layout) const
{
	dynamic_cast<GUI*>(m_childWidget)->onChangeLayout(t_layout);
}

//-----------------------------------------------------------------------------
void isis::gui::GUIFrame::onOpenFile() const
{
	dynamic_cast<GUI*>(m_childWidget)->onOpenFile();
}

//-----------------------------------------------------------------------------
void isis::gui::GUIFrame::onOpenFolder() const
{
	dynamic_cast<GUI*>(m_childWidget)->onOpenFolder();
}

//-----------------------------------------------------------------------------
void isis::gui::GUIFrame::onCloseAllPatients() const
{
        dynamic_cast<GUI*>(m_childWidget)->onCloseAllPatients();
}

//-----------------------------------------------------------------------------
void isis::gui::GUIFrame::onRestoreThumbnailsPanel() const
{
        dynamic_cast<GUI*>(m_childWidget)->onRestoreThumbnailsPanel();
}

//-----------------------------------------------------------------------------
void isis::gui::GUIFrame::onManagePeers() const
{
        // Get GUI instance to access DIMSE network manager
        auto* gui = dynamic_cast<GUI*>(m_childWidget);
        if (!gui)
        {
                QMessageBox::warning(const_cast<GUIFrame*>(this), "Error",
                    "Could not access GUI instance");
                return;
        }

        gui->showManagePeersDialog(const_cast<GUIFrame*>(this));
}

//-----------------------------------------------------------------------------
void isis::gui::GUIFrame::onQueryRetrieve() const
{
        // Get GUI instance to access DIMSE network manager
        auto* gui = dynamic_cast<GUI*>(m_childWidget);
        if (!gui)
        {
                QMessageBox::warning(const_cast<GUIFrame*>(this), "Error",
                    "Could not access GUI instance");
                return;
        }

        gui->showQueryRetrieveDialog(const_cast<GUIFrame*>(this));
}

//-----------------------------------------------------------------------------
void isis::gui::GUIFrame::onSendToPacs() const
{
        // Get GUI instance to access DIMSE network manager
        auto* gui = dynamic_cast<GUI*>(m_childWidget);
        if (!gui)
        {
                QMessageBox::warning(const_cast<GUIFrame*>(this), "Error",
                    "Could not access GUI instance");
                return;
        }

        gui->showSendToPacsDialog(const_cast<GUIFrame*>(this));
}

//-----------------------------------------------------------------------------
void isis::gui::GUIFrame::onDimseEcho() const
{
        // Get GUI instance to access DIMSE network manager
        auto* gui = dynamic_cast<GUI*>(m_childWidget);
        if (!gui)
        {
                QMessageBox::warning(const_cast<GUIFrame*>(this), "Error",
                    "Could not access GUI instance");
                return;
        }

        auto* dimseManager = gui->getDimseNetworkManager();
        if (!dimseManager)
        {
                QMessageBox::warning(const_cast<GUIFrame*>(this), "Error",
                    "DIMSE Network Manager not initialized");
                return;
        }

        auto config = dimseManager->getConfig();
        auto peers = config->getEnabledPeers();

        if (peers.empty())
        {
                QMessageBox::information(const_cast<GUIFrame*>(this), "Error",
                    "No PACS peers configured.\n\nPlease use 'Manage PACS Peers...' to add servers.");
                return;
        }

        // Simple info dialog listing peers
        QString peerList;
        for (const auto& peer : peers)
        {
                peerList += QString("%1 (%2@%3:%4)\n")
                    .arg(QString::fromStdString(peer.name))
                    .arg(QString::fromStdString(peer.aeTitle))
                    .arg(QString::fromStdString(peer.hostname))
                    .arg(peer.port);
        }

        QMessageBox::information(const_cast<GUIFrame*>(this), "C-ECHO Test",
            QString("Configured PACS servers:\n\n%1\nUse 'Manage PACS Peers...' to test connections.")
                .arg(peerList));
}

//-----------------------------------------------------------------------------
void isis::gui::GUIFrame::initView()
{
	m_ui.setupUi(this);
	setWindowIcon(QIcon(iconapp));
	const QSize baselineContentSize(900, 640);
	const int minContentWidth = 640;
	const int minContentHeight = 480;
	const QScreen* targetScreen =
		windowHandle() ? windowHandle()->screen() : QGuiApplication::primaryScreen();
	const QSize availableSize = targetScreen
		? targetScreen->availableGeometry().size()
		: baselineContentSize;
	const int topBarHeight = 0;

	int contentWidth = baselineContentSize.width();
	if (availableSize.width() > 0)
	{
		contentWidth = qMin(contentWidth, availableSize.width());
		if (availableSize.width() >= minContentWidth)
		{
			contentWidth = qMax(contentWidth, minContentWidth);
		}
	}
	else
	{
		contentWidth = qMax(contentWidth, minContentWidth);
	}

	const int availableHeightForContent = availableSize.height() - topBarHeight;
	int contentHeight = baselineContentSize.height();
	if (availableHeightForContent > 0)
	{
		contentHeight = qMin(contentHeight, availableHeightForContent);
		if (availableHeightForContent >= minContentHeight)
		{
			contentHeight = qMax(contentHeight, minContentHeight);
		}
	}
	else
	{
		contentHeight = qMax(contentHeight, minContentHeight);
	}

	const QSize minimumFrameSize(contentWidth, contentHeight + topBarHeight);
	setMinimumSize(minimumFrameSize);
	m_ui.widgetContent->setMinimumSize(contentWidth, contentHeight);
}
