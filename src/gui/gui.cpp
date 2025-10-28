/*
 * ------------------------------------------------------------------------------------
 *  File: gui.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements the main GUI window, wiring together importers, controllers, and widgets.
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

#include "gui.h"

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QIODevice>
#include <QLabel>
#include <QKeySequence>
#include <QMessageBox>
#include <QMetaType>
#include <QPixmap>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QLocale>
#include <QStringList>
#include <QtCore/Qt>
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

#include "core/image.h"
#include "core/patient.h"
#include "core/series.h"
#include "core/study.h"
#include "dialogs/dimsepeersdialog.h"
#include "dialogs/dimsequerywindow.h"
#include "dialogs/dimsesenddialog.h"

namespace
{
void registerFilesImporterMetaTypes()
{
        qRegisterMetaType<isis::core::Patient*>("isis::core::Patient*");
        qRegisterMetaType<isis::core::Study*>("isis::core::Study*");
        qRegisterMetaType<isis::core::Series*>("isis::core::Series*");
        qRegisterMetaType<isis::core::Image*>("isis::core::Image*");
}

QString loadStylesheetResource(const QString& resourcePath)
{
        QFile file(resourcePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
                return {};
        }
        return QString::fromUtf8(file.readAll());
}
} // namespace

isis::gui::GUI::GUI(QWidget* parent) : QMainWindow(parent)
{
        registerFilesImporterMetaTypes();
        qRegisterMetaType<Widget2D::CursorInfo>("isis::gui::Widget2D::CursorInfo");
        initView();
        initData();
        createConnections();
}

isis::gui::GUI::~GUI()
{
	if (m_filesImporter->isRunning())
	{
		m_filesImporter->stopImporter(QStringLiteral("application shutdown"));
	}
	m_widgetsController->waitForRenderingThreads();
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::initView()
{
	m_ui.setupUi(this);
        initLayoutShortcuts();
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::initLayoutShortcuts()
{
        const struct LayoutShortcutDefinition
        {
                WidgetsContainer::layouts layout = WidgetsContainer::layouts::one;
                const char* text = "";
                Qt::Key key = Qt::Key_F1;
        } definitions[] = {
                {WidgetsContainer::layouts::one, QT_TRANSLATE_NOOP("GUI", "Single viewport"), Qt::Key_F1},
                {WidgetsContainer::layouts::twoRowOneBottom, QT_TRANSLATE_NOOP("GUI", "Two horizontal viewports"), Qt::Key_F2},
                {WidgetsContainer::layouts::twoColumnOneRight, QT_TRANSLATE_NOOP("GUI", "Two vertical viewports"), Qt::Key_F3},
                {WidgetsContainer::layouts::threeRowOneBottom, QT_TRANSLATE_NOOP("GUI", "Three horizontal viewports"), Qt::Key_F4},
                {WidgetsContainer::layouts::threeColumnOneRight, QT_TRANSLATE_NOOP("GUI", "Three vertical viewports"), Qt::Key_F5}
        };

        int index = 0;
        for (const auto& definition : definitions)
        {
                auto* const action = new QAction(tr(definition.text), this);
                if (!action)
                {
                        continue;
                }
                action->setShortcut(QKeySequence(definition.key));
                action->setShortcutContext(Qt::ApplicationShortcut);
                connect(action, &QAction::triggered, this,
                        [this, layout = definition.layout]()
                        {
                                QApplication::setOverrideCursor(Qt::WaitCursor);
                                onChangeLayout(layout);
                        });
                addAction(action);
                if (index < static_cast<int>(m_layoutShortcutActions.size()))
                {
                        m_layoutShortcutActions[index] = action;
                }
                ++index;
        }
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::applyDarkTheme()
{
        WidgetsContainer* const widgetsContainer = m_widgetsController
                ? m_widgetsController->getWidgetsContainer()
                : nullptr;

        if (widgetsContainer)
        {
                widgetsContainer->setStyleSheet(QString());
                widgetsContainer->updateToolbarIconTint(QColor(230, 230, 230), QColor(140, 140, 140));
        }

        if (m_thumbnailsWidget)
        {
                m_thumbnailsWidget->applyDarkPalette();
        }

        static const QString darkThemeStylesheet =
                loadStylesheetResource(QStringLiteral(":/res/styles/isis_theme.qss"));
        if (qApp)
        {
                if (!darkThemeStylesheet.isEmpty())
                {
                        qApp->setStyleSheet(darkThemeStylesheet);
                }
                else
                {
                        qApp->setStyleSheet(QString());
                }
        }
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::initData()
{
	//create file importer
	m_filesImporter =
		std::make_shared<FilesImporter>(this);
	m_filesImporter->startImporter();

	//create DIMSE network manager
	m_dimseNetworkManager =
		std::make_unique<DimseNetworkManager>(m_filesImporter.get(), this);
	m_dimseNetworkManager->initialize();

	//create widget controller
	m_widgetsController =
		std::make_unique<WidgetsController>();
        m_widgetsController
                ->createWidgets(WidgetsContainer::layouts::one);
	m_widgetsController
		->setFilesImporter(m_filesImporter.get());
	auto* const widgetsContainer
		= m_widgetsController->getWidgetsContainer();
	setCentralWidget(widgetsContainer);
        if (widgetsContainer)
        {
                widgetsContainer->setActiveLayout(WidgetsContainer::layouts::one);
        }
        const auto containerUi = widgetsContainer->getUI();
        m_patientSummaryLabel = containerUi.labelPatientSummary;
        m_studyDateTimeLabel = containerUi.labelStudyDateTime;
        m_seriesSummaryLabel = containerUi.labelSeriesSummary;
        m_seriesHeaderLabel = containerUi.labelSeriesHeader;
        m_seriesTagLabel = containerUi.labelSeriesTag;
        m_seriesHeaderIcon = containerUi.labelSeriesIcon;
        m_seriesDetailsLabel = containerUi.labelSeriesDetails;
        if (m_seriesTagLabel)
        {
                m_seriesTagLabel->clear();
                m_seriesTagLabel->setVisible(false);
        }
        if (m_seriesHeaderIcon)
        {
                const QPixmap headerPixmap(QStringLiteral(":/res/icon_folder.png"));
                if (!headerPixmap.isNull())
                {
                        m_seriesHeaderIcon->setPixmap(headerPixmap.scaled(16, 16, Qt::KeepAspectRatio,
                                Qt::SmoothTransformation));
                }
                m_seriesHeaderIcon->setText({});
        }
        updateStudySummary();
        updateWindowTitle();
        m_widgetsController->warmUpAdvancedViewers(this);
        //create toolbars and widgets
        m_thumbnailsWidget = new ThumbnailsWidget();
        if (containerUi.patientBrowserLayout)
        {
                containerUi.patientBrowserLayout->addWidget(m_thumbnailsWidget);
        }
        else if (containerUi.patientBrowserContainer)
        {
                if (auto* const hostLayout = containerUi.patientBrowserContainer->layout())
                {
                        hostLayout->addWidget(m_thumbnailsWidget);
                }
                else
                {
                        auto* const fallbackLayout = new QVBoxLayout(containerUi.patientBrowserContainer);
                        fallbackLayout->setContentsMargins(0, 0, 0, 0);
                        fallbackLayout->setSpacing(6);
                        fallbackLayout->addWidget(m_thumbnailsWidget);
                }
        }
	else if (containerUi.widgetPatients && containerUi.widgetPatients->layout())
	{
		containerUi.widgetPatients->layout()->addWidget(m_thumbnailsWidget);
	}
	applyDarkTheme();
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::createConnections() const
{
	connectFilesImporter();
	connectFunctions();
	Q_UNUSED(connect(m_widgetsController.get(),
		&WidgetsController::seriesActivated,
		this,
		&GUI::onSeriesActivated));
	Q_UNUSED(connect(m_widgetsController.get(),
		&WidgetsController::activeFrameMetricsChanged,
		this,
		&GUI::onFrameMetricsChanged));
	Q_UNUSED(connect(m_widgetsController.get(),
		&WidgetsController::activeCursorInfoChanged,
		this,
		&GUI::onCursorInfoChanged));
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::connectFilesImporter() const
{
        constexpr auto connectionType = Qt::QueuedConnection;
        Q_UNUSED(connect(m_filesImporter.get(),
                &FilesImporter::addNewThumbnail,
                m_thumbnailsWidget,
                &ThumbnailsWidget::addThumbnail,
                connectionType));
        Q_UNUSED(connect(m_filesImporter.get(),
                &FilesImporter::populateWidget,
                m_widgetsController.get(),
                &WidgetsController::populateWidget,
                connectionType));
        Q_UNUSED(connect(m_filesImporter.get(),
                &FilesImporter::showThumbnailsWidget,
                this,
                &GUI::onShowThumbnailsWidget,
                connectionType));
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::disconnectFilesImporter() const
{
	disconnect(m_filesImporter.get(),
	           &FilesImporter::addNewThumbnail,
	           m_thumbnailsWidget,
	           &ThumbnailsWidget::addThumbnail);
	disconnect(m_filesImporter.get(),
	           &FilesImporter::populateWidget,
	           m_widgetsController.get(),
	           &WidgetsController::populateWidget);
	disconnect(m_filesImporter.get(),
	           &FilesImporter::showThumbnailsWidget,
	           this, &GUI::onShowThumbnailsWidget);
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::connectFunctions() const
{
	auto* const widgetsContainer =
		m_widgetsController->getWidgetsContainer();
	Q_UNUSED(connect(widgetsContainer,
		&WidgetsContainer::applyTransformation,
		this, &GUI::onApplyTransformation));
	Q_UNUSED(connect(widgetsContainer,
		&WidgetsContainer::closePatients,
		this, &GUI::onCloseAllPatients));
	Q_UNUSED(connect(widgetsContainer,
		&WidgetsContainer::createWidget3D,
		this, &GUI::onCreateWidget3D));
	Q_UNUSED(connect(widgetsContainer,
		&WidgetsContainer::createWidgetMPR,
		this, &GUI::onCreateWidgetMPR));
	Q_UNUSED(connect(widgetsContainer,
		&WidgetsContainer::windowPresetRequested,
		m_widgetsController.get(),
		&WidgetsController::applyWindowPreset));
	Q_UNUSED(connect(widgetsContainer,
		&WidgetsContainer::interactionToolRequested,
		m_widgetsController.get(),
		&WidgetsController::activateInteractionTool));
	Q_UNUSED(connect(m_widgetsController.get(),
		&WidgetsController::activeToolChanged,
		widgetsContainer,
		&WidgetsContainer::setActiveInteractionTool));
	Q_UNUSED(connect(m_widgetsController.get(),
		&WidgetsController::activeToolChanged,
		this,
		&GUI::onActiveToolChanged));
	Q_UNUSED(connect(widgetsContainer,
		&WidgetsContainer::openFileRequested,
		this,
		&GUI::onOpenFile));
	Q_UNUSED(connect(widgetsContainer,
		&WidgetsContainer::openFolderRequested,
		this,
		&GUI::onOpenFolder));
        Q_UNUSED(connect(widgetsContainer,
                &WidgetsContainer::queryRetrieveRequested,
                this,
                [this]()
                {
                        auto* self = const_cast<GUI*>(this);
                        if (self)
                        {
                                self->showQueryRetrieveDialog(self);
                        }
                }));
        Q_UNUSED(connect(widgetsContainer,
                &WidgetsContainer::layoutRequested,
                this,
                &GUI::onChangeLayout));
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::onOpenFile()
{
	auto* const fileDialog =
		new QFileDialog(this, "Select files");
	fileDialog->setFileMode(QFileDialog::ExistingFiles);
	if (fileDialog->exec() == QDialog::Accepted)
	{
		m_filesImporter->addFiles(fileDialog->selectedFiles());
	}
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::onOpenFolder()
{
	auto* const filedialog =
		new QFileDialog(this, "Select folder");
	filedialog->setFileMode(QFileDialog::Directory);
	filedialog->setOptions(QFileDialog::ShowDirsOnly);
	if (filedialog->exec() == QDialog::Accepted)
	{
		m_filesImporter->addFolders(filedialog->selectedFiles());
	}
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::showManagePeersDialog(QWidget* parent) const
{
        QWidget* dialogParent = parent ? parent : const_cast<GUI*>(this);

        if (!m_dimseNetworkManager)
        {
                QMessageBox::warning(dialogParent, tr("DIMSE unavailable"),
                        tr("DIMSE services are not initialized."));
                return;
        }

        auto config = m_dimseNetworkManager->getConfig();
        auto pool = m_dimseNetworkManager->getConnectionPool();
        if (!config || !pool)
        {
                QMessageBox::warning(dialogParent, tr("DIMSE unavailable"),
                        tr("DIMSE configuration is incomplete."));
                return;
        }

        dialogs::DimsePeersDialog dialog(dialogParent);
        dialog.setDimseConfig(config);
        dialog.setConnectionPool(pool);
        dialog.exec();
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::showQueryRetrieveDialog(QWidget* parent) const
{
        QWidget* dialogParent = parent ? parent : const_cast<GUI*>(this);

        if (!m_dimseNetworkManager)
        {
                QMessageBox::warning(dialogParent, tr("DIMSE unavailable"),
                        tr("DIMSE services are not initialized."));
                return;
        }

        auto config = m_dimseNetworkManager->getConfig();
        auto pool = m_dimseNetworkManager->getConnectionPool();
        if (!config || !pool)
        {
                QMessageBox::warning(dialogParent, tr("DIMSE unavailable"),
                        tr("DIMSE configuration is incomplete."));
                return;
        }

        dialogs::DimseQueryWindow dialog(dialogParent);
        dialog.setDimseConfig(config);
        dialog.setConnectionPool(pool);
        dialog.exec();
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::showSendToPacsDialog(QWidget* parent) const
{
        QWidget* dialogParent = parent ? parent : const_cast<GUI*>(this);

        if (!m_dimseNetworkManager)
        {
                QMessageBox::warning(dialogParent, tr("DIMSE unavailable"),
                        tr("DIMSE services are not initialized."));
                return;
        }

        auto config = m_dimseNetworkManager->getConfig();
        auto pool = m_dimseNetworkManager->getConnectionPool();
        auto storeService = m_dimseNetworkManager->getStoreService();
        if (!config || !pool || !storeService)
        {
                QMessageBox::warning(dialogParent, tr("DIMSE unavailable"),
                        tr("Store service is not available."));
                return;
        }

        dialogs::DimseSendDialog dialog(dialogParent);
        dialog.setDimseConfig(config);
        dialog.setConnectionPool(pool);
        dialog.setStoreService(storeService);
        dialog.exec();
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::onCreateWidget3D() const
{
	m_widgetsController->createWidgetMPR3D(WidgetBase::WidgetType::widget3d);
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::onCreateWidgetMPR() const
{
	m_widgetsController->createWidgetMPR3D(WidgetBase::WidgetType::widgetmpr);
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::onChangeLayout(const WidgetsContainer::layouts& t_layout) const
{
        if (!m_widgetsController)
        {
                QApplication::restoreOverrideCursor();
                return;
        }

        WidgetsContainer* const container = m_widgetsController->getWidgetsContainer();
        if (container)
        {
                container->setActiveLayout(t_layout);
        }
	m_widgetsController->createWidgets(t_layout);
	QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::onApplyTransformation(const transformationType& t_type) const
{
	m_widgetsController->applyTransformation(t_type);
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::onShowThumbnailsWidget(const bool& t_flag) const
{
        if (!m_widgetsController)
        {
                return;
        }

        if (auto* const container = m_widgetsController->getWidgetsContainer())
        {
                container->setPatientsPanelAvailability(t_flag);
        }
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::onRestoreThumbnailsPanel() const
{
        onShowThumbnailsWidget(true);
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::onCloseAllPatients()
{
        QApplication::setOverrideCursor(Qt::WaitCursor);
        disconnectFilesImporter();
        onShowThumbnailsWidget(false);
        m_filesImporter->stopImporter(QStringLiteral("close all patients"));
        m_widgetsController->resetData();
        m_thumbnailsWidget->resetData();
	m_filesImporter->getCoreController()->resetData();
	connectFilesImporter();
	m_filesImporter->startImporter();
        m_currentPatient = nullptr;
        m_currentStudy = nullptr;
        m_currentSeries = nullptr;
        m_currentImage = nullptr;
        m_hasFrameMetrics = false;
        m_currentFrameMetrics = {};
        updateStudySummary();
        updateWindowTitle();
	QApplication::restoreOverrideCursor();
}



//-----------------------------------------------------------------------------
void isis::gui::GUI::onSeriesActivated(core::Patient* patient, core::Study* study,
        core::Series* series, core::Image* image)
{
        m_currentPatient = patient;
        m_currentStudy = study;
        m_currentSeries = series;
        m_currentImage = image;
        m_currentFrameMetrics = {};
        m_hasFrameMetrics = false;
        m_currentCursorInfo = {};
        m_hasCursorInfo = false;
        if (m_thumbnailsWidget && series)
        {
                m_thumbnailsWidget->updateSeriesProgress(series, 0, 0);
        }
        updateStudySummary();
        updateWindowTitle();
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::onActiveToolChanged(InteractionTool tool)
{
        m_activeTool = tool;
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::onFrameMetricsChanged(const Widget2D::FrameMetrics& metrics)
{
        m_currentFrameMetrics = metrics;
        m_hasFrameMetrics = true;
        if (m_thumbnailsWidget && m_currentSeries)
        {
                m_thumbnailsWidget->updateSeriesProgress(m_currentSeries, metrics.frameIndex, metrics.frameCount);
        }
        updateWindowTitle();
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::onCursorInfoChanged(const Widget2D::CursorInfo& info)
{
        if (info.valid)
        {
                m_currentCursorInfo = info;
                m_hasCursorInfo = true;
        }
        else
        {
                m_currentCursorInfo = {};
                m_hasCursorInfo = false;
        }
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::updateStudySummary()
{
        if (!m_patientSummaryLabel || !m_studyDateTimeLabel || !m_seriesSummaryLabel || !m_seriesHeaderLabel)
        {
                return;
        }

        m_patientSummaryLabel->setText(formatPatientSummary());
        m_studyDateTimeLabel->setText(formatStudyDateTime());
        const QString seriesSummary = formatSeriesSummary();
        const QString seriesHeader = formatSeriesHeader();
        const QString tagText = formatSeriesTag();
        m_seriesSummaryLabel->setText(seriesSummary);
        m_seriesHeaderLabel->setText(seriesHeader);
        if (m_seriesTagLabel)
        {
                m_seriesTagLabel->setText(tagText);
                m_seriesTagLabel->setVisible(!tagText.isEmpty());
        }
        if (m_seriesDetailsLabel)
        {
                QString details = seriesSummary.trimmed();
                const QString trimmedTag = tagText.trimmed();
                if (!details.isEmpty() && !trimmedTag.isEmpty())
                {
                details.append(QStringLiteral("  \u2022  "));
                }
                if (!trimmedTag.isEmpty())
                {
                        details.append(trimmedTag);
                }
                m_seriesDetailsLabel->setText(details);
                m_seriesDetailsLabel->setVisible(!details.isEmpty());
        }
}

//-----------------------------------------------------------------------------
void isis::gui::GUI::updateWindowTitle()
{
        QStringList segments;
        const QString patientSummary = formatPatientSummary();
        if (!patientSummary.isEmpty())
        {
                segments << patientSummary;
        }

        const QString studyDateTime = formatStudyDateTime();
        if (!studyDateTime.isEmpty())
        {
                segments << studyDateTime;
        }

        if (m_currentSeries)
        {
                QStringList seriesSegments;
                const QString seriesNumber = QString::fromStdString(m_currentSeries->getNumber()).trimmed();
                if (!seriesNumber.isEmpty())
                {
                        seriesSegments << QStringLiteral("Serie %1").arg(seriesNumber);
                }

                if (m_hasFrameMetrics && m_currentFrameMetrics.frameCount > 0)
                {
                        const int normalizedIndex = std::clamp(m_currentFrameMetrics.frameIndex, 0,
                                m_currentFrameMetrics.frameCount - 1);
                        seriesSegments << QStringLiteral("Imagem %1/%2")
                                .arg(normalizedIndex + 1)
                                .arg(m_currentFrameMetrics.frameCount);
                }

                const QString description = QString::fromStdString(m_currentSeries->getDescription()).trimmed();
                if (!description.isEmpty())
                {
                        seriesSegments << description;
                }

                if (!seriesSegments.isEmpty())
                {
                        segments << seriesSegments.join(QStringLiteral(" "));
                }
        }

        if (segments.isEmpty())
        {
                setWindowTitle(QStringLiteral("Isis DICOM Viewer"));
                return;
        }

        setWindowTitle(segments.join(QStringLiteral(" | ")));
}

//-----------------------------------------------------------------------------
QString isis::gui::GUI::formatPatientSummary() const
{
        QStringList segments;

        QString name;
        if (m_currentPatient)
        {
                name = QString::fromStdString(m_currentPatient->getName()).trimmed();
        }
        if (name.isEmpty())
        {
                name = QStringLiteral("No patient");
        }
        segments << name;

        QString patientId;
        if (m_currentPatient)
        {
                patientId = QString::fromStdString(m_currentPatient->getID()).trimmed();
        }
        if (patientId.isEmpty())
        {
                patientId = metadataValue(m_currentSeries, 0x0010, 0x0020);
        }
        if (!patientId.isEmpty())
        {
                segments << QStringLiteral("ID %1").arg(patientId);
        }

        QStringList demographics;
        QString sex = metadataValue(m_currentSeries, 0x0010, 0x0040).trimmed().toUpper();
        if (!sex.isEmpty())
        {
                demographics << sex;
        }

        if (m_currentPatient && m_currentPatient->getAge() > 0)
        {
                demographics << QStringLiteral("%1y").arg(m_currentPatient->getAge());
        }

        if (!demographics.isEmpty())
        {
                segments << demographics.join(QStringLiteral(" / "));
        }

        return segments.join(QStringLiteral(" | "));
}

//-----------------------------------------------------------------------------
QString isis::gui::GUI::formatStudyDateTime() const
{
        QString dateValue = metadataValue(m_currentSeries, 0x0008, 0x0020);
        if (dateValue.isEmpty() && m_currentStudy)
        {
                dateValue = QString::fromStdString(m_currentStudy->getDate()).trimmed();
        }

        const QString timeValue = metadataValue(m_currentSeries, 0x0008, 0x0030);

        const QString formattedDate = formatDicomDate(dateValue);
        const QString formattedTime = formatDicomTime(timeValue);

        QStringList parts;
        if (!formattedDate.isEmpty())
        {
                parts << formattedDate;
        }
        if (!formattedTime.isEmpty())
        {
                parts << formattedTime;
        }

        return parts.isEmpty() ? QStringLiteral("Date unavailable") : parts.join(QStringLiteral(" "));
}

//-----------------------------------------------------------------------------
QString isis::gui::GUI::formatSliceTimestamp() const
{
        if (!m_currentSeries)
        {
                return QStringLiteral("--");
        }

        auto formattedFromTags = [this](unsigned short dateGroup, unsigned short dateElement,
                unsigned short timeGroup, unsigned short timeElement) -> QString
        {
                const QString dateValue = formatDicomDate(
                        metadataValue(m_currentSeries, dateGroup, dateElement));
                const QString timeValue = formatDicomTime(
                        metadataValue(m_currentSeries, timeGroup, timeElement));
                QStringList parts;
                if (!dateValue.isEmpty())
                {
                        parts << dateValue;
                }
                if (!timeValue.isEmpty())
                {
                        parts << timeValue;
                }
                return parts.join(QStringLiteral(" "));
        };

        // Prefer Acquisition Date/Time (0008,0022 / 0008,0032)
        QString formatted = formattedFromTags(0x0008, 0x0022, 0x0008, 0x0032);
        if (!formatted.isEmpty())
        {
                return formatted;
        }

        // Fallback to Content Date/Time (0008,0023 / 0008,0033)
        formatted = formattedFromTags(0x0008, 0x0023, 0x0008, 0x0033);
        if (!formatted.isEmpty())
        {
                return formatted;
        }

        // As a last resort reuse the study date/time summary.
        const QString studyFormatted = formatStudyDateTime();
        if (!studyFormatted.isEmpty() && studyFormatted != QStringLiteral("Date unavailable"))
        {
                return studyFormatted;
        }

        return QStringLiteral("--");
}

//-----------------------------------------------------------------------------
QString isis::gui::GUI::formatSeriesSummary() const
{
        QStringList parts;
        if (m_currentStudy)
        {
                const QString studyDescription = QString::fromStdString(m_currentStudy->getDescription()).trimmed();
                if (!studyDescription.isEmpty())
                {
                        parts << studyDescription;
                }
        }

        if (m_currentSeries)
        {
                const bool hasTagLabel = !formatSeriesTag().isEmpty();
                const QString modality = metadataValue(m_currentSeries, 0x0008, 0x0060).toUpper();
                if (!modality.isEmpty())
                {
                        parts << modality;
                }

                if (!hasTagLabel)
                {
                        const QString bodyPart = metadataValue(m_currentSeries, 0x0018, 0x0015);
                        if (!bodyPart.isEmpty())
                        {
                                parts << bodyPart;
                        }
                }

                const QString protocol = metadataValue(m_currentSeries, 0x0018, 0x1030);

                QStringList seriesSegments;
                const QString seriesNumber = QString::fromStdString(m_currentSeries->getNumber()).trimmed();
                const QString seriesDescription = QString::fromStdString(m_currentSeries->getDescription()).trimmed();
                if (!seriesNumber.isEmpty())
                {
                        seriesSegments << QStringLiteral("Serie %1").arg(seriesNumber);
                }
                if (!seriesDescription.isEmpty())
                {
                        seriesSegments << seriesDescription;
                }

                if (!protocol.isEmpty())
                {
                        seriesSegments << protocol;
                }

                if (!seriesSegments.isEmpty())
                {
                        parts << seriesSegments.join(QStringLiteral(" | "));
                }
        }

        if (parts.isEmpty())
        {
                return QStringLiteral("Select a series");
        }
        return parts.join(QStringLiteral(" | "));
}

//-----------------------------------------------------------------------------
QString isis::gui::GUI::formatSeriesHeader() const
{
        if (!m_currentSeries)
        {
                return QStringLiteral("No series selected");
        }

        const QString seriesNumber = QString::fromStdString(m_currentSeries->getNumber()).trimmed();
        const QString seriesDescription = QString::fromStdString(m_currentSeries->getDescription()).trimmed();
        QString header;
        if (!seriesNumber.isEmpty())
        {
                header = QStringLiteral("Serie %1").arg(seriesNumber);
        }
        if (!seriesDescription.isEmpty())
        {
                if (!header.isEmpty())
                {
                        header.append(QStringLiteral(" | "));
                }
                header.append(seriesDescription);
        }
        return header.isEmpty() ? QStringLiteral("No series selected") : header;
}

//-----------------------------------------------------------------------------
QString isis::gui::GUI::formatSeriesTag() const
{
        if (!m_currentSeries)
        {
                return {};
        }

        const auto normalizeUpper = [](const QString& value) -> QString
        {
                const QString trimmed = value.trimmed();
                return trimmed.isEmpty() ? QString() : trimmed.toUpper();
        };

        QString normalized = normalizeUpper(metadataValue(m_currentSeries, 0x0018, 0x0015));
        if (!normalized.isEmpty())
        {
                return normalized;
        }

        normalized = normalizeUpper(metadataValue(m_currentSeries, 0x0018, 0x1030));
        if (!normalized.isEmpty())
        {
                return normalized;
        }

        return {};
}

//-----------------------------------------------------------------------------
QString isis::gui::GUI::metadataValue(core::Series* series, unsigned short group, unsigned short element) const
{
        if (!series)
        {
                return {};
        }

        const auto* metadata = series->getMetadataForSeries();
        if (!metadata)
        {
                return {};
        }

        const unsigned int key = (static_cast<unsigned int>(group) << 16) | static_cast<unsigned int>(element);
        const auto it = metadata->Values.find(key);
        if (it == metadata->Values.end())
        {
                return {};
        }

        return QString::fromStdString(it->second).trimmed();
}

//-----------------------------------------------------------------------------
QString isis::gui::GUI::formatDicomDate(const QString& value) const
{
        if (value.size() < 8)
        {
                return {};
        }
        return QStringLiteral("%1/%2/%3")
                .arg(value.mid(6, 2), value.mid(4, 2), value.mid(0, 4));
}

//-----------------------------------------------------------------------------
QString isis::gui::GUI::formatDicomTime(const QString& value) const
{
        if (value.size() < 2)
        {
                return {};
        }

        QString padded = value;
        while (padded.size() < 6)
        {
                padded.append(QLatin1Char('0'));
        }

        return QStringLiteral("%1:%2:%3")
                .arg(padded.mid(0, 2), padded.mid(2, 2), padded.mid(4, 2));
}

//-----------------------------------------------------------------------------
QString isis::gui::GUI::formatSpacing(const double value) const
{
        if (value <= 0.0 || !std::isfinite(value))
        {
                return {};
        }

        const QLocale locale;
        const int precision = (value < 1.0) ? 3 : (value < 10.0 ? 2 : 1);
        return locale.toString(value, 'f', precision);
}














