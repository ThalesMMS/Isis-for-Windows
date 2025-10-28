/*
 * ------------------------------------------------------------------------------------
 *  File: widgetscontroller.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the WidgetsController coordinating render widgets, data, and toolbar state.
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
#include <QObject>
#include <QWidget>
#include <memory>
#include <unordered_map>


#include "widgetscontainer.h"
#include "widgetsrepository.h"
#include "widget3d.h"
#include "widgetmpr.h"
#include "widgetbase.h"
#include "widget2d.h"
#include "measures/measuresmanager.h"

namespace isis::core
{
        class Patient;
        class Study;
        class Series;
        class Image;
}

namespace isis::gui
{
        class FilesImporter;
        class Widget2D;
        class vtkWidget2D;
        class WidgetsController final : public QObject
        {
        Q_OBJECT
        public:
                WidgetsController();
		~WidgetsController() = default;


		//getters
		[[nodiscard]] WidgetsRepository* getWidgetsRepository() const { return m_widgetsRepository.get(); }
		[[nodiscard]] WidgetsContainer* getWidgetsContainer() const { return m_widgetsContainer.get(); }
		[[nodiscard]] TabWidget* getActiveWidget() const { return m_activeWidget; }

		//setters
		void setFilesImporter(FilesImporter* t_importer) { m_filesImporter = t_importer; }

		void createWidgets(const WidgetsContainer::layouts& t_layout);
                void createWidgetMPR3D(const WidgetBase::WidgetType& t_type);
		void applyTransformation(const transformationType& t_type) const;
		void warmUpAdvancedViewers(QWidget* contextWidget);
		void resetData() const;
		void waitForRenderingThreads() const;
		
        public slots:
                void setActiveWidget(TabWidget* t_widget);
                void setMaximize(TabWidget* t_widget) const;
                void populateWidget(core::Series* t_series, core::Image* t_image);
		void applyWindowPreset(double center, double width);
                void activateInteractionTool(InteractionTool tool) const;

        signals:
                void seriesActivated(core::Patient* patient, core::Study* study,
                        core::Series* series, core::Image* image);
		void activeFrameMetricsChanged(const Widget2D::FrameMetrics& metrics);
		void activeCursorInfoChanged(const Widget2D::CursorInfo& info);
                void activeToolChanged(InteractionTool tool);

        private:
                struct VtkToolBinding
                {
                        QMetaObject::Connection toolConnection = {};
                        QMetaObject::Connection destroyedConnection = {};
                        vtkWidget2D* target = nullptr;
                };

                std::unique_ptr<WidgetsRepository> m_widgetsRepository = {};
                std::unique_ptr<WidgetsContainer> m_widgetsContainer = {};
                std::unique_ptr<measures::MeasuresManager> m_measuresManager = {};
                TabWidget* m_activeWidget = {};
                FilesImporter* m_filesImporter = {};
                WidgetsContainer::layouts m_currentLayout = WidgetsContainer::layouts::none;
                std::unordered_map<Widget2D*, VtkToolBinding> m_vtkToolConnections = {};

                struct WarmedWidgetContext
                {
                        std::unique_ptr<QWidget> host = {};
                        std::unique_ptr<WidgetBase> widget = {};
                        bool initialized = false;

                        [[nodiscard]] bool hasWidget() const noexcept
                        {
                                return static_cast<bool>(widget);
                        }

                        std::unique_ptr<WidgetBase> takeWidget()
                        {
                                if (widget)
                                {
                                        widget->setParent(nullptr);
                                }
                                initialized = false;
                                host.reset();
                                return std::move(widget);
                        }
                };

                WarmedWidgetContext m_warmupVolumeContext = {};
                WarmedWidgetContext m_warmupMprContext = {};

                void initData();
                void createRemoveWidgets(const std::size_t& t_nrWidgets) const;
                void createConnections();
                void resetConnections();
                [[nodiscard]] TabWidget* createNewWidget() const;
                [[nodiscard]] TabWidget* findNextAvailableWidget() const;
                void connectVtkToolBridge(Widget2D* t_widget);
                [[nodiscard]] std::size_t computeNumberWidgetsFromLayout(const WidgetsContainer::layouts& t_layout);
                [[nodiscard]] static const char* layoutToString(const WidgetsContainer::layouts& t_layout);
                [[nodiscard]] std::unique_ptr<WidgetBase> takeWarmedWidget(const WidgetBase::WidgetType& t_type);
        };
}


