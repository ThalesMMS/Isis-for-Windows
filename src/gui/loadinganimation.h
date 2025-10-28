/*
 * ------------------------------------------------------------------------------------
 *  File: loadinganimation.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the LoadingAnimation widget used to indicate background processing.
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

#include "ui_loadinganimation.h"
#include <QLoggingCategory>
#include <QMovie>
#include <QWidget>

Q_DECLARE_LOGGING_CATEGORY(lcLoadingAnimation)

namespace isis::gui
{
	class LoadingAnimation final : public QWidget
	{
	Q_OBJECT
	public:
		explicit LoadingAnimation(QWidget* parent = nullptr);
		~LoadingAnimation() = default;

	private:
		Ui::LoadingAnimation m_ui = {};
		QMovie m_movie {};

		void initView();
		void showEvent(QShowEvent* event) override;
		void hideEvent(QHideEvent* event) override;
		void moveEvent(QMoveEvent* event) override;
		void resizeEvent(QResizeEvent* event) override;
		void logGeometry(const char* context) const;
	};
}
