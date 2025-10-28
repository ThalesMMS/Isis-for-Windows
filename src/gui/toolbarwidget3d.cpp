/*
 * ------------------------------------------------------------------------------------
 *  File: toolbarwidget3d.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements the 3D toolbar widget exposing volume rendering controls and presets.
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

#include "toolbarwidget3d.h"
#include "utils.h"
#include <qdir.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QToolButton>


isis::gui::ToolbarWidget3D::ToolbarWidget3D(QWidget* parent)
	: QWidget(parent)
{
	initView();
}

//-----------------------------------------------------------------------------
void isis::gui::ToolbarWidget3D::onFilterChanged(const int& t_index)
{
	const auto path =
		m_ui.comboBoxFilters->itemData(t_index);
	if (path.isValid())
	{
		emit filterChanged(path.toString());
	}
}

//-----------------------------------------------------------------------------
void isis::gui::ToolbarWidget3D::onCropPressed()
{
	emit cropPressed(dynamic_cast<QToolButton*>
		(sender())->isChecked());
}

//-----------------------------------------------------------------------------
void isis::gui::ToolbarWidget3D::initView()
{
	m_ui.setupUi(this);
	initCombo();
}

//-----------------------------------------------------------------------------
void isis::gui::ToolbarWidget3D::initCombo() const
{
	QDir dir(filters3dDir);
	dir.setNameFilters(QStringList() << "*.json");
	const auto files =
		dir.entryInfoList();
	for (const auto& file : files)
	{
		QFile currentFile(file.absoluteFilePath());
		if (!currentFile.open(QIODevice::ReadOnly
			| QIODevice::Text))
		{
			continue;
		}
		const QByteArray data =
			currentFile.readAll();
		currentFile.close();
		const auto doc =
			QJsonDocument::fromJson(data);
		const QJsonObject root =
			doc.object();
		if (!root.empty())
		{
			m_ui.comboBoxFilters->addItem(
				root.value("name").toString());
			m_ui.comboBoxFilters->setItemData(
				m_ui.comboBoxFilters->count() - 1,
				file.absoluteFilePath());
		}
	}
	m_ui.comboBoxFilters->addItem("MIP");
	m_ui.comboBoxFilters->setItemData(
		m_ui.comboBoxFilters->count() - 1, "MIP");
}

