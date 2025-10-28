/*
 * ------------------------------------------------------------------------------------
 *  File: widget3d.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the Widget3D component managing a 3D render widget and its helpers.
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

#include <qfuture.h>
#include <QElapsedTimer>
#include <QLabel>
#include <QResizeEvent>
#include <QString>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>
#include "vtkwidget3d.h"
#include "widgetbase.h"
#include "toolbarwidget3d.h"
#include "ui_widget3d.h"

namespace isis::gui
{
	class Widget3D final : public WidgetBase
	{
	Q_OBJECT
	public:
		explicit Widget3D(QWidget* parent = Q_NULLPTR);
		~Widget3D() { m_future.waitForFinished(); }

		//getters
		[[nodiscard]] QFuture<void> getFuture() const { return m_future; }
		
		void render() override;

	signals:
		void finishedRenderAsync();

	protected:
		bool eventFilter(QObject* watched, QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

	private slots:
		void onfilterChanged(const QString& t_filter) const;
		void onCropPressed(const bool& t_pressed) const;
		void onActivateWidget(const bool& t_flag);
		void onSetMaximized() const;
		void onFinishedRenderAsync();

	private:
		Ui::Widget3D m_ui = {};
		ToolbarWidget3D* m_toolbar = {};
                QVTKOpenGLNativeWidget* m_qtvtkWidget = {};
                vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow3D = {};
                std::unique_ptr<vtkWidget3D> m_vtkWidget = {};
                QFuture<void> m_future = {};
                QElapsedTimer m_renderTimer = {};
		QLabel* m_statusOverlay = nullptr;

		void initView() override;
		void initData() override;
		void createConnections() override;
		void startLoadingAnimation() override;
		void positionLoadingAnimation();
		void showStatusOverlay(const QString& message, bool warning);
		void hideStatusOverlay();
		void updateStatusOverlayGeometry();
		void static onRenderAsync(Widget3D* t_self);
	};
}









