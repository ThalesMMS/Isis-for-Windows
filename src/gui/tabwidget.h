/*
 * ------------------------------------------------------------------------------------
 *  File: tabwidget.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the TabWidget class providing the main tabbed navigation for the viewer.
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

#include "ui_tabwidget.h"
#include "widgetbase.h"

#include <memory>

namespace isis::gui
{
	class Widget2D;
	class TabWidget final : public QWidget
	{
	Q_OBJECT
	public:
		explicit TabWidget(QWidget* parent);
		~TabWidget() = default;

		void createWidget2D();
                void createWidgetMPR3D(const WidgetBase::WidgetType& t_type,
                        std::unique_ptr<WidgetBase> warmedWidget = {});
		void resetWidget();

		//getters
		[[nodiscard]] WidgetBase::WidgetType getWidgetType() const;
		[[nodiscard]] WidgetBase* getTabbedWidget() const { return m_tabbedWidget; }
		[[nodiscard]] WidgetBase* getActiveTabbedWidget() const;
		[[nodiscard]] bool getIsActive() const { return m_isActive; }
		[[nodiscard]] bool getIsMaximized() const { return m_isMaximized; }

		//setters
		void setIsActive(const bool& t_flag) { m_isActive = t_flag; }
		void setIsMaximized(const bool& t_flag) { m_isMaximized = t_flag; }
		void setTabTitle(const int& t_index, const QString& t_name) const;

	public slots:
		void onFocus(const bool& t_flag);
		void onMaximize();
		void closeWidget(const int& t_index) const;
		
	signals:
		void focused(TabWidget* t_widget);
		void setMaximized(TabWidget* t_widget);

	protected:
		void focusInEvent(QFocusEvent* event) override;
		void dragEnterEvent(QDragEnterEvent* event) override;
		void dragLeaveEvent(QDragLeaveEvent* event) override;
		void dropEvent(QDropEvent* event) override;

	private:
		Ui::TabWidget m_ui = {};
		WidgetBase* m_tabbedWidget = {};
		bool m_isActive = false;
		bool m_isMaximized = false;

		void updateTabBarVisibility() const;

		void populateWidget(core::Series* t_series, core::Image* t_image);
		[[nodiscard]] std::tuple<core::Series*, core::Image*> getDropData(const QString& t_data);
		[[nodiscard]] bool canCreateWidgetMPR3D() const;
	};
}

