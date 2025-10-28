/*
 * ------------------------------------------------------------------------------------
 *  File: gui.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the GUI class, the primary window of the Isis DICOM Viewer application.
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

#include <array>
#include <memory>
#include "widget2d.h"
#include "filesimporter.h"
#include "thumbnailswidget.h"
#include "ui_gui.h"
#include "widgetscontroller.h"
#include "dimsenetworkmanager.h"

class QLabel;
class QAction;

namespace isis::core
{
        class Patient;
        class Study;
        class Series;
        class Image;
}

namespace isis::gui
{
	class GUI final : public QMainWindow
	{
	Q_OBJECT

public:
		explicit GUI(QWidget* parent = Q_NULLPTR);
		~GUI();

		void onChangeLayout(const WidgetsContainer::layouts& t_layout) const;
		void onCloseAllPatients();
		void onOpenFile();
		void onOpenFolder();
                void showManagePeersDialog(QWidget* parent) const;
                void showQueryRetrieveDialog(QWidget* parent) const;
                void showSendToPacsDialog(QWidget* parent) const;

		// DIMSE Network access
		DimseNetworkManager* getDimseNetworkManager() const { return m_dimseNetworkManager.get(); }
		
		
        public slots:
                void onApplyTransformation(const transformationType& t_type) const;
                void onShowThumbnailsWidget(const bool& t_flag) const;
                void onRestoreThumbnailsPanel() const;
		void onCreateWidget3D() const;
		void onCreateWidgetMPR() const;
		void onSeriesActivated(core::Patient* patient, core::Study* study,
                        core::Series* series, core::Image* image);
                void onActiveToolChanged(InteractionTool tool);
		void onFrameMetricsChanged(const Widget2D::FrameMetrics& metrics);
		void onCursorInfoChanged(const Widget2D::CursorInfo& info);
	
	private:
		Ui::guiClass m_ui = {};
		std::shared_ptr<FilesImporter> m_filesImporter = {};
		std::unique_ptr<WidgetsController> m_widgetsController = {};
		std::unique_ptr<DimseNetworkManager> m_dimseNetworkManager = {};
		ThumbnailsWidget* m_thumbnailsWidget = {};
		QLabel* m_patientSummaryLabel = {};
		QLabel* m_studyDateTimeLabel = {};
		QLabel* m_seriesSummaryLabel = {};
		QLabel* m_seriesHeaderLabel = {};
		QLabel* m_seriesHeaderIcon = {};
		QLabel* m_seriesTagLabel = {};
                QLabel* m_seriesDetailsLabel = {};
		core::Patient* m_currentPatient = {};
		core::Study* m_currentStudy = {};
		core::Series* m_currentSeries = {};
		core::Image* m_currentImage = {};
		Widget2D::FrameMetrics m_currentFrameMetrics = {};
		bool m_hasFrameMetrics = false;
		Widget2D::CursorInfo m_currentCursorInfo = {};
		bool m_hasCursorInfo = false;
                InteractionTool m_activeTool = InteractionTool::scroll;
                std::array<QAction*, 5> m_layoutShortcutActions = {};

		void initView();
		void initData();
		void createConnections() const;
		void connectFilesImporter() const;
		void disconnectFilesImporter() const;
		void connectFunctions() const;
                void initLayoutShortcuts();
                void applyDarkTheme();
                void updateStudySummary();
                void updateWindowTitle();
                QString formatPatientSummary() const;
                QString formatStudyDateTime() const;
                QString formatSeriesSummary() const;
                QString formatSeriesHeader() const;
                QString formatSeriesTag() const;
                QString formatDicomDate(const QString& value) const;
                QString formatDicomTime(const QString& value) const;
                QString metadataValue(core::Series* series, unsigned short group, unsigned short element) const;
                QString formatSpacing(double value) const;
                QString formatSliceTimestamp() const;
	};
}
