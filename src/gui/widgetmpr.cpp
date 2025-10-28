/*
 * ------------------------------------------------------------------------------------
 *  File: widgetmpr.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements the WidgetMPR composite that synchronizes three orthogonal slice views.
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

#include "widgetmpr.h"
#include <QFocusEvent>
#include <QLoggingCategory>
#include <vtkEventQtSlotConnect.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <QtConcurrent/qtconcurrentrun.h>
#include "tabwidget.h"
#include <algorithm>
#include "mproverlaycanvas.h"

Q_DECLARE_LOGGING_CATEGORY(lcLoadingAnimation)
Q_LOGGING_CATEGORY(lcWidgetMpr, "isis.gui.widgetmpr")

isis::gui::WidgetMPR::WidgetMPR(QWidget* parent)
        : WidgetBase(parent)
{
	initData();
	initView();
	createConnections();
	m_tabWidget = parent;
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetMPR::render()
{
        if (m_future.isRunning())
        {
                qCInfo(lcWidgetMpr) << "Render requested while previous async job is still running."
                                    << "Queuing rerun once it finishes.";
                m_pendingRenderRequest = true;
                return;
        }
        m_pendingRenderRequest = false;

        if (!m_image)
        {
                qCWarning(lcWidgetMpr) << "Cannot render MPR widget without image.";
                return;
        }
        if (!m_widgetMPR)
        {
                qCCritical(lcWidgetMpr) << "Cannot render MPR widget without underlying vtkWidgetMPR instance.";
                return;
        }

        bool interactorReady = true;
        const QString waitMessage = tr("Renderização aguardando inicialização");
        for (int i = 0; i < 3; ++i)
        {
                auto* const widget = m_qtvtkWidgets[i];
                if (!widget)
                {
                        qCWarning(lcWidgetMpr)
                                << "Render aborted: missing QVTK widget for view"
                                << i;
                        interactorReady = false;
                        continue;
                }

                auto* const renderWindow = widget->renderWindow();
                if (!renderWindow)
                {
                        qCWarning(lcWidgetMpr)
                                << "Render aborted: missing render window for view"
                                << i;
                        interactorReady = false;
                        continue;
                }

		auto* interactor = renderWindow->GetInteractor();
		if (!interactor)
		{
			auto* createdInteractor = renderWindow->MakeRenderWindowInteractor();
			if (createdInteractor)
			{
				createdInteractor->Initialize();
			}
			interactor = createdInteractor ? createdInteractor : renderWindow->GetInteractor();
		}
                if (!interactor)
                {
                        qCWarning(lcWidgetMpr)
                                << "Render aborted: interactor unavailable for view"
                                << i;
                        interactorReady = false;
                }
        }

        if (!interactorReady)
        {
                showStatusOverlay(waitMessage, true);
                qCWarning(lcWidgetMpr) << "Postponing render until all VTK interactors are initialized.";
                return;
        }
        qCInfo(lcWidgetMpr) << "Starting render for" << (m_image->getIsMultiFrame() ? "multi-frame" : "single-frame")
                            << "dataset.";
        m_toolbar->getUI().toolButtonReslicer->setVisible(false);
        m_toolbar->getUI().toolButtonReset->setVisible(false);
        qCDebug(lcWidgetMpr) << "Hid reslice/reset actions while preparing render.";
        hideStatusOverlay();
        startLoadingAnimation();
        if (m_image->getIsMultiFrame())
        {
                qCDebug(lcWidgetMpr) << "Assigning multi-frame image to vtkWidgetMPR.";
                m_widgetMPR->setImage(m_image);
        }
        else
        {
                qCDebug(lcWidgetMpr) << "Assigning single-frame image and series to vtkWidgetMPR.";
                m_widgetMPR->setImage(m_image);
                if (!m_series)
                {
                        qCWarning(lcWidgetMpr) << "Single-frame render requested but series metadata is missing.";
                }
                m_widgetMPR->setSeries(m_series);
        }
        qCInfo(lcWidgetMpr) << "Launching asynchronous render task.";
        const Qt::ConnectionType connectionType = Qt::ConnectionType(
                Qt::QueuedConnection | Qt::UniqueConnection);
        const bool connected = connect(this, &WidgetMPR::finishedRenderAsync,
                this, &WidgetMPR::onFinishedRenderAsync, connectionType);
        if (!connected)
        {
                qCWarning(lcWidgetMpr) << "Failed to establish finishedRenderAsync connection before async render.";
        }
        m_future = QtConcurrent::run(onRenderAsync, this);
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetMPR::onActivateResliceWidget(const bool& t_flag)
{
        m_resliceCursorEnabled = t_flag;
	if (m_widgetMPR)
	{
		m_widgetMPR->setShowCursor(t_flag);
	}
	for (const auto& overlay : m_overlays)
	{
		if (overlay)
		{
			overlay->setCursorVisible(t_flag);
		}
	}
        if (m_toolbar)
        {
                m_toolbar->getUI().toolButtonReslicer->setChecked(t_flag);
        }
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetMPR::onResetResliceWidget() const
{
	if (m_widgetMPR)
	{
		m_widgetMPR->resetResliceWidget();
	}
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetMPR::onSetMaximized() const
{
	if (m_tabWidget)
	{
		dynamic_cast<TabWidget*>
			(m_tabWidget)->onMaximize();
	}
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetMPR::onActivateWidget(const bool& t_flag, QObject* t_object)
{
	for (const auto& widget : m_qtvtkWidgets)
	{
		if (widget == t_object)
		{
			m_widgetMPR->setActiveRenderWindow(widget->renderWindow());
		}
	}
	if (t_flag)
	{
		auto* event = new QFocusEvent(QEvent::FocusIn,
			Qt::FocusReason::MouseFocusReason);
		focusInEvent(event);
		delete event;
	}
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetMPR::onFinishedRenderAsync()
{
        const bool shouldRerun = m_pendingRenderRequest;
        m_pendingRenderRequest = false;

        qCInfo(lcWidgetMpr) << "Asynchronous render task completed.";
        disconnect(this, &WidgetMPR::finishedRenderAsync,
                this, &WidgetMPR::onFinishedRenderAsync);
        stopLoadingAnimation();

        const QString failureMessage = m_widgetMPR->lastFailureMessage();
        const QString warningMessage = m_widgetMPR->lastWarningMessage();
        const bool hasRenderableVolume = m_widgetMPR->hasValidVolume();

        if (!hasRenderableVolume)
        {
                const QString displayMessage = failureMessage.isEmpty()
                        ? tr("Unable to reconstruct multiplanar views for this dataset.")
                        : failureMessage;
                for (const auto& widget : m_qtvtkWidgets)
                {
                        widget->setVisible(true);
                        qCDebug(lcWidgetMpr) << "Widget" << widget << "is now visible:" << widget->isVisible();
                }
                showStatusOverlay(displayMessage, false);
                qCWarning(lcWidgetMpr) << "MPR volume unavailable." << displayMessage;
                if (shouldRerun)
                {
                        qCInfo(lcWidgetMpr) << "Dispatching queued render after failed async job.";
                        render();
                }
                return;
        }

        for (const auto& widget : m_qtvtkWidgets)
        {
                widget->setVisible(true);
                qCDebug(lcWidgetMpr) << "Widget" << widget << "is now visible:" << widget->isVisible();
        }
        hideStatusOverlay();

	m_widgetMPR->setRenderWindowsMPR(
		m_qtvtkWidgets[0]->renderWindow(),
		m_qtvtkWidgets[1]->renderWindow(),
		m_qtvtkWidgets[2]->renderWindow());
	qCDebug(lcWidgetMpr) << "Render windows assigned to vtkWidgetMPR, triggering render.";
	m_widgetMPR->render();
	m_widgetMPR->setShowCursor(m_resliceCursorEnabled);
	for (const auto& overlay : m_overlays)
	{
		if (overlay)
		{
			overlay->setCursorVisible(m_resliceCursorEnabled);
			overlay->forceRefresh();
		}
	}
	m_toolbar->getUI().toolButtonReslicer->setChecked(m_resliceCursorEnabled);
	m_toolbar->getUI().toolButtonReslicer->setVisible(true);
	m_toolbar->getUI().toolButtonReset->setVisible(true);
	qCDebug(lcWidgetMpr) << "Reslice/reset actions restored visibility.";

        if (!warningMessage.isEmpty())
        {
                showStatusOverlay(warningMessage, true);
                qCWarning(lcWidgetMpr) << "MPR rendered with warning." << warningMessage;
        }
        else
        {
                hideStatusOverlay();
        }

        if (shouldRerun)
        {
                qCInfo(lcWidgetMpr) << "Dispatching queued render after completing async job.";
                render();
        }
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetMPR::initData()
{
	m_toolbar = new ToolbarWidgetMPR(this);
        m_toolbar->getUI().toolButtonReslicer->setChecked(m_resliceCursorEnabled);
	for (auto i = 0; i < 3; ++i)
	{
		m_qtvtkWidgets[i] = new QVTKOpenGLNativeWidget(this);
		m_renderWindow[i] = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
		m_qtvtkWidgets[i]->setRenderWindow(m_renderWindow[i]);
		m_qtvtkWidgets[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		m_qtvtkWidgets[i]->setVisible(false);
	}
        m_statusOverlay = new QLabel(this);
        m_statusOverlay->setObjectName(QStringLiteral("mprStatusOverlay"));
        m_statusOverlay->setAlignment(Qt::AlignCenter);
        m_statusOverlay->setWordWrap(true);
        m_statusOverlay->setTextFormat(Qt::RichText);
        m_statusOverlay->setVisible(false);
        m_statusOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        m_statusOverlay->setStyleSheet(QStringLiteral("background-color: transparent; font-size: 14px;"));
        updateStatusOverlayGeometry();
	m_widgetMPR = std::make_unique<vtkWidgetMPR>();
	for (auto i = 0; i < 3; ++i)
	{
		m_overlays[i] = std::make_unique<MprOverlayCanvas>(m_qtvtkWidgets[i], m_widgetMPR.get(), i);
		m_overlays[i]->setCursorVisible(m_resliceCursorEnabled);
	}
	m_vtkEvents = std::make_unique<vtkEventFilter>(this);
	setWidgetType(WidgetType::widgetmpr);
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetMPR::initView()
{
	m_ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
        auto* const layout = new QVBoxLayout;
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(2);
        auto* const widgetsLayout = new QGridLayout;
	widgetsLayout->addWidget(m_qtvtkWidgets[0], 0, 0);
	widgetsLayout->addWidget(m_qtvtkWidgets[1], 1, 0);
	widgetsLayout->addWidget(m_qtvtkWidgets[2], 0, 1, 2, 1);
        widgetsLayout->setContentsMargins(0, 0, 0, 0);
	widgetsLayout->setSpacing(2);
	layout->addWidget(m_toolbar);
	layout->addLayout(widgetsLayout);
	setLayout(layout);
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetMPR::createConnections()
{
	if (m_toolbar)
	{
		Q_UNUSED(connect(m_toolbar,
			&ToolbarWidgetMPR::activateResliceWidget,
			this, &WidgetMPR::onActivateResliceWidget));
		Q_UNUSED(connect(m_toolbar,
			&ToolbarWidgetMPR::resetResliceWidget,
			this, &WidgetMPR::onResetResliceWidget));
	}
	setFocusPolicy(Qt::FocusPolicy::WheelFocus);
	for (const auto& widgets : m_qtvtkWidgets)
	{
		widgets->installEventFilter(m_vtkEvents.get());
	}
	Q_UNUSED(connect(m_vtkEvents.get(),
		&vtkEventFilter::activateWidget,
		this, &WidgetMPR::onActivateWidget));
	Q_UNUSED(connect(m_vtkEvents.get(),
		&vtkEventFilter::setMaximized,
		this, &WidgetMPR::onSetMaximized));
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetMPR::startLoadingAnimation()
{
        if (m_loadingAnimation)
        {
                m_loadingAnimation->hide();
                m_loadingAnimation.reset();
        }
}

void isis::gui::WidgetMPR::positionLoadingAnimation()
{
        if (!m_loadingAnimation)
        {
                return;
        }

        const QRect targetRect = rect();
        if (targetRect.isEmpty())
        {
                return;
        }
        const QSize overlaySize = m_loadingAnimation->size();
        const QPoint desired = targetRect.center() - QPoint(overlaySize.width() / 2, overlaySize.height() / 2);
        const int clampedX = std::max(targetRect.left(), std::min(desired.x(), targetRect.right() - overlaySize.width() + 1));
        const int clampedY = std::max(targetRect.top(), std::min(desired.y(), targetRect.bottom() - overlaySize.height() + 1));
        m_loadingAnimation->move(clampedX, clampedY);
        qCDebug(lcLoadingAnimation) << "WidgetMPR overlay positioned"
                << "widget" << this
                << "targetRect" << targetRect
                << "overlaySize" << overlaySize
                << "topLeft" << QPoint(clampedX, clampedY);
}

void isis::gui::WidgetMPR::resizeEvent(QResizeEvent* event)
{
        QWidget::resizeEvent(event);
        positionLoadingAnimation();
        updateStatusOverlayGeometry();
}

void isis::gui::WidgetMPR::showStatusOverlay(const QString& message, const bool warning)
{
        if (!m_statusOverlay)
        {
                return;
        }

        const QString paletteColor = warning ? QStringLiteral("#f2c94c") : QStringLiteral("#ff6b6b");
        const QString escaped = (message.isEmpty()
                                         ? tr("Unexpected renderer state.")
                                         : message).toHtmlEscaped().replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
        const QString html = QStringLiteral(
                "<div style=\"display:inline-block; max-width:72%%; background-color: rgba(16, 16, 16, 200); "
                "color:%1; padding: 18px 26px; border-radius: 12px; font-weight:600;\">%2</div>")
                .arg(paletteColor, escaped);
        m_statusOverlay->setText(html);
        m_statusOverlay->adjustSize();
        updateStatusOverlayGeometry();
        m_statusOverlay->show();
        m_statusOverlay->raise();
}

void isis::gui::WidgetMPR::hideStatusOverlay()
{
        if (m_statusOverlay)
        {
                m_statusOverlay->hide();
        }
}

void isis::gui::WidgetMPR::updateStatusOverlayGeometry()
{
        if (!m_statusOverlay)
        {
                return;
        }
        const QRect targetRect = rect();
        if (targetRect.isEmpty())
        {
                return;
        }
        const QSize hint = m_statusOverlay->sizeHint();
        if (!hint.isValid())
        {
                return;
        }
        const int horizontalMargin = 24;
        const int verticalMargin = 20;
        const int width = std::min(targetRect.width() - 2 * horizontalMargin, hint.width() + 32);
        const int height = hint.height() + 24;
        const int x = targetRect.left() + (targetRect.width() - width) / 2;
        const int y = targetRect.top() + verticalMargin;
        m_statusOverlay->setGeometry(QRect(x, y, width, height));
}
//-----------------------------------------------------------------------------
void isis::gui::WidgetMPR::onRenderAsync(WidgetMPR* t_self)
{
        if (t_self->m_widgetMPR)
        {
                try
                {
                        qCInfo(lcWidgetMpr) << "Asynchronous task creating 3D matrix.";
                        t_self->m_widgetMPR->create3DMatrix();
                        qCInfo(lcWidgetMpr) << "3D matrix creation completed inside async task.";
                }
                catch (const std::exception& ex)
                {
                        qCCritical(lcWidgetMpr) << "Failed to create 3D matrix:" << ex.what();
                }
                emit t_self->finishedRenderAsync();
        }
        else
        {
                qCWarning(lcWidgetMpr) << "Async render task invoked without vtkWidgetMPR instance.";
        }
}






