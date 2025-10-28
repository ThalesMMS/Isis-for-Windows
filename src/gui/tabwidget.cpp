/*
 * ------------------------------------------------------------------------------------
 *  File: tabwidget.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements the tabbed container that hosts patient, study, and series views in the GUI.
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

#include "tabwidget.h"
#include <QFocusEvent>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QMimeData>
#include <QVTKOpenGLNativeWidget.h>
#include <QTabBar>
#include <QWidget>
#include <QString>
#include <QtGlobal>
#include <memory>
#include <qstyle.h>
#include <study.h>
#include <patient.h>
#include "widget2d.h"
#include "widget3d.h"
#include "widgetmpr.h"

Q_LOGGING_CATEGORY(lcTabWidget, "isis.gui.tabwidget")

isis::gui::TabWidget::TabWidget(QWidget* parent)
        : QWidget(parent)
{
	m_ui.setupUi(this);
	dynamic_cast<QTabWidget*>
		(findChild<QTabWidget*>("tab"))->
		setStyleSheet(inActiveTabStyle);
	setStyleSheet(inActiveTabStyle);
	setAcceptDrops(true);
	Q_UNUSED(connect(m_ui.tab, &QTabWidget::tabCloseRequested,
		this, &isis::gui::TabWidget::closeWidget));
	updateTabBarVisibility();
}

//-----------------------------------------------------------------------------
void isis::gui::TabWidget::createWidget2D()
{
	delete m_tabbedWidget;
	m_tabbedWidget = new Widget2D(this);
	m_tabbedWidget->setWidgetType(WidgetBase::WidgetType::widget2d);
	m_ui.tab->addTab(m_tabbedWidget, {}, "2D");
	m_ui.tab->setTabsClosable(true);
	m_ui.tab->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);
	m_ui.tab->tabBar()->setTabButton(0, QTabBar::LeftSide, nullptr);
	updateTabBarVisibility();
}

//-----------------------------------------------------------------------------
void isis::gui::TabWidget::createWidgetMPR3D(const WidgetBase::WidgetType& t_type,
        std::unique_ptr<WidgetBase> warmedWidget)
{
        if (!canCreateWidgetMPR3D())
        {
                qCWarning(lcTabWidget)
                        << "createWidgetMPR3D aborted: current 2D widget is not ready.";
                return;
        }

        std::unique_ptr<WidgetBase> widget = std::move(warmedWidget);
        const bool usingWarmed = static_cast<bool>(widget);
        if (!widget)
        {
                if (t_type == WidgetBase::WidgetType::widget3d)
                {
                        widget = std::make_unique<Widget3D>(this);
                }
                else
                {
                        widget = std::make_unique<WidgetMPR>(this);
                }
        }
        else
        {
                widget->setParent(this);
        }

        widget->setTabWidget(this);

        auto ensureNativeHandles = [](WidgetBase* candidate)
        {
                if (!candidate)
                {
                        return;
                }

                candidate->ensurePolished();
                Q_UNUSED(candidate->winId());

                const auto vtkChildren = candidate->findChildren<QVTKOpenGLNativeWidget*>(
                        QString(), Qt::FindChildrenRecursively);
                for (QVTKOpenGLNativeWidget* vtkChild : vtkChildren)
                {
                        if (!vtkChild)
                        {
                                continue;
                        }

                        vtkChild->ensurePolished();
                        Q_UNUSED(vtkChild->winId());
                }
        };

        auto clearDontShowAttribute = [](WidgetBase* candidate)
        {
                if (!candidate)
                {
                        return;
                }

                candidate->setAttribute(Qt::WA_DontShowOnScreen, false);
                const auto vtkChildren = candidate->findChildren<QVTKOpenGLNativeWidget*>(
                        QString(), Qt::FindChildrenRecursively);
                for (QVTKOpenGLNativeWidget* vtkChild : vtkChildren)
                {
                        if (!vtkChild)
                        {
                                continue;
                        }

                        vtkChild->setAttribute(Qt::WA_DontShowOnScreen, false);
                }
        };

        if (usingWarmed)
        {
                clearDontShowAttribute(widget.get());
        }
        else
        {
                ensureNativeHandles(widget.get());
        }

        const QString name = (t_type == WidgetBase::WidgetType::widget3d)
                ? QStringLiteral("Volume")
                : QStringLiteral("MPR");
        widget->setSeries(m_tabbedWidget->getSeries());
        widget->setImage(m_tabbedWidget->getImage());
        widget->setVisible(true);
        widget->raise();
        widget->render();
        const int tabIndex = m_ui.tab->addTab(widget.get(), name);
        m_ui.tab->setCurrentIndex(tabIndex);
        updateTabBarVisibility();

        qCInfo(lcTabWidget)
                << "Added advanced viewer tab"
                << name
                << "index"
                << tabIndex;

        widget.release();
}

//-----------------------------------------------------------------------------
void isis::gui::TabWidget::resetWidget()
{
	auto* const widget = dynamic_cast<Widget2D*>(m_tabbedWidget);
	widget->waitForPendingTasks();
	delete m_tabbedWidget;
	m_tabbedWidget = nullptr;
	while (m_ui.tab->count())
	{
		delete m_ui.tab->widget(0);
	}
	createWidget2D();
	updateTabBarVisibility();
}

//-----------------------------------------------------------------------------
isis::gui::WidgetBase::WidgetType isis::gui::TabWidget::getWidgetType() const
{
	return m_tabbedWidget
		? m_tabbedWidget->getWidgetType()
		: WidgetBase::WidgetType::none;
}

//-----------------------------------------------------------------------------
isis::gui::WidgetBase* isis::gui::TabWidget::getActiveTabbedWidget() const
{
	return dynamic_cast<WidgetBase*>(m_ui.tab->widget(m_ui.tab->currentIndex()));
}

//-----------------------------------------------------------------------------
void isis::gui::TabWidget::setTabTitle(const int& t_index, const QString& t_name) const
{
	m_ui.tab->setTabText(t_index, t_name);
}

//-----------------------------------------------------------------------------
void isis::gui::TabWidget::onFocus(const bool& t_flag)
{
	m_isActive = t_flag;
	auto* const tab =
		dynamic_cast<QTabWidget*>(findChild<QTabWidget*>("tab"));
	tab->setProperty("active", t_flag);
	if (m_isActive)
	{
		tab->setStyleSheet(activeTabStyle);
		setStyleSheet(activeTabStyle);
		emit focused(this);
	}
	else
	{
		tab->setStyleSheet(inActiveTabStyle);
		setStyleSheet(inActiveTabStyle);
	}
	style()->unpolish(tab);
	style()->polish(tab);
	update();
}

//-----------------------------------------------------------------------------
void isis::gui::TabWidget::onMaximize()
{
	emit setMaximized(this);
}

//-----------------------------------------------------------------------------
void isis::gui::TabWidget::closeWidget(const int& t_index) const
{
	auto* const widget = m_ui.tab->widget(t_index);
	m_ui.tab->removeTab(t_index);
	delete widget;
	updateTabBarVisibility();
}

//-----------------------------------------------------------------------------
void isis::gui::TabWidget::focusInEvent(QFocusEvent* event)
{
	if (event->reason() == Qt::FocusReason::MouseFocusReason
		&& !m_isActive)
	{
		onFocus(true);
	}
	QWidget::focusInEvent(event);
}

//-----------------------------------------------------------------------------
void isis::gui::TabWidget::dragEnterEvent(QDragEnterEvent* event)
{
	event->accept();
}

//-----------------------------------------------------------------------------
void isis::gui::TabWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
	event->accept();
}

//-----------------------------------------------------------------------------
void isis::gui::TabWidget::dropEvent(QDropEvent* event)
{
	const QString data = event->mimeData()->text();
	if (data.isEmpty())
	{
		return;
	}
	auto const [series, image] = getDropData(data);
	if (!series || !image)
	{
		return;
	}
	resetWidget();
	populateWidget(series, image);
}

//-----------------------------------------------------------------------------
void isis::gui::TabWidget::populateWidget(core::Series* t_series, core::Image* t_image)
{
	auto* const widget = dynamic_cast<Widget2D*>(m_tabbedWidget);
	if (!widget)
	{
		return;
	}
	widget->setSeries(t_series);
	widget->setImage(t_image);
	auto* const study = t_series->getParentObject();
	widget->setIndexes(study->getParentObject()->getIndex(),
		study->getIndex(), t_series->getIndex(),
		t_image->getIndex());
	widget->setIsImageLoaded(true);
	widget->render();
	if (!m_isActive)
	{
		auto* ev = new QFocusEvent(QEvent::FocusIn,
			Qt::FocusReason::MouseFocusReason);
		focusInEvent(ev);
		delete ev;
	}
}

//-----------------------------------------------------------------------------
std::tuple<isis::core::Series*, isis::core::Image*> isis::gui::TabWidget::getDropData(
	const QString& t_data)
{
	const auto jsonDoc = QJsonDocument::fromJson(t_data.toUtf8());
	const auto obj = jsonDoc.object();
	return std::make_tuple(
		reinterpret_cast<core::Series*>(obj["Series"].toString().toULongLong(nullptr, 16)),
		reinterpret_cast<core::Image*>(obj["Image"].toString().toULongLong(nullptr, 16)));
}

//-----------------------------------------------------------------------------
bool isis::gui::TabWidget::canCreateWidgetMPR3D() const
{
	auto* const series = m_tabbedWidget->getSeries();
        return m_tabbedWidget->getImage() &&
                (series->getSinlgeFrameImages().size() >= 2
			|| !series->getMultiFrameImages().empty());
}

//-----------------------------------------------------------------------------
void isis::gui::TabWidget::updateTabBarVisibility() const
{
	if (!m_ui.tab)
	{
		return;
	}
	if (auto* const bar = m_ui.tab->tabBar())
	{
		const bool shouldShow = m_ui.tab->count() > 1;
		bar->setVisible(shouldShow);
		if (!shouldShow)
		{
			bar->setMinimumHeight(0);
			bar->setMaximumHeight(0);
		}
		else
		{
			bar->setMaximumHeight(QWIDGETSIZE_MAX);
		}
	}
}

