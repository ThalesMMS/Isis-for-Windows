/*
 * ------------------------------------------------------------------------------------
 *  File: toolbarwidgetmpr.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the ToolbarWidgetMPR Qt widget exposing multiplanar reconstruction controls.
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

#include <QWidget>
#include "ui_toolbarwidgetmpr.h"

namespace isis::gui
{
	class ToolbarWidgetMPR final : public QWidget
	{
	Q_OBJECT
	public:
		explicit ToolbarWidgetMPR(QWidget* parent = Q_NULLPTR);
		~ToolbarWidgetMPR() = default;

		//getters
		[[nodiscard]] Ui::ToolbarWidgetMPR getUI() const { return m_ui; }

	signals:
		void activateResliceWidget(const bool& t_flag);
		void resetResliceWidget();

	private slots:
		void onButtonPressed();

	private:
		Ui::ToolbarWidgetMPR m_ui = {};

		void initView();
		void createConnections() const;
	};
}

