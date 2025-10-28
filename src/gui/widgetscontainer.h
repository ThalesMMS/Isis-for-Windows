/*
 * ------------------------------------------------------------------------------------
 *  File: widgetscontainer.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the WidgetsContainer that owns widget layouts and navigation helpers.
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

#include <QColor>
#include <QHash>
#include <QIcon>
#include "tabwidget.h"
#include "widget2d.h"
#include "ui_widgetscontainer.h"

QT_BEGIN_NAMESPACE
class QButtonGroup;
class QAction;
class QActionGroup;
class QMenu;
class QToolButton;
QT_END_NAMESPACE

namespace isis::gui
{
	class WidgetsContainer final : public QWidget
	{
	Q_OBJECT
	public:
		explicit WidgetsContainer(QWidget* t_parent = Q_NULLPTR);
		~WidgetsContainer() = default;

		enum class layouts
		{
			none,
			one,
			twoRowOneBottom,
			twoColumnOneRight,
			threeRowOneBottom,
			threeColumnOneRight
		};

		//getters
		Ui::WidgetsContainer getUI() const { return m_ui; }

		//setters
		void setWidgetReference(std::vector<TabWidget*>* t_widgetsReference) {
			m_widgetsReference = t_widgetsReference; }

		void setLayout(const layouts& t_layout);
                void updateToolbarIconTint(const QColor& normalTint, const QColor& disabledTint);
                void setActiveInteractionTool(InteractionTool tool);
                void setPatientsPanelAvailability(bool available);
                void setActiveLayout(layouts layout);
		
signals:
	void applyTransformation(const transformationType& t_type);
	void closePatients();
	void createWidget3D();
	void createWidgetMPR();
	void windowPresetRequested(double center, double width);
                void interactionToolRequested(InteractionTool tool);
                void openFileRequested();
                void openFolderRequested();
                void queryRetrieveRequested();
                void layoutRequested(const layouts& layout);

	public slots:
                void setWindowPresets(const QVector<Widget2D::WindowPreset>& presets,
                        double activeCenter,
                        double activeWidth);

	private slots:
		void onApplyTransformation();
		void onClosePatients();
		void onCreateWidget3D();
		void onCreateWidgetMPR();
		void onWindowPresetChanged(int index);
                void onNavigationToolChanged(int id);
                void onTogglePatientsPanel();

	private:
                struct PresetDefinition
                {
                        QString label;
                        double center = 0.0;
                        double width = 0.0;
                };

		Ui::WidgetsContainer m_ui = {};
		std::vector<TabWidget*>* m_widgetsReference = {};
                QHash<QToolButton*, QIcon> m_toolbarBaseIcons = {};
                QButtonGroup* m_navigationGroup = {};
                bool m_patientsPanelCollapsed = false;
                bool m_patientsPanelAvailable = true;
                QMenu* m_layoutMenu = {};
                QActionGroup* m_layoutActionGroup = {};
                QHash<layouts, QAction*> m_layoutActions = {};
                QColor m_toolbarTintNormal;
                QColor m_toolbarTintDisabled;
                QVector<PresetDefinition> m_staticWindowPresets = {};
                QVector<Widget2D::WindowPreset> m_dicomWindowPresets = {};
                double m_lastActiveWindowCenter = 0.0;
                double m_lastActiveWindowWidth = 0.0;

		void initView();
		void setProperties();
                void refreshPresetHighlight();
                void setPatientsPanelCollapsed(bool collapsed);
                void initLayoutControls();
                void onLayoutActionTriggered(QAction* action);
                void rebuildWindowPresetCombo();
                int findPresetIndex(double center, double width) const;

		void one() const;
		void twoRowOneBottom() const;
		void twoColumnOneRight() const;
		void threeRowOneBottom() const;
		void threeColumnOneRight() const;
	};
}

