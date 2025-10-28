/*
 * ------------------------------------------------------------------------------------
 *  File: framelesswindow.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the FramelessWindow base class that provides chrome-less window behaviour.
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


#include <QFrame>
#include <QMainWindow>
#include <QWidget>
#include <QList>
#include <QMargins>
#include <QRect>
#include <QLoggingCategory>
#include <QtGlobal>

Q_DECLARE_LOGGING_CATEGORY(lcFramelessWindow)


namespace isis::gui
{
	class Frameless : public QFrame
	{
	Q_OBJECT
	public:
		explicit Frameless(QWidget* parent = nullptr);
		~Frameless() = default;

		void setResizeable(bool resizeable = true);
		void setResizeableAreaWidth(int width = 5);
		void setContsMargins(const QMargins& margins);
		void setContMargins(int left, int top, int right, int bottom);

		[[nodiscard]] bool isResizeable() const { return m_bResizeable; }
		[[nodiscard]] QMargins contMargins() const;
		[[nodiscard]] QRect contRect() const;
		void getaContMargins(int* left, int* top, int* right, int* bottom) const;

	public slots:
		void fullScreen();

	protected:
		void setTitleBar(QWidget* titlebar);
		void ignoreWidget(QWidget* widget);
		bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
		void showEvent(QShowEvent* event) override;
		void hideEvent(QHideEvent* event) override;
		void resizeEvent(QResizeEvent* event) override;

	private slots:
		void onTitleBarDestroyed();

	private:
		QWidget* m_titlebar = {};
		QList<QWidget*> m_whiteList;
		int m_borderWidth;
		QMargins m_margins;
		QMargins m_frames;
		bool m_bJustMaximized;
		bool m_bResizeable;

		void logWindowMetrics(const char* context) const;
	};
}
