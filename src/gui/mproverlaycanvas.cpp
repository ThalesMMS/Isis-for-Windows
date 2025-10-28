/*
 * ------------------------------------------------------------------------------------
 *  File: mproverlaycanvas.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements a transparent Qt overlay that paints MPR crosshair guides using QPainter.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "mproverlaycanvas.h"

#include <QEvent>
#include <QPainter>
#include <QVTKOpenGLNativeWidget.h>

#include <algorithm>
#include <vtkCallbackCommand.h>
#include <vtkCommand.h>
#include <vtkObject.h>
#include <vtkRenderWindow.h>

#include "vtkwidgetmpr.h"

namespace isis::gui
{
        namespace
        {
                constexpr std::array<QColor, 3> crosshairPalette = {
                        QColor(3, 218, 198),
                        QColor(255, 214, 10),
                        QColor(255, 110, 199)
                };
        }

        MprOverlayCanvas::MprOverlayCanvas(QVTKOpenGLNativeWidget* t_hostWidget,
                vtkWidgetMPR* t_widget,
                const int t_viewIndex)
                : QWidget(t_hostWidget)
                , m_hostWidget(t_hostWidget)
                , m_mprWidget(t_widget)
                , m_viewIndex(t_viewIndex)
        {
                setAttribute(Qt::WA_TransparentForMouseEvents, true);
                setAttribute(Qt::WA_NoSystemBackground);
                setAttribute(Qt::WA_TranslucentBackground);
                setAttribute(Qt::WA_OpaquePaintEvent, false);
                setFocusPolicy(Qt::NoFocus);
                if (m_hostWidget)
                {
                        m_hostWidget->installEventFilter(this);
                        ensureGeometryMatchesHost();
                        raise();
                }
                registerRenderObserver();
        }

        MprOverlayCanvas::~MprOverlayCanvas()
        {
                unregisterRenderObserver();
                if (m_hostWidget)
                {
                        m_hostWidget->removeEventFilter(this);
                }
        }

        void MprOverlayCanvas::setCursorVisible(const bool t_visible)
        {
                if (m_cursorVisible == t_visible)
                {
                        return;
                }
                m_cursorVisible = t_visible;
                scheduleUpdate();
        }

        void MprOverlayCanvas::forceRefresh()
        {
                scheduleUpdate();
        }

        bool MprOverlayCanvas::eventFilter(QObject* t_watched, QEvent* t_event)
        {
                if (!m_hostWidget || t_watched != m_hostWidget)
                {
                        return QWidget::eventFilter(t_watched, t_event);
                }

                switch (t_event->type())
                {
                case QEvent::Resize:
                case QEvent::Move:
                case QEvent::Show:
                        ensureGeometryMatchesHost();
                        if (m_cursorVisible)
                        {
                                show();
                        }
                        scheduleUpdate();
                        break;
                case QEvent::Hide:
                        hide();
                        break;
                default:
                        break;
                }
                return QWidget::eventFilter(t_watched, t_event);
        }

        void MprOverlayCanvas::paintEvent(QPaintEvent* t_event)
        {
                QWidget::paintEvent(t_event);
                if (!m_cursorVisible || !m_mprWidget)
                {
                        return;
                }

                std::array<std::array<double, 4>, 2> segments = {};
                if (!m_mprWidget->crosshairSegmentsForView(m_viewIndex, segments))
                {
                        return;
                }

                QPainter painter(this);
                painter.setRenderHint(QPainter::Antialiasing, true);

                QPen pen(crosshairColor());
                const qreal logicalWidth = std::max<qreal>(1.2, 1.2 / devicePixelRatioF());
                pen.setWidthF(logicalWidth);
                painter.setPen(pen);

                for (const auto& segment : segments)
                {
                        const QPointF p1 = mapToWidget(segment[0], segment[1]);
                        const QPointF p2 = mapToWidget(segment[2], segment[3]);
                        painter.drawLine(p1, p2);
                }
        }

        vtkRenderWindow* MprOverlayCanvas::hostRenderWindow() const
        {
                return (m_hostWidget) ? m_hostWidget->renderWindow() : nullptr;
        }

        void MprOverlayCanvas::ensureGeometryMatchesHost()
        {
                if (!m_hostWidget)
                {
                        return;
                }
                const QRect hostRect = m_hostWidget->rect();
                if (hostRect.isEmpty())
                {
                        return;
                }
                if (geometry() != hostRect)
                {
                        setGeometry(hostRect);
                        if (m_hostWidget->isVisible())
                        {
                                raise();
                        }
                }
        }

        void MprOverlayCanvas::registerRenderObserver()
        {
                if (m_renderCallback || !hostRenderWindow())
                {
                        return;
                }
                m_renderCallback = vtkSmartPointer<vtkCallbackCommand>::New();
                m_renderCallback->SetClientData(this);
                m_renderCallback->SetCallback(&MprOverlayCanvas::onRenderEvent);
                m_renderObserverTag = hostRenderWindow()->AddObserver(vtkCommand::EndEvent, m_renderCallback);
        }

        void MprOverlayCanvas::unregisterRenderObserver()
        {
                if (!hostRenderWindow() || !m_renderCallback || m_renderObserverTag == 0)
                {
                        return;
                }
                hostRenderWindow()->RemoveObserver(m_renderObserverTag);
                m_renderObserverTag = 0;
                m_renderCallback = nullptr;
        }

        void MprOverlayCanvas::scheduleUpdate()
        {
                QMetaObject::invokeMethod(
                        this,
                        static_cast<void (MprOverlayCanvas::*)()>(&MprOverlayCanvas::update),
                        Qt::QueuedConnection);
        }

        QColor MprOverlayCanvas::crosshairColor() const
        {
                if (m_viewIndex >= 0 && m_viewIndex < static_cast<int>(crosshairPalette.size()))
                {
                        return crosshairPalette[static_cast<std::size_t>(m_viewIndex)];
                }
                return QColor(3, 218, 198);
        }

        QPointF MprOverlayCanvas::mapToWidget(const double t_x, const double t_y) const
        {
                const qreal dpr = devicePixelRatioF();
                const qreal logicalX = static_cast<qreal>(t_x) / (dpr <= 0.0 ? 1.0 : dpr);
                const qreal logicalY = static_cast<qreal>(t_y) / (dpr <= 0.0 ? 1.0 : dpr);
                return {logicalX, static_cast<qreal>(height()) - logicalY};
        }

        void MprOverlayCanvas::onRenderEvent(vtkObject*,
                unsigned long,
                void* t_clientData,
                void*)
        {
                if (const auto canvas = static_cast<MprOverlayCanvas*>(t_clientData))
                {
                        canvas->scheduleUpdate();
                }
        }
}
