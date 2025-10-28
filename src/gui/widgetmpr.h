/*
 * ------------------------------------------------------------------------------------
 *  File: widgetmpr.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the WidgetMPR component used to host synchronized multiplanar views.
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

#include <QFuture>
#include <QLabel>
#include <QVTKOpenGLNativeWidget.h>
#include <QResizeEvent>
#include "ui_widgetmpr.h"
#include "vtkwidgetmpr.h"
#include "toolbarwidgetmpr.h"
#include "mproverlaycanvas.h"

namespace isis::gui
{
	class WidgetMPR final : public WidgetBase
	{
	Q_OBJECT
	public:
		explicit WidgetMPR(QWidget* parent = Q_NULLPTR);
		~WidgetMPR() { m_future.waitForFinished(); }

		//getters
		[[nodiscard]] QFuture<void> getFuture() const { return m_future; }

		void render() override;
		void resizeEvent(QResizeEvent* event) override;

	signals:
		void finishedRenderAsync();

	private slots:
		void onActivateResliceWidget(const bool& t_flag);
		void onResetResliceWidget() const;
		void onSetMaximized() const;
		void onActivateWidget(const bool& t_flag, QObject* t_object);
		void onFinishedRenderAsync();

	private:
		Ui::WidgetMPR m_ui = {};
		QVTKOpenGLNativeWidget* m_qtvtkWidgets[3] = {};
		std::unique_ptr<vtkWidgetMPR> m_widgetMPR = {};
		ToolbarWidgetMPR* m_toolbar = {};
		QFuture<void> m_future = {};
                QLabel* m_statusOverlay = nullptr;
                bool m_pendingRenderRequest = false;
                bool m_resliceCursorEnabled = true;
                std::unique_ptr<MprOverlayCanvas> m_overlays[3] = {};


		void initData() override;
		void initView() override;
		void createConnections() override;
		void startLoadingAnimation() override;
		void positionLoadingAnimation();
		void showStatusOverlay(const QString& message, bool warning);
		void hideStatusOverlay();
		void updateStatusOverlayGeometry();
		void static onRenderAsync(WidgetMPR* t_self);
	};
}

