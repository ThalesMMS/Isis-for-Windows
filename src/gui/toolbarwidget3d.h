/*
 * ------------------------------------------------------------------------------------
 *  File: toolbarwidget3d.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the ToolbarWidget3D class providing user actions for 3D visualization.
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

#include "ui_toolbarwidget3d.h"

namespace isis::gui
{
	class ToolbarWidget3D final : public QWidget
	{
	Q_OBJECT
	public:
		explicit ToolbarWidget3D(QWidget* parent = Q_NULLPTR);
		~ToolbarWidget3D() = default;

		[[nodiscard]] Ui::ToolbarWidget3D getUI() const { return m_ui; }

	private slots:
		void onFilterChanged(const int& t_index);
		void onCropPressed();

	signals:
		void filterChanged(const QString& t_path);
		void cropPressed(const bool& t_pressed);

	private:
		Ui::ToolbarWidget3D m_ui = {};

		void initView();
		void initCombo() const;
	};
}

