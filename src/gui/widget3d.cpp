/*
 * ------------------------------------------------------------------------------------
 *  File: widget3d.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements the Widget3D composite that couples 3D renderers with overlays and controls.
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

#include "widget3d.h"
#include "tabwidget.h"
#include <vtkRendererCollection.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkNew.h>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QLoggingCategory>
#include <QString>
#include <QMetaObject>
#include <QtConcurrent/qtconcurrentrun.h>
#include <algorithm>

Q_DECLARE_LOGGING_CATEGORY(lcLoadingAnimation)
Q_LOGGING_CATEGORY(lcWidget3D, "isis.gui.widget3d")

isis::gui::Widget3D::Widget3D(QWidget* parent)
	: WidgetBase(parent)
{
	initData();
	initView();
	createConnections();
	m_tabWidget = parent;
}

//-----------------------------------------------------------------------------
void isis::gui::Widget3D::render()
{
        qCInfo(lcWidget3D) << "render() requested" << "patientIdx" << m_patientIndex
                           << "studyIdx" << m_studyIndex << "seriesIdx" << m_seriesIndex
                           << "imageIdx" << m_imageIndex << "hasImage" << static_cast<bool>(m_image)
                           << "hasSeries" << static_cast<bool>(m_series);
        if (!m_image)
        {
                const QString failureMessage = tr("Unable to compose a 3D volume because no image is loaded.");
                showStatusOverlay(failureMessage, false);
                qCWarning(lcWidget3D)
                        << "render() aborted: missing image for seriesIdx"
                        << m_seriesIndex
                        << "message"
                        << failureMessage;
                return;
        }
        if (!m_series)
        {
                const QString failureMessage = tr("Unable to compose a 3D volume because the series metadata is missing.");
                showStatusOverlay(failureMessage, false);
                qCWarning(lcWidget3D)
                        << "render() aborted: missing series for seriesIdx"
                        << m_seriesIndex
                        << "message"
                        << failureMessage;
                return;
        }

        auto* const renderWindow = (m_qtvtkWidget) ? m_qtvtkWidget->renderWindow() : nullptr;
        if (!renderWindow)
        {
                const QString waitMessage = tr("Renderização aguardando inicialização");
                showStatusOverlay(waitMessage, true);
                qCWarning(lcWidget3D)
                        << "render() aborted: render window unavailable for interactor initialization.";
                return;
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
                const QString waitMessage = tr("Renderização aguardando inicialização");
                showStatusOverlay(waitMessage, true);
                qCWarning(lcWidget3D)
                        << "render() aborted: VTK interactor could not be created.";
                return;
        }
        try
        {
                m_toolbar->getUI().toolButtonCrop->setVisible(false);
                m_toolbar->getUI().comboBoxFilters->setVisible(false);
                hideStatusOverlay();
                startLoadingAnimation();
                m_vtkWidget->setImage(m_image);
                m_vtkWidget->setSeries(m_series);
                m_vtkWidget->setInteractor(interactor);
                m_renderTimer.start();
                Q_UNUSED(connect(this, &Widget3D::finishedRenderAsync,
                        this, &Widget3D::onFinishedRenderAsync, Qt::UniqueConnection));
                m_future = QtConcurrent::run(onRenderAsync, this);
                qCInfo(lcWidget3D) << "render() async task started" << "isRunning" << m_future.isRunning();
        }
        catch (const std::exception& ex)
        {
                qCCritical(lcWidget3D) << "render() failed" << ex.what();
        }
}

//-----------------------------------------------------------------------------
bool isis::gui::Widget3D::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::KeyPress)
	{
		auto* const keyEvent = dynamic_cast<QKeyEvent*>(event);
		const int key = keyEvent->key();
		auto* const combo = m_toolbar->getUI().comboBoxFilters;
		switch (key)
		{
		case Qt::Key_Left:
		{
			const int currentIndex = combo->currentIndex();
			combo->setCurrentIndex(!combo->currentIndex()
				? combo->count() - 1
				: currentIndex - 1);
			break;
		}
		case Qt::Key_Right:
		{
			const int currentIndex = combo->currentIndex();
			combo->setCurrentIndex(currentIndex == combo->count() - 1
				? 0
				: currentIndex + 1);
			break;
		}
		default:
			break;
		}
	}
	return QWidget::eventFilter(watched, event);
}

//-----------------------------------------------------------------------------
void isis::gui::Widget3D::onfilterChanged(const QString& t_filter) const
{
        if (m_qtvtkWidget && m_vtkWidget)
        {
                const auto seriesUid = m_series ? QString::fromStdString(m_series->getUID()) : QStringLiteral("n/a");
                qCInfo(lcWidget3D) << "Filter change requested" << t_filter
                                   << "seriesUid" << seriesUid << "imageIdx" << m_imageIndex;
                m_vtkWidget->setFilter(t_filter);
                if (auto* const renderWindow = m_qtvtkWidget->renderWindow())
                {
                        renderWindow->Render();
                }
                qCDebug(lcWidget3D) << "Filter applied" << t_filter;
        }
}

//-----------------------------------------------------------------------------
void isis::gui::Widget3D::onCropPressed(const bool& t_pressed) const
{
        qCInfo(lcWidget3D) << "Crop widget toggled" << (t_pressed ? "enabled" : "disabled")
                           << "seriesIdx" << m_seriesIndex << "imageIdx" << m_imageIndex;
        m_vtkWidget->activateBoxWidget(t_pressed);
        if (auto* const renderWindow = m_qtvtkWidget->renderWindow())
        {
                renderWindow->Render();
        }
}

//-----------------------------------------------------------------------------
void isis::gui::Widget3D::onActivateWidget(const bool& t_flag)
{
	if (t_flag)
	{
		auto* event = new QFocusEvent(QEvent::FocusIn,
			Qt::FocusReason::MouseFocusReason);
		focusInEvent(event);
		delete event;
	}
}

//-----------------------------------------------------------------------------
void isis::gui::Widget3D::onSetMaximized() const
{
	if (m_tabWidget)
	{
		dynamic_cast<TabWidget*>
			(m_tabWidget)->onMaximize();
	}
}

//-----------------------------------------------------------------------------
void isis::gui::Widget3D::onFinishedRenderAsync()
{
        Q_UNUSED(disconnect(this, &Widget3D::finishedRenderAsync,
                this, &Widget3D::onFinishedRenderAsync));
        stopLoadingAnimation();

        const QString errorMessage = m_vtkWidget->lastVolumeError();
        const QString warningMessage = m_vtkWidget->lastVolumeWarning();

        if (!m_vtkWidget->hasRenderableVolume())
        {
                const QString displayMessage = errorMessage.isEmpty()
                        ? tr("Unable to decode a 3D volume for the selected dataset.")
                        : errorMessage;
                showStatusOverlay(displayMessage, false);
                qCWarning(lcWidget3D) << "Volume rendering unavailable." << displayMessage;
                return;
        }

        hideStatusOverlay();
        if (auto* const renderWindow = m_qtvtkWidget->renderWindow())
        {
                renderWindow->Render();
        }
        onfilterChanged(m_toolbar->getUI().comboBoxFilters->itemData(0).toString());
        m_toolbar->getUI().toolButtonCrop->setVisible(true);
        m_toolbar->getUI().comboBoxFilters->setVisible(true);
        installEventFilter(this);
        if (m_renderTimer.isValid())
        {
                qCInfo(lcWidget3D)
                        << "[Telemetry] Volume composition finished"
                        << "elapsedMs" << m_renderTimer.elapsed();
        }
        if (!warningMessage.isEmpty())
        {
                showStatusOverlay(warningMessage, true);
                qCWarning(lcWidget3D) << "Volume render completed with warning." << warningMessage;
        }
        else
        {
                hideStatusOverlay();
        }
}

//-----------------------------------------------------------------------------
void isis::gui::Widget3D::onRenderAsync(Widget3D* t_self)
{
        QElapsedTimer totalTimer;
        totalTimer.start();
        QString failureReason;
        std::shared_ptr<VtkDicomVolume> volume;
        try
        {
                volume = t_self->m_vtkWidget->acquireVolume(&failureReason);
        }
        catch (const std::exception& ex)
        {
                failureReason = QString::fromUtf8(ex.what());
        }
        catch (...)
        {
                failureReason = QStringLiteral("Unexpected error while decoding series volume.");
        }
        const qint64 decodeDurationMs = totalTimer.elapsed();
        QMetaObject::invokeMethod(
                t_self,
                [t_self, volume, failureReason, decodeDurationMs]() mutable
                {
                        QElapsedTimer composeTimer;
                        composeTimer.start();
                        const bool success = t_self->m_vtkWidget->composeAndRenderVolume(volume, failureReason);
                        const qint64 composeDurationMs = composeTimer.elapsed();
                        qCInfo(lcWidget3D) << "vtkWidget3D::render completed"
                                           << "seriesIdx" << t_self->m_seriesIndex
                                           << "imageIdx" << t_self->m_imageIndex
                                           << "decodeMs" << decodeDurationMs
                                           << "composeMs" << composeDurationMs
                                           << "totalMs" << (decodeDurationMs + composeDurationMs)
                                           << "success" << success;
                        emit t_self->finishedRenderAsync();
                },
                Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
void isis::gui::Widget3D::initView()
{
	m_ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	auto* const layout = new QVBoxLayout;
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_toolbar);
	layout->addWidget(m_qtvtkWidget);
	setLayout(layout);
}

//-----------------------------------------------------------------------------
void isis::gui::Widget3D::initData()
{
	m_qtvtkWidget = new QVTKOpenGLNativeWidget(this);

	vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
	renderWindow->SetDoubleBuffer(true);
	m_renderWindow3D = renderWindow.GetPointer();
	m_qtvtkWidget->setRenderWindow(m_renderWindow3D);
	m_statusOverlay = new QLabel(this);
	m_statusOverlay->setObjectName(QStringLiteral("volumeStatusOverlay"));
	m_statusOverlay->setAlignment(Qt::AlignCenter);
	m_statusOverlay->setWordWrap(true);
	m_statusOverlay->setTextFormat(Qt::RichText);
        m_statusOverlay->setVisible(false);
        m_statusOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
	m_statusOverlay->setStyleSheet(QStringLiteral("background-color: transparent; font-size: 14px;"));
	updateStatusOverlayGeometry();
	m_vtkWidget = std::make_unique<vtkWidget3D>();
	m_vtkWidget->setRenderWindow(m_renderWindow3D);
	m_toolbar = new ToolbarWidget3D(this);
	m_vtkEvents = std::make_unique<vtkEventFilter>(this);
	setWidgetType(WidgetType::widget3d);
}

//-----------------------------------------------------------------------------
void isis::gui::Widget3D::createConnections()
{
	if (m_toolbar)
	{
		Q_UNUSED(connect(m_toolbar,
			&ToolbarWidget3D::filterChanged, this,
			&Widget3D::onfilterChanged));
		Q_UNUSED(connect(m_toolbar, &ToolbarWidget3D::cropPressed,
			this, &Widget3D::onCropPressed));
	}
	setFocusPolicy(Qt::FocusPolicy::WheelFocus);
	m_qtvtkWidget->installEventFilter(m_vtkEvents.get());
	Q_UNUSED(connect(m_vtkEvents.get(),
		&vtkEventFilter::activateWidget,
		this, &Widget3D::onActivateWidget));
	Q_UNUSED(connect(m_vtkEvents.get(),
		&vtkEventFilter::setMaximized,
		this, &Widget3D::onSetMaximized));
}

//-----------------------------------------------------------------------------
void isis::gui::Widget3D::startLoadingAnimation()
{
        if (m_loadingAnimation)
        {
                m_loadingAnimation->hide();
                m_loadingAnimation.reset();
        }
}

void isis::gui::Widget3D::positionLoadingAnimation()
{
        if (!m_loadingAnimation)
        {
                return;
        }

        const QRect targetRect = (m_qtvtkWidget && !m_qtvtkWidget->geometry().isEmpty())
                ? m_qtvtkWidget->geometry()
                : rect();
        if (targetRect.isEmpty())
        {
                return;
        }
        const QSize overlaySize = m_loadingAnimation->size();
        const QPoint desired = targetRect.center() - QPoint(overlaySize.width() / 2, overlaySize.height() / 2);
        const int minX = targetRect.left();
        const int maxX = targetRect.right() - overlaySize.width() + 1;
        const int minY = targetRect.top();
        const int maxY = targetRect.bottom() - overlaySize.height() + 1;
        const int clampedX = std::max(minX, std::min(desired.x(), maxX));
        const int clampedY = std::max(minY, std::min(desired.y(), maxY));
        m_loadingAnimation->move(clampedX, clampedY);
        qCDebug(lcLoadingAnimation) << "Widget3D overlay positioned"
                << "widget" << this
                << "targetRect" << targetRect
                << "overlaySize" << overlaySize
                << "topLeft" << QPoint(clampedX, clampedY);
}

void isis::gui::Widget3D::resizeEvent(QResizeEvent* event)
{
        QWidget::resizeEvent(event);
        positionLoadingAnimation();
        updateStatusOverlayGeometry();
}

void isis::gui::Widget3D::showStatusOverlay(const QString& message, const bool warning)
{
        if (!m_statusOverlay)
        {
                return;
        }
        const QString paletteColor = warning ? QStringLiteral("#f2c94c") : QStringLiteral("#ff6b6b");
        const QString escaped = (message.isEmpty()
                                         ? tr("An unexpected rendering condition occurred.")
                                         : message).toHtmlEscaped().replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
        const QString html = QStringLiteral(
                "<div style=\"display:inline-block; max-width:70%%; background-color: rgba(16, 16, 16, 200); "
                "color:%1; padding: 18px 26px; border-radius: 12px; font-weight:600;\">%2</div>")
                .arg(paletteColor, escaped);
        m_statusOverlay->setText(html);
        m_statusOverlay->adjustSize();
        updateStatusOverlayGeometry();
        m_statusOverlay->show();
        m_statusOverlay->raise();
}

void isis::gui::Widget3D::hideStatusOverlay()
{
        if (m_statusOverlay)
        {
                m_statusOverlay->hide();
        }
}

void isis::gui::Widget3D::updateStatusOverlayGeometry()
{
        if (!m_statusOverlay)
        {
                return;
        }
        const QRect targetRect = (m_qtvtkWidget && !m_qtvtkWidget->geometry().isEmpty())
                ? m_qtvtkWidget->geometry()
                : rect();
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

