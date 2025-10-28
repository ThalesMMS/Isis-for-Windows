/*
 * ------------------------------------------------------------------------------------
 *  File: framelesswindow.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements a custom frameless Qt window with draggable chrome and shadow effects.
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

#include "framelesswindow.h"
#include <QApplication>
#include <QString>
#include <QPoint>
#include <QLoggingCategory>
#include <QShowEvent>
#include <QHideEvent>
#include <QResizeEvent>
#include <windows.h>
#include <WinUser.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <iostream>
#pragma comment (lib,"Dwmapi.lib")
#pragma comment (lib,"user32.lib")

Q_LOGGING_CATEGORY(lcFramelessWindow, "isis.gui.framelesswindow")

isis::gui::Frameless::Frameless(QWidget* parent)
	: QFrame(parent),
	  m_borderWidth(5),
	  m_bJustMaximized(false),
	  m_bResizeable(true)
{
	setObjectName(QStringLiteral("FramelessWindow"));
	setWindowFlags(windowFlags() | Qt::Window
		| Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
	setResizeable(m_bResizeable);
	logWindowMetrics("ctor");
}

//-----------------------------------------------------------------------------
void isis::gui::Frameless::setResizeable(const bool resizeable)
{
	const bool visible = isVisible();
	m_bResizeable = resizeable;
	if (m_bResizeable)
	{
		setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
		auto hwnd = reinterpret_cast<HWND>(this->winId());
		const DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
		::SetWindowLong(hwnd, GWL_STYLE, style | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CAPTION);
	}
	else
	{
		setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
		auto hwnd = reinterpret_cast<HWND>(this->winId());
		const DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
		::SetWindowLong(hwnd, GWL_STYLE, style & ~WS_MAXIMIZEBOX & ~WS_CAPTION);
	}
	const MARGINS shadow = {1, 1, 1, 1};
	DwmExtendFrameIntoClientArea(HWND(winId()), &shadow);
	setVisible(visible);
	qCDebug(lcFramelessWindow) << "setResizeable"
		<< "resizeable" << m_bResizeable
		<< "visibleBefore" << visible
		<< "frames" << m_frames
		<< "margins" << m_margins;
	logWindowMetrics("setResizeable");
}

//-----------------------------------------------------------------------------
void isis::gui::Frameless::setResizeableAreaWidth(int width)
{
	if (1 > width) width = 1;
	m_borderWidth = width;
}

//-----------------------------------------------------------------------------
void isis::gui::Frameless::setTitleBar(QWidget* titlebar)
{
	m_titlebar = titlebar;
	if (!titlebar) return;
	connect(titlebar, SIGNAL(destroyed(QObject*)), this, SLOT(onTitleBarDestroyed()));
}

//-----------------------------------------------------------------------------
void isis::gui::Frameless::onTitleBarDestroyed()
{
	if (m_titlebar == sender())
	{
		m_titlebar = Q_NULLPTR;
	}
}

//-----------------------------------------------------------------------------
void isis::gui::Frameless::ignoreWidget(QWidget* widget)
{
	if (!widget) return;
	if (m_whiteList.contains(widget)) return;
	m_whiteList.append(widget);
}

//-----------------------------------------------------------------------------
bool isis::gui::Frameless::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
#if (QT_VERSION == QT_VERSION_CHECK(5, 11, 1))
    MSG* msg = *reinterpret_cast<MSG**>(message);
#else
	MSG* msg = static_cast<MSG*>(message);
#endif

	switch (msg->message)
	{
	case WM_NCCALCSIZE:
		{
			const auto adjustClientArea = [](RECT& rect)
			{
				if (rect.top > 0)
				{
					rect.top -= 1;
				}
			};

			if (msg->wParam == TRUE)
			{
				auto* const params = reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
				if (params)
				{
					adjustClientArea(params->rgrc[0]);
				}
				*result = 0;
				return true;
			}

			if (msg->lParam != 0)
			{
				auto* const rect = reinterpret_cast<RECT*>(msg->lParam);
				if (rect)
				{
					adjustClientArea(*rect);
				}
				*result = 0;
				return true;
			}

			break;
		}
	case WM_NCHITTEST:
		{
			*result = 0;

			const LONG border_width = m_borderWidth;
			RECT winrect;
			GetWindowRect(HWND(winId()), &winrect);

			const long x = GET_X_LPARAM(msg->lParam);
			const long y = GET_Y_LPARAM(msg->lParam);

			if (m_bResizeable)
			{
				const bool resizeWidth = minimumWidth() != maximumWidth();
				const bool resizeHeight = minimumHeight() != maximumHeight();

				if (resizeWidth)
				{
					//left border
					if (x >= winrect.left && x < winrect.left + border_width)
					{
						*result = HTLEFT;
					}
					//right border
					if (x < winrect.right && x >= winrect.right - border_width)
					{
						*result = HTRIGHT;
					}
				}
				if (resizeHeight)
				{
					//bottom border
					if (y < winrect.bottom && y >= winrect.bottom - border_width)
					{
						*result = HTBOTTOM;
					}
					//top border
					if (y >= winrect.top && y < winrect.top + border_width)
					{
						*result = HTTOP;
					}
				}
				if (resizeWidth && resizeHeight)
				{
					//bottom left corner
					if (x >= winrect.left && x < winrect.left + border_width &&
						y < winrect.bottom && y >= winrect.bottom - border_width)
					{
						*result = HTBOTTOMLEFT;
					}
					//bottom right corner
					if (x < winrect.right && x >= winrect.right - border_width &&
						y < winrect.bottom && y >= winrect.bottom - border_width)
					{
						*result = HTBOTTOMRIGHT;
					}
					//top left corner
					if (x >= winrect.left && x < winrect.left + border_width &&
						y >= winrect.top && y < winrect.top + border_width)
					{
						*result = HTTOPLEFT;
					}
					//top right corner
					if (x < winrect.right && x >= winrect.right - border_width &&
						y >= winrect.top && y < winrect.top + border_width)
					{
						*result = HTTOPRIGHT;
					}
				}
			}
			if (0 != *result) return true;

			//*result still equals 0, that means the cursor locate OUTSIDE the frame area
			//but it may locate in titlebar area
			if (!m_titlebar) return false;

			//support highdpi
			const double dpr = this->devicePixelRatioF();
			const QPoint pos = m_titlebar->mapFromGlobal(QPoint(x / dpr, y / dpr));

			if (!m_titlebar->rect().contains(pos)) return false;
			QWidget* child = m_titlebar->childAt(pos);
			if (!child)
			{
				*result = HTCAPTION;
				return true;
			}
			else
			{
				if (m_whiteList.contains(child))
				{
					*result = HTCAPTION;
					return true;
				}
			}
			return false;
		} //end case WM_NCHITTEST
	case WM_GETMINMAXINFO:
		{
			if (IsZoomed(msg->hwnd))
			{
				RECT frame = {0, 0, 0, 0};
				AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);

				//record frame area data
				const double dpr = this->devicePixelRatioF();

				m_frames.setLeft(lround(abs(frame.left) / dpr + 0.5));
				m_frames.setTop(lround(abs(frame.bottom) / dpr + 0.5));
				m_frames.setRight(lround(abs(frame.right) / dpr + 0.5));
				m_frames.setBottom(lround(abs(frame.bottom) / dpr + 0.5));

				setContentsMargins(m_frames.left() + m_margins.left(),
				                   m_frames.top() + m_margins.top(),
				                   m_frames.right() + m_margins.right(),
				                   m_frames.bottom() + m_margins.bottom());
				m_bJustMaximized = true;
				qCDebug(lcFramelessWindow) << "nativeEvent WM_GETMINMAXINFO maximized"
					<< "frames" << m_frames
					<< "margins" << m_margins;
			}
			else
			{
				if (m_bJustMaximized)
				{
					setContentsMargins(m_margins);
					m_frames = QMargins();
					m_bJustMaximized = false;
					qCDebug(lcFramelessWindow) << "nativeEvent WM_GETMINMAXINFO restored";
				}
			}
			return false;
		}
	default:
		break;
	}
	return QFrame::nativeEvent(eventType, message, result);
}

//-----------------------------------------------------------------------------
void isis::gui::Frameless::showEvent(QShowEvent* event)
{
	QFrame::showEvent(event);
	logWindowMetrics("showEvent");
}

//-----------------------------------------------------------------------------
void isis::gui::Frameless::hideEvent(QHideEvent* event)
{
	logWindowMetrics("hideEvent");
	QFrame::hideEvent(event);
}

//-----------------------------------------------------------------------------
void isis::gui::Frameless::resizeEvent(QResizeEvent* event)
{
	QFrame::resizeEvent(event);
	logWindowMetrics("resizeEvent");
}

//-----------------------------------------------------------------------------
void isis::gui::Frameless::setContsMargins(const QMargins& margins)
{
	setContentsMargins(margins + m_frames);
	m_margins = margins;
}

//-----------------------------------------------------------------------------
void isis::gui::Frameless::setContMargins(int left, int top, int right, int bottom)
{
	setContentsMargins(left + m_frames.left(),
		top + m_frames.top(),
		right + m_frames.right(),
		bottom + m_frames.bottom());
	m_margins.setLeft(left);
	m_margins.setTop(top);
	m_margins.setRight(right);
	m_margins.setBottom(bottom);
}

//-----------------------------------------------------------------------------
QMargins isis::gui::Frameless::contMargins() const
{
	QMargins margins = contentsMargins();
	margins -= m_frames;
	return margins;
}

//-----------------------------------------------------------------------------
void isis::gui::Frameless::getaContMargins(int* left, int* top, int* right, int* bottom) const
{
	if (!(left && top && right && bottom))
	{
		return;
	}
	const QMargins margins = contentsMargins();
	*left = margins.left();
	*top = margins.top();
	*right = margins.right();
	*bottom = margins.bottom();
	if (isMaximized())
	{
		*left -= m_frames.left();
		*top -= m_frames.top();
		*right -= m_frames.right();
		*bottom -= m_frames.bottom();
	}
}

//-----------------------------------------------------------------------------
QRect isis::gui::Frameless::contRect() const
{
	QRect rect = contentsRect();
	const int width = rect.width();
	const int height = rect.height();
	rect.setLeft(rect.left() - m_frames.left());
	rect.setTop(rect.top() - m_frames.top());
	rect.setWidth(width);
	rect.setHeight(height);
	return rect;
}

//-----------------------------------------------------------------------------
void isis::gui::Frameless::logWindowMetrics(const char* context) const
{
	const QRect geo = geometry();
	const QRect frameGeo = frameGeometry();
	const QPoint globalTopLeft = mapToGlobal(QPoint(0, 0));
	qCDebug(lcFramelessWindow) << context
		<< "widget" << this
		<< "objectName" << objectName()
		<< "visible" << isVisible()
		<< "windowFlags" << Qt::hex << static_cast<quint64>(windowFlags())
		<< "hasTranslucentAttr" << testAttribute(Qt::WA_TranslucentBackground)
		<< "geometry" << geo
		<< "frameGeometry" << frameGeo
		<< "globalTopLeft" << globalTopLeft
		<< "devicePixelRatio" << devicePixelRatioF();
}

//-----------------------------------------------------------------------------
void isis::gui::Frameless::fullScreen()
{
	if (isMaximized())
	{
		setContentsMargins(m_margins);
		m_frames = QMargins();
	}
	showFullScreen();
}

