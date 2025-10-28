/*
 * ------------------------------------------------------------------------------------
 *  File: dimsequerywindow.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of DICOM Query/Retrieve window.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "dimsequerywindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QDate>
#include <QApplication>
#include <QtConcurrent>
#include <QPointer>
#include "dimsepeersdialog.h"

namespace isis::gui::dialogs
{
    DimseQueryWindow::DimseQueryWindow(QWidget* parent)
        : QDialog(parent)
    {
        setupUi();
        setWindowTitle("Query/Retrieve DICOM Studies");
        resize(1000, 700);

        // Create event manager
        m_eventManager = std::make_shared<core::events::CallbackManager>();
    }

    void DimseQueryWindow::setDimseConfig(std::shared_ptr<core::network::DimseConfig> config)
    {
        m_config = config;
        refreshPeerList();
    }

    void DimseQueryWindow::setConnectionPool(std::shared_ptr<core::network::DimseConnectionPool> pool)
    {
        m_connectionPool = pool;

        // Create services
        if (m_connectionPool && m_eventManager)
        {
            m_queryService = std::make_shared<core::network::DimseQueryService>(
                m_connectionPool, m_eventManager);
            m_retrieveService = std::make_shared<core::network::DimseRetrieveService>(
                m_connectionPool, m_eventManager);
        }
    }

    void DimseQueryWindow::setRetrieveCallback(std::function<void(const std::string&)> callback)
    {
        m_retrieveCallback = callback;
    }

    void DimseQueryWindow::setupUi()
    {
        auto* mainLayout = new QVBoxLayout(this);

        // Peer selection
        auto* peerLayout = new QHBoxLayout();
        peerLayout->addWidget(new QLabel("PACS Server:", this));
        m_peerCombo = new QComboBox(this);
        connect(m_peerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &DimseQueryWindow::onPeerChanged);
        peerLayout->addWidget(m_peerCombo, 1);
        m_managePeersButton = new QPushButton("Manage Peers...", this);
        connect(m_managePeersButton, &QPushButton::clicked, this, &DimseQueryWindow::onManagePeers);
        peerLayout->addWidget(m_managePeersButton);
        mainLayout->addLayout(peerLayout);

        // Query filters group
        auto* filtersGroup = new QGroupBox("Search Filters", this);
        auto* filtersLayout = new QFormLayout();

        m_patientNameEdit = new QLineEdit(this);
        m_patientNameEdit->setPlaceholderText("e.g., DOE^JOHN or *DOE*");
        filtersLayout->addRow("Patient Name:", m_patientNameEdit);

        m_patientIdEdit = new QLineEdit(this);
        filtersLayout->addRow("Patient ID:", m_patientIdEdit);

        auto* dateLayout = new QHBoxLayout();
        m_studyDateFromEdit = new QDateEdit(this);
        m_studyDateFromEdit->setDate(QDate::currentDate().addMonths(-6));
        m_studyDateFromEdit->setCalendarPopup(true);
        dateLayout->addWidget(m_studyDateFromEdit);
        dateLayout->addWidget(new QLabel("to", this));
        m_studyDateToEdit = new QDateEdit(this);
        m_studyDateToEdit->setDate(QDate::currentDate());
        m_studyDateToEdit->setCalendarPopup(true);
        dateLayout->addWidget(m_studyDateToEdit);
        dateLayout->addStretch();
        filtersLayout->addRow("Study Date:", dateLayout);

        m_studyDescEdit = new QLineEdit(this);
        filtersLayout->addRow("Study Description:", m_studyDescEdit);

        m_accessionEdit = new QLineEdit(this);
        filtersLayout->addRow("Accession Number:", m_accessionEdit);

        // Modality checkboxes
        auto* modalityLayout = new QHBoxLayout();
        m_modalityCT = new QCheckBox("CT", this);
        m_modalityMR = new QCheckBox("MR", this);
        m_modalityUS = new QCheckBox("US", this);
        m_modalityXA = new QCheckBox("XA", this);
        m_modalityCR = new QCheckBox("CR", this);
        modalityLayout->addWidget(m_modalityCT);
        modalityLayout->addWidget(m_modalityMR);
        modalityLayout->addWidget(m_modalityUS);
        modalityLayout->addWidget(m_modalityXA);
        modalityLayout->addWidget(m_modalityCR);
        modalityLayout->addStretch();
        filtersLayout->addRow("Modalities:", modalityLayout);

        m_maxResultsSpin = new QSpinBox(this);
        m_maxResultsSpin->setRange(1, 1000);
        m_maxResultsSpin->setValue(100);
        filtersLayout->addRow("Max Results:", m_maxResultsSpin);

        filtersGroup->setLayout(filtersLayout);
        mainLayout->addWidget(filtersGroup);

        // Query buttons
        auto* queryButtonLayout = new QHBoxLayout();
        m_queryButton = new QPushButton("Search", this);
        m_queryButton->setDefault(true);
        connect(m_queryButton, &QPushButton::clicked, this, &DimseQueryWindow::onQuery);
        queryButtonLayout->addWidget(m_queryButton);

        m_clearButton = new QPushButton("Clear Filters", this);
        connect(m_clearButton, &QPushButton::clicked, this, &DimseQueryWindow::onClearFilters);
        queryButtonLayout->addWidget(m_clearButton);

        queryButtonLayout->addStretch();
        mainLayout->addLayout(queryButtonLayout);

        // Results table
        auto* resultsGroup = new QGroupBox("Search Results", this);
        auto* resultsLayout = new QVBoxLayout();

        m_resultsTable = new QTableWidget(this);
        m_resultsTable->setColumnCount(8);
        m_resultsTable->setHorizontalHeaderLabels({
            "Patient Name", "Patient ID", "Study Date", "Study Description",
            "Accession #", "Modality", "# Series", "# Images"
        });
        m_resultsTable->horizontalHeader()->setStretchLastSection(true);
        m_resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_resultsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        connect(m_resultsTable, &QTableWidget::cellDoubleClicked,
                this, &DimseQueryWindow::onResultDoubleClicked);
        connect(m_resultsTable, &QTableWidget::itemSelectionChanged,
                this, &DimseQueryWindow::onResultSelectionChanged);

        resultsLayout->addWidget(m_resultsTable);

        // Status bar
        auto* statusLayout = new QHBoxLayout();
        m_statusLabel = new QLabel("Ready", this);
        statusLayout->addWidget(m_statusLabel);
        m_progressBar = new QProgressBar(this);
        m_progressBar->setVisible(false);
        statusLayout->addWidget(m_progressBar);
        resultsLayout->addLayout(statusLayout);

        resultsGroup->setLayout(resultsLayout);
        mainLayout->addWidget(resultsGroup, 1);

        // Action buttons
        auto* actionButtonLayout = new QHBoxLayout();

        m_retrieveButton = new QPushButton("Retrieve Selected", this);
        m_retrieveButton->setEnabled(false);
        connect(m_retrieveButton, &QPushButton::clicked, this, &DimseQueryWindow::onRetrieve);
        actionButtonLayout->addWidget(m_retrieveButton);

        m_retrieveAllButton = new QPushButton("Retrieve All", this);
        m_retrieveAllButton->setEnabled(false);
        connect(m_retrieveAllButton, &QPushButton::clicked, this, &DimseQueryWindow::onRetrieveAll);
        actionButtonLayout->addWidget(m_retrieveAllButton);

        actionButtonLayout->addStretch();

        m_cancelButton = new QPushButton("Close", this);
        connect(m_cancelButton, &QPushButton::clicked, this, &DimseQueryWindow::onCancel);
        actionButtonLayout->addWidget(m_cancelButton);

        mainLayout->addLayout(actionButtonLayout);
    }

    void DimseQueryWindow::refreshPeerList(const QString& preferredPeerId)
    {
        if (!m_config || !m_peerCombo)
            return;

        QString targetId = preferredPeerId;
        if (targetId.isEmpty() && m_peerCombo->currentIndex() >= 0)
        {
            targetId = m_peerCombo->currentData().toString();
        }

        m_peerCombo->clear();

        auto peers = m_config->getEnabledPeers();
        int indexToSelect = -1;
        int row = 0;
        for (const auto& peer : peers)
        {
            const QString peerId = QString::fromStdString(peer.id);
            m_peerCombo->addItem(
                QString::fromStdString(peer.name + " (" + peer.aeTitle + ")"),
                peerId);
            if (!targetId.isEmpty() && peerId == targetId)
            {
                indexToSelect = row;
            }
            ++row;
        }

        if (indexToSelect >= 0)
        {
            m_peerCombo->setCurrentIndex(indexToSelect);
        }
        else if (m_peerCombo->count() > 0)
        {
            m_peerCombo->setCurrentIndex(0);
        }
    }

    void DimseQueryWindow::onPeerChanged(int index)
    {
        Q_UNUSED(index);
        clearResults();
    }

    void DimseQueryWindow::onQuery()
    {
        if (!m_config || !m_queryService || m_queryInProgress)
            return;

        if (m_peerCombo->currentIndex() < 0)
        {
            QMessageBox::warning(this, "No Peer Selected", "Please select a PACS server");
            return;
        }

        QString peerId = m_peerCombo->currentData().toString();
        const auto* peer = m_config->getPeer(peerId.toStdString());
        if (!peer)
            return;

        // Get query filter
        auto filter = getQueryFilter();

        // Clear previous results
        clearResults();
        m_queryInProgress = true;
        setQueryEnabled(false);
        updateProgressStatus("Searching...");
        m_progressBar->setVisible(true);
        m_progressBar->setRange(0, 0); // Indeterminate

        // Perform query asynchronously
        auto future = QtConcurrent::run([this, peer, filter]() {
            std::vector<core::network::RemoteStudyInfo> results;
            auto status = m_queryService->queryStudies(*peer, m_config->getLocalAEConfig(),
                                                       filter, results);
            return std::make_pair(status, results);
        });

        // Wait for completion with event processing
        while (!future.isFinished())
        {
            QApplication::processEvents();
        }

        auto [status, results] = future.result();

        m_queryInProgress = false;
        setQueryEnabled(true);
        m_progressBar->setVisible(false);

        if (status == core::network::DimseStatus::Success)
        {
            populateResults(results);
            updateProgressStatus(QString("Found %1 studies").arg(results.size()));
        }
        else
        {
            QString errorMsg = QString::fromStdString(m_queryService->getLastError());
            updateProgressStatus("Query failed");
            QMessageBox::warning(this, "Query Failed", "Failed to query PACS:\n" + errorMsg);
        }
    }

    void DimseQueryWindow::onClearFilters()
    {
        m_patientNameEdit->clear();
        m_patientIdEdit->clear();
        m_studyDateFromEdit->setDate(QDate::currentDate().addMonths(-6));
        m_studyDateToEdit->setDate(QDate::currentDate());
        m_studyDescEdit->clear();
        m_accessionEdit->clear();
        m_modalityCT->setChecked(false);
        m_modalityMR->setChecked(false);
        m_modalityUS->setChecked(false);
        m_modalityXA->setChecked(false);
        m_modalityCR->setChecked(false);
        m_maxResultsSpin->setValue(100);
        clearResults();
    }

    void DimseQueryWindow::onRetrieve()
    {
        auto selectedRows = getSelectedStudyRows();
        if (selectedRows.empty())
            return;

        for (int row : selectedRows)
        {
            if (row >= 0 && row < static_cast<int>(m_queryResults.size()))
            {
                retrieveStudy(m_queryResults[row]);
            }
        }
    }

    void DimseQueryWindow::onRetrieveAll()
    {
        if (m_queryResults.empty())
            return;

        auto reply = QMessageBox::question(this, "Confirm Retrieve All",
            QString("Are you sure you want to retrieve all %1 studies?").arg(m_queryResults.size()),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes)
        {
            for (const auto& study : m_queryResults)
            {
                retrieveStudy(study);
            }
        }
    }

    void DimseQueryWindow::onCancel()
    {
        reject();
    }

    void DimseQueryWindow::onResultSelectionChanged()
    {
        bool hasSelection = !m_resultsTable->selectedItems().isEmpty();
        m_retrieveButton->setEnabled(hasSelection);
        m_retrieveAllButton->setEnabled(!m_queryResults.empty());
    }

    void DimseQueryWindow::onResultDoubleClicked(int row, int column)
    {
        Q_UNUSED(column);
        if (row >= 0 && row < static_cast<int>(m_queryResults.size()))
        {
            retrieveStudy(m_queryResults[row]);
        }
    }

    void DimseQueryWindow::onManagePeers()
    {
        if (!m_config)
        {
            QMessageBox::warning(this, "DIMSE Configuration",
                                 "DIMSE configuration is not available.");
            return;
        }

        QString currentPeerId;
        if (m_peerCombo && m_peerCombo->currentIndex() >= 0)
        {
            currentPeerId = m_peerCombo->currentData().toString();
        }

        DimsePeersDialog dialog(this);
        dialog.setDimseConfig(m_config);
        dialog.setConnectionPool(m_connectionPool);
        dialog.exec();

        refreshPeerList(currentPeerId);
    }

    void DimseQueryWindow::populateResults(const std::vector<core::network::RemoteStudyInfo>& studies)
    {
        m_queryResults = studies;
        m_resultsTable->setRowCount(0);

        for (const auto& study : studies)
        {
            int row = m_resultsTable->rowCount();
            m_resultsTable->insertRow(row);

            m_resultsTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(study.patientName)));
            m_resultsTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(study.patientID)));
            m_resultsTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(study.studyDate)));
            m_resultsTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(study.studyDescription)));
            m_resultsTable->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(study.accessionNumber)));
            m_resultsTable->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(study.modality)));
            m_resultsTable->setItem(row, 6, new QTableWidgetItem(QString::number(study.numberOfSeries)));
            m_resultsTable->setItem(row, 7, new QTableWidgetItem(QString::number(study.numberOfInstances)));
        }

        m_resultsTable->resizeColumnsToContents();
    }

    void DimseQueryWindow::clearResults()
    {
        m_queryResults.clear();
        m_resultsTable->setRowCount(0);
        m_retrieveButton->setEnabled(false);
        m_retrieveAllButton->setEnabled(false);
        updateProgressStatus("Ready");
    }

    core::network::QueryFilter DimseQueryWindow::getQueryFilter()
    {
        core::network::QueryFilter filter;

        filter.patientName = m_patientNameEdit->text().trimmed().toStdString();
        filter.patientID = m_patientIdEdit->text().trimmed().toStdString();
        filter.studyDescription = m_studyDescEdit->text().trimmed().toStdString();
        filter.accessionNumber = m_accessionEdit->text().trimmed().toStdString();

        // Format date range
        QString dateFrom = m_studyDateFromEdit->date().toString("yyyyMMdd");
        QString dateTo = m_studyDateToEdit->date().toString("yyyyMMdd");
        filter.studyDate = (dateFrom + "-" + dateTo).toStdString();

        // Get selected modalities
        if (m_modalityCT->isChecked()) filter.modalities.push_back("CT");
        if (m_modalityMR->isChecked()) filter.modalities.push_back("MR");
        if (m_modalityUS->isChecked()) filter.modalities.push_back("US");
        if (m_modalityXA->isChecked()) filter.modalities.push_back("XA");
        if (m_modalityCR->isChecked()) filter.modalities.push_back("CR");

        filter.maxResults = m_maxResultsSpin->value();

        return filter;
    }

    std::vector<int> DimseQueryWindow::getSelectedStudyRows()
    {
        std::vector<int> rows;
        auto selectedItems = m_resultsTable->selectedItems();

        QSet<int> uniqueRows;
        for (auto* item : selectedItems)
        {
            uniqueRows.insert(item->row());
        }

        rows.reserve(uniqueRows.size());
        for (int row : uniqueRows)
        {
            rows.push_back(row);
        }

        return rows;
    }

    void DimseQueryWindow::retrieveStudy(const core::network::RemoteStudyInfo& study)
    {
        if (!m_config || !m_retrieveService)
            return;

        if (m_peerCombo->currentIndex() < 0)
            return;

        QString peerId = m_peerCombo->currentData().toString();
        const auto* peer = m_config->getPeer(peerId.toStdString());
        if (!peer)
            return;

        updateProgressStatus(QString("Retrieving study for %1...")
            .arg(QString::fromStdString(study.patientName)));

        // Copy data needed on the worker thread so we don't touch `this` after destruction.
        auto retrieveService = m_retrieveService;
        auto localAE = m_config->getLocalAEConfig();
        auto peerCopy = *peer;

        // Start retrieve asynchronously
        QPointer<DimseQueryWindow> self(this);
        QtConcurrent::run([self, retrieveService, localAE, peerCopy, study]() {
            if (!retrieveService)
            {
                return core::network::DimseStatus::Failure;
            }
            return retrieveService->retrieveStudy(
                peerCopy,
                localAE,
                study.studyInstanceUID,
                [self, study](double progress, const std::string& msg) {
                    Q_UNUSED(msg);
                    if (!self)
                    {
                        return;
                    }
                    const QString statusMsg = QString("Retrieving %1 - %2%")
                                                  .arg(QString::fromStdString(study.patientName))
                                                  .arg(static_cast<int>(progress * 100));
                    QMetaObject::invokeMethod(
                        self.data(),
                        "updateProgressStatus",
                        Qt::QueuedConnection,
                        Q_ARG(QString, statusMsg));
                });
        });

        // Note: In a real implementation, you'd want to track this future
        // and handle the result properly, possibly with a progress dialog

        QMessageBox::information(this, "Retrieve Started",
            QString("Started retrieving study for %1\nFiles will be imported automatically when received.")
                .arg(QString::fromStdString(study.patientName)));
    }

    void DimseQueryWindow::updateProgressStatus(const QString& message)
    {
        m_statusLabel->setText(message);
    }

    void DimseQueryWindow::setQueryEnabled(bool enabled)
    {
        m_queryButton->setEnabled(enabled);
        m_clearButton->setEnabled(enabled);
        m_peerCombo->setEnabled(enabled);
        m_patientNameEdit->setEnabled(enabled);
        m_patientIdEdit->setEnabled(enabled);
        m_studyDateFromEdit->setEnabled(enabled);
        m_studyDateToEdit->setEnabled(enabled);
        m_studyDescEdit->setEnabled(enabled);
        m_accessionEdit->setEnabled(enabled);
        m_modalityCT->setEnabled(enabled);
        m_modalityMR->setEnabled(enabled);
        m_modalityUS->setEnabled(enabled);
        m_modalityXA->setEnabled(enabled);
        m_modalityCR->setEnabled(enabled);
        m_maxResultsSpin->setEnabled(enabled);
    }

} // namespace isis::gui::dialogs
