/*
 * ------------------------------------------------------------------------------------
 *  File: mproverlaycanvas.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the Qt overlay canvas responsible for painting MPR crosshair guides.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <QPointer>
#include <QWidget>
#include <array>

#include <vtkSmartPointer.h>

class vtkCallbackCommand;
class vtkObject;
class vtkRenderWindow;
class QVTKOpenGLNativeWidget;

namespace isis::gui
{
        class vtkWidgetMPR;

        class MprOverlayCanvas final : public QWidget
        {
        public:
                MprOverlayCanvas(QVTKOpenGLNativeWidget* t_hostWidget,
                        vtkWidgetMPR* t_widget,
                        int t_viewIndex);
                ~MprOverlayCanvas() override;

                void setCursorVisible(bool t_visible);
                void forceRefresh();

        protected:
                bool eventFilter(QObject* t_watched, QEvent* t_event) override;
                void paintEvent(QPaintEvent* t_event) override;

        private:
                [[nodiscard]] vtkRenderWindow* hostRenderWindow() const;
                void ensureGeometryMatchesHost();
                void registerRenderObserver();
                void unregisterRenderObserver();
                void scheduleUpdate();
                [[nodiscard]] QColor crosshairColor() const;
                [[nodiscard]] QPointF mapToWidget(double t_x, double t_y) const;

                static void onRenderEvent(vtkObject* t_caller,
                        unsigned long t_eventId,
                        void* t_clientData,
                        void* t_callData);

                QPointer<QVTKOpenGLNativeWidget> m_hostWidget = {};
                vtkWidgetMPR* m_mprWidget = nullptr;
                vtkSmartPointer<vtkCallbackCommand> m_renderCallback = {};
                unsigned long m_renderObserverTag = 0;
                int m_viewIndex = 0;
                bool m_cursorVisible = true;
        };
}
