/*
 * ------------------------------------------------------------------------------------
 *  File: widgetscontroller.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Manages creation, layout, and updates of the application's viewing widgets.
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

#include "widgetscontroller.h"
#include "filesimporter.h"
#include "widget2d.h"
#include "vtkwidget2d.h"
#include "widget3d.h"
#include "widgetmpr.h"

#include <QCoreApplication>
#include <QEvent>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <QVariant>
#include <QVTKOpenGLNativeWidget.h>
#include <QWidget>
#include <atomic>

Q_LOGGING_CATEGORY(lcWidgetsController, "isis.gui.widgetscontroller")

isis::gui::WidgetsController::WidgetsController()
{
        initData();
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsController::initData()
{
	m_widgetsRepository = std::make_unique<WidgetsRepository>();
	m_widgetsContainer = std::make_unique<WidgetsContainer>();
	m_widgetsContainer->setWidgetReference(&m_widgetsRepository->getWidgets());
	m_measuresManager = std::make_unique<measures::MeasuresManager>();
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsController::createWidgets(const WidgetsContainer::layouts& t_layout)
{
        resetConnections();
        createRemoveWidgets(computeNumberWidgetsFromLayout(t_layout));
        m_widgetsContainer->setLayout(t_layout);
        m_currentLayout = t_layout;
        createConnections();
        (*m_widgetsRepository->getWidgets().begin())->onFocus(true);
}

void isis::gui::WidgetsController::createWidgetMPR3D(const WidgetBase::WidgetType& t_type)
{
        if (!m_activeWidget || !m_activeWidget->getTabbedWidget()->getSeries()
                || !m_activeWidget->getTabbedWidget()->getImage())
        {
                qCWarning(lcWidgetsController)
                        << "Ignoring advanced viewer request: active widget missing data";
                return;
        }

        auto warmedWidget = takeWarmedWidget(t_type);
        const bool usingWarmed = static_cast<bool>(warmedWidget);

        qCInfo(lcWidgetsController)
                << "Creating advanced viewer on demand"
                << "type"
                << static_cast<int>(t_type)
                << "usingWarmed"
                << usingWarmed;

        m_activeWidget->createWidgetMPR3D(t_type, std::move(warmedWidget));
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsController::applyTransformation(const transformationType& t_type) const
{
        if (!m_activeWidget)
        {
                return;
        }

        if (auto* const widget2d = dynamic_cast<Widget2D*>(m_activeWidget->getTabbedWidget());
                widget2d && widget2d->getWidgetType() == WidgetBase::WidgetType::widget2d
                && widget2d->getIsImageLoaded() && widget2d->getImage())
        {
                widget2d->onApplyTransformation(t_type);
        }
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsController::applyWindowPreset(double center, double width)
{
        if (!m_activeWidget)
        {
                return;
        }

        auto* const widget2d = dynamic_cast<Widget2D*>(m_activeWidget->getTabbedWidget());
        if (!widget2d || widget2d->getWidgetType() != WidgetBase::WidgetType::widget2d)
        {
                return;
        }

        if (!widget2d->getIsImageLoaded())
        {
                return;
        }

        widget2d->applyWindowPreset(center, width);
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsController::activateInteractionTool(InteractionTool tool) const
{
        if (!m_activeWidget)
        {
                return;
        }

        auto* const widget2d = dynamic_cast<Widget2D*>(m_activeWidget->getTabbedWidget());
        if (!widget2d || widget2d->getWidgetType() != WidgetBase::WidgetType::widget2d)
        {
                return;
        }

        vtkWidget2D* vtkWidget = nullptr;
        {
                const QVariant property = widget2d->property(Widget2D::vtkWidgetPropertyName);
                const auto rawValue = property.isValid()
                        ? property.value<quintptr>()
                        : static_cast<quintptr>(0);
                if (rawValue != 0)
                {
                        vtkWidget = reinterpret_cast<vtkWidget2D*>(rawValue);
                }
        }

        auto* interactor = vtkWidget ? vtkWidget->getInteractor() : nullptr;
        if (interactor)
        {
                // Initialize measures manager if not already done
                if (m_measuresManager && m_measuresManager->getCurrentToolType() == measures::MeasureToolType::None)
                {
                        // First initialization
                        m_measuresManager->initialize(interactor);
                }

                // Map InteractionTool to MeasureToolType and activate
                switch (tool)
                {
                case InteractionTool::measureDistance:
                        m_measuresManager->activateTool(measures::MeasureToolType::Distance);
                        break;
                case InteractionTool::measureAngle:
                        m_measuresManager->activateTool(measures::MeasureToolType::Angle);
                        break;
                case InteractionTool::measureBiDimensional:
                        m_measuresManager->activateTool(measures::MeasureToolType::BiDimensional);
                        break;
                case InteractionTool::measureContour:
                        m_measuresManager->activateTool(measures::MeasureToolType::Contour);
                        break;
                default:
                        // Deactivate measurement tools for other interaction modes
                        m_measuresManager->deactivateCurrentTool();
                        break;
                }
        }
        else if (m_measuresManager)
        {
                m_measuresManager->deactivateCurrentTool();
        }

        widget2d->setActiveTool(tool);
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsController::warmUpAdvancedViewers(QWidget* contextWidget)
{
        static std::atomic_bool warmed{false};
        if (warmed.exchange(true))
        {
                qCInfo(lcWidgetsController) << "Skipping advanced viewer warm-up; already performed.";
                return;
        }

        QWidget* const warmParent = contextWidget
                ? contextWidget
                : m_widgetsContainer.get();

        if (!warmParent)
        {
                qCWarning(lcWidgetsController)
                        << "Unable to identify a parent widget for warm-up; skipping.";
                return;
        }

        qCInfo(lcWidgetsController)
                << "Pre-warming advanced viewer contexts under"
                << warmParent;

        auto ensureNativeHandles = [](QWidget* widget)
        {
                if (!widget)
                {
                        return;
                }

                widget->ensurePolished();
                widget->setAttribute(Qt::WA_DontShowOnScreen, true);
                Q_UNUSED(widget->winId());

                const auto vtkChildren = widget->findChildren<QVTKOpenGLNativeWidget*>(
                        QString(), Qt::FindChildrenRecursively);
                for (QVTKOpenGLNativeWidget* vtkChild : vtkChildren)
                {
                        if (!vtkChild)
                        {
                                continue;
                        }

                        vtkChild->setAttribute(Qt::WA_DontShowOnScreen, true);
                        vtkChild->ensurePolished();
                        Q_UNUSED(vtkChild->winId());
                }
        };

        const QString volumeLabel = tr("3D volume viewer");
        const QString mprLabel = tr("MPR viewer");
        QStringList failedWarmups = {};

        auto warmUpWidget = [&](auto widgetFactory,
                WarmedWidgetContext& storage,
                const QString& label)
        {
                if (storage.initialized)
                {
                        qCInfo(lcWidgetsController)
                                << label << "context already initialized.";
                        return true;
                }

                storage.host = std::make_unique<QWidget>(warmParent);
                QWidget* const host = storage.host.get();
                const QString sanitizedLabel = QString(label)
                        .simplified()
                        .replace(QLatin1Char(' '), QLatin1Char('_'));
                host->setObjectName(QStringLiteral("warmup_%1").arg(sanitizedLabel));
                host->setAttribute(Qt::WA_DontShowOnScreen, true);
                host->resize(2, 2);
                host->hide();

                try
                {
                        storage.widget.reset(widgetFactory(host));
                        WidgetBase* const widget = storage.widget.get();
                        if (!widget)
                        {
                                qCWarning(lcWidgetsController)
                                        << "Warm-up factory returned null widget for"
                                        << label;
                                storage.host.reset();
                                storage.widget.reset();
                                return false;
                        }

                        widget->setAttribute(Qt::WA_DontShowOnScreen, true);
                        widget->hide();
                        ensureNativeHandles(widget);
                        storage.initialized = true;
                        qCInfo(lcWidgetsController)
                                << label << "warm-up completed.";
                        return true;
                }
                catch (const std::exception& ex)
                {
                        qCCritical(lcWidgetsController)
                                << label << "warm-up failed with error" << ex.what();
                        storage.host.reset();
                        storage.widget.reset();
                        return false;
                }
                catch (...)
                {
                        qCCritical(lcWidgetsController)
                                << label << "warm-up failed with an unknown error.";
                        storage.host.reset();
                        storage.widget.reset();
                        return false;
                }
        };

        const bool volumeOk = warmUpWidget(
                [](QWidget* parent) -> WidgetBase*
                {
                        return new Widget3D(parent);
                },
                m_warmupVolumeContext,
                volumeLabel);

        if (!volumeOk)
        {
                failedWarmups << volumeLabel;
        }

        const bool mprOk = warmUpWidget(
                [](QWidget* parent) -> WidgetBase*
                {
                        return new WidgetMPR(parent);
                },
                m_warmupMprContext,
                mprLabel);

        if (!mprOk)
        {
                failedWarmups << mprLabel;
        }

        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        if (!failedWarmups.isEmpty())
        {
                const QString failureText = tr("The application could not pre-initialize the following viewers: %1.\n\nThey will be created on demand when requested. If rendering continues to fail, please restart the viewer.")
                        .arg(failedWarmups.join(tr(", ")));

                QMessageBox::warning(warmParent,
                        tr("Advanced viewer warm-up"),
                        failureText,
                        QMessageBox::StandardButton::Ok,
                        QMessageBox::StandardButton::Ok);

                qCWarning(lcWidgetsController)
                        << "Advanced viewer warm-up completed with warnings." << failureText;
                return;
        }

        qCInfo(lcWidgetsController)
                << "Advanced viewer warm-up completed successfully.";
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsController::resetData() const
{
	waitForRenderingThreads();
	const auto widgets = m_widgetsRepository->getWidgets();
	for (const auto& widget : widgets)
	{
		if (widget->getTabbedWidget()->getImage())
		{
			widget->resetWidget();
		}
	}
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsController::waitForRenderingThreads() const
{
        const auto widgets = m_widgetsRepository->getWidgets();
        for (const auto& widget : widgets)
        {
                if (auto* widget2d = dynamic_cast<Widget2D*>(widget->getTabbedWidget()))
                {
                        widget2d->waitForPendingTasks();
                }
        }
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsController::setActiveWidget(TabWidget* t_widget)
{
	if (m_activeWidget)
	{
		m_activeWidget->onFocus(false);
	}
	m_activeWidget = t_widget;
        if (!m_activeWidget)
        {
                emit seriesActivated(nullptr, nullptr, nullptr, nullptr);
                emit activeToolChanged(InteractionTool::scroll);
                return;
        }

        auto* const activeBase = m_activeWidget->getActiveTabbedWidget();
        if (auto* const widget2d = dynamic_cast<Widget2D*>(activeBase))
        {
                auto* const series = widget2d->getSeries();
                auto* const study = series ? series->getParentObject() : nullptr;
                auto* const patient = study ? study->getParentObject() : nullptr;
                emit seriesActivated(patient, study, series, widget2d->getImage());
                widget2d->forceFrameMetricsUpdate();
                emit activeToolChanged(widget2d->activeTool());
        }
        else
        {
                emit seriesActivated(nullptr, nullptr, nullptr, nullptr);
                emit activeToolChanged(InteractionTool::scroll);
        }
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsController::setMaximize(TabWidget* t_widget) const
{
	const auto widgets = m_widgetsRepository->getWidgets();
	t_widget->setIsMaximized(!t_widget->getIsMaximized());
	for (const auto& widget : widgets)
	{
		if (t_widget != widget)
		{
			widget->setVisible(!t_widget->getIsMaximized());
		}
	}
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsController::populateWidget(core::Series* t_series, core::Image* t_image)
{
        auto* const widget = findNextAvailableWidget();
        if (widget)
        {
                auto* const widget2d = dynamic_cast<Widget2D*>(widget->getActiveTabbedWidget());
                if (!widget2d)
                {
                        return;
                }
                if (!t_series || !t_image)
                {
                        qCWarning(lcWidgetsController)
                                << "populateWidget received null data from importer."
                                << "seriesMissing" << (t_series == nullptr)
                                << "imageMissing" << (t_image == nullptr);
                        widget2d->setIsImageLoaded(false);
                        return;
                }
                widget2d->setWidgetType(WidgetBase::WidgetType::widget2d);
                widget2d->setRenderRequestSource(QStringLiteral("FilesImporter::populateWidget"));
                widget2d->setSeries(t_series);
                widget2d->setImage(t_image);
                auto* const study = t_series->getParentObject();
                widget2d->setIndexes(study->getParentObject()->getIndex(),
                                     study->getIndex(), t_series->getIndex(),
                                     t_image->getIndex());
                widget2d->setIsImageLoaded(true);
                auto* const patient = study ? study->getParentObject() : nullptr;
                emit seriesActivated(patient, study, t_series, t_image);
                qCInfo(lcWidgetsController)
                        << "Requesting render for series" << QString::fromStdString(t_series->getUID())
                        << "(index" << t_series->getIndex() << ")"
                        << "image" << QString::fromStdString(t_image->getSOPInstanceUID())
                        << "(index" << t_image->getIndex() << ")"
                        << "layout" << layoutToString(m_currentLayout);
                widget2d->render();
                connectVtkToolBridge(widget2d);
                if (widget2d->wasRenderAbortedDueToMissingContext())
                {
                        qCWarning(lcWidgetsController)
                                << "Widget2D render aborted due to missing context."
                                << "seriesMissing" << (widget2d->getSeries() == nullptr)
                                << "imageMissing" << (widget2d->getImage() == nullptr)
                                << "trigger" << widget2d->getRenderRequestSource();
                        widget2d->setIsImageLoaded(false);
                        widget2d->clearRenderAbortedDueToMissingContext();
                        return;
                }
                Q_UNUSED(connect(m_filesImporter,
                        &FilesImporter::refreshScrollValues,
                        widget2d,
                        &Widget2D::onRefreshScrollValues,
                        Qt::QueuedConnection));
                widget2d->forceFrameMetricsUpdate();
        }
}

//-----------------------------------------------------------------------------
const char* isis::gui::WidgetsController::layoutToString(const WidgetsContainer::layouts& t_layout)
{
        switch (t_layout)
        {
        case WidgetsContainer::layouts::one:
                return "one";
        case WidgetsContainer::layouts::twoRowOneBottom:
                return "twoRowOneBottom";
        case WidgetsContainer::layouts::twoColumnOneRight:
                return "twoColumnOneRight";
        case WidgetsContainer::layouts::threeRowOneBottom:
                return "threeRowOneBottom";
        case WidgetsContainer::layouts::threeColumnOneRight:
                return "threeColumnOneRight";
        case WidgetsContainer::layouts::none:
        default:
                return "none";
        }
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsController::createRemoveWidgets(const std::size_t& t_nrWidgets) const
{
	while (t_nrWidgets != m_widgetsRepository->getWidgets().size())
	{
		m_widgetsRepository->getWidgets().size() > t_nrWidgets
			? m_widgetsRepository->removeWidget()
			: m_widgetsRepository->addWidget(createNewWidget());
	}
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsController::createConnections()
{
        const auto widgets = m_widgetsRepository->getWidgets();
        for (const auto& widget : widgets)
        {
                Q_UNUSED(connect(widget, &TabWidget::focused, this,
                        &WidgetsController::setActiveWidget));
                Q_UNUSED(connect(widget, &TabWidget::setMaximized, this,
                        &WidgetsController::setMaximize));
                if (auto* const widget2d = dynamic_cast<Widget2D*>(widget->getTabbedWidget()))
                {
                        connectVtkToolBridge(widget2d);
                        Q_UNUSED(connect(widget2d, &Widget2D::frameMetricsChanged, this,
                                [this, widget](const Widget2D::FrameMetrics& metrics)
                                {
                                        if (widget == m_activeWidget)
                                        {
                                                emit activeFrameMetricsChanged(metrics);
                                        }
                                }));
                        Q_UNUSED(connect(widget2d, &Widget2D::cursorInfoChanged, this,
                                [this, widget](const Widget2D::CursorInfo& info)
                                {
                                        if (widget == m_activeWidget)
                                        {
                                                emit activeCursorInfoChanged(info);
                                        }
                                }));
                        if (m_widgetsContainer)
                        {
                                Q_UNUSED(connect(widget2d,
                                        &Widget2D::windowPresetsChanged,
                                        m_widgetsContainer.get(),
                                        &WidgetsContainer::setWindowPresets,
                                        Qt::UniqueConnection));
                        }
                }
        }
}

//-----------------------------------------------------------------------------
void isis::gui::WidgetsController::resetConnections()
{
        m_activeWidget = nullptr;
        for (auto& binding : m_vtkToolConnections)
        {
                QObject::disconnect(binding.second.toolConnection);
                QObject::disconnect(binding.second.destroyedConnection);
        }
        m_vtkToolConnections.clear();
        const auto widgets = m_widgetsRepository->getWidgets();
        for (const auto& widget : widgets)
        {
                widget->onFocus(false);
                widget->setIsMaximized(false);
		widget->setVisible(true);
		disconnect(widget, &TabWidget::focused, this,
		           &WidgetsController::setActiveWidget);
		disconnect(widget, &TabWidget::setMaximized, this,
		           &WidgetsController::setMaximize);
		if (auto* const widget2d = dynamic_cast<Widget2D*>(widget->getTabbedWidget()); widget2d)
		{
			disconnect(m_filesImporter, &FilesImporter::refreshScrollValues,
			           widget2d, &Widget2D::onRefreshScrollValues);
			disconnect(widget2d, nullptr, this, nullptr);
			if (m_widgetsContainer)
			{
				disconnect(widget2d, nullptr, m_widgetsContainer.get(), nullptr);
			}
		}
	}
}

//-----------------------------------------------------------------------------
isis::gui::TabWidget* isis::gui::WidgetsController::createNewWidget() const
{
	auto* const widget = new TabWidget(m_widgetsContainer.get());
	widget->createWidget2D();
	return widget;
}

//-----------------------------------------------------------------------------
isis::gui::TabWidget* isis::gui::WidgetsController::findNextAvailableWidget() const
{
	const auto widgets = m_widgetsRepository->getWidgets();
	const auto it = std::find_if(widgets.begin(),
	                             widgets.end(), [](TabWidget* t_widget)
	                             {
		                             return !t_widget->getActiveTabbedWidget()->getIsImageLoaded();
	                             });
	return it == widgets.end() ? nullptr : *it;
}

//-----------------------------------------------------------------------------
std::size_t isis::gui::WidgetsController::computeNumberWidgetsFromLayout(const WidgetsContainer::layouts& t_layout)
{
        return t_layout == WidgetsContainer::layouts::one ? 1 : static_cast<std::size_t>(t_layout) / 2 + 2ul;
}

//-----------------------------------------------------------------------------
std::unique_ptr<isis::gui::WidgetBase> isis::gui::WidgetsController::takeWarmedWidget(
        const WidgetBase::WidgetType& t_type)
{
        WarmedWidgetContext* context = nullptr;
        switch (t_type)
        {
        case WidgetBase::WidgetType::widget3d:
                context = &m_warmupVolumeContext;
                break;
        case WidgetBase::WidgetType::widgetmpr:
                context = &m_warmupMprContext;
                break;
        default:
                return {};
        }

        if (!context->hasWidget())
        {
                return {};
        }

        return context->takeWidget();
}

void isis::gui::WidgetsController::connectVtkToolBridge(Widget2D* t_widget)
{
        if (!t_widget)
        {
                return;
        }

        QVariant property = t_widget->property(Widget2D::vtkWidgetPropertyName);
        auto rawValue = property.isValid()
                ? property.value<quintptr>()
                : static_cast<quintptr>(0);
        auto* vtkWidget = rawValue != 0
                ? reinterpret_cast<vtkWidget2D*>(rawValue)
                : nullptr;

        if (!vtkWidget)
        {
                if (auto* const candidate = vtkWidget2D::findForContext(t_widget->getSeries(), t_widget->getImage()))
                {
                        candidate->publishBridgeProperty(t_widget);
                        property = t_widget->property(Widget2D::vtkWidgetPropertyName);
                        rawValue = property.isValid()
                                ? property.value<quintptr>()
                                : static_cast<quintptr>(0);
                        vtkWidget = rawValue != 0
                                ? reinterpret_cast<vtkWidget2D*>(rawValue)
                                : nullptr;
                }
        }

        auto bindingIt = m_vtkToolConnections.find(t_widget);
        const auto removeBinding = [this, &bindingIt]()
        {
                if (bindingIt != m_vtkToolConnections.end())
                {
                        QObject::disconnect(bindingIt->second.toolConnection);
                        QObject::disconnect(bindingIt->second.destroyedConnection);
                        m_vtkToolConnections.erase(bindingIt);
                }
        };

        if (!vtkWidget)
        {
                removeBinding();
                return;
        }

        if (bindingIt != m_vtkToolConnections.end())
        {
                if (bindingIt->second.target == vtkWidget)
                {
                        return;
                }
                removeBinding();
                bindingIt = m_vtkToolConnections.end();
        }

        const auto currentTool = t_widget->activeTool();
        vtkWidget->setActiveTool(currentTool);
        emit activeToolChanged(currentTool);

        const auto toolConnection = QObject::connect(t_widget, &Widget2D::activeToolChanged,
                this, [this, vtkWidget](InteractionTool t_tool)
                {
                        vtkWidget->setActiveTool(t_tool);
                        emit activeToolChanged(t_tool);
                });
        if (!toolConnection)
        {
                return;
        }

        const auto destroyedConnection = QObject::connect(t_widget, &QObject::destroyed,
                this, [this, t_widget]()
                {
                        auto it = m_vtkToolConnections.find(t_widget);
                        if (it != m_vtkToolConnections.end())
                        {
                                QObject::disconnect(it->second.toolConnection);
                                QObject::disconnect(it->second.destroyedConnection);
                                m_vtkToolConnections.erase(it);
                        }
                });
        if (!destroyedConnection)
        {
                QObject::disconnect(toolConnection);
                return;
        }

        m_vtkToolConnections.emplace(t_widget, VtkToolBinding{toolConnection, destroyedConnection, vtkWidget});
}




