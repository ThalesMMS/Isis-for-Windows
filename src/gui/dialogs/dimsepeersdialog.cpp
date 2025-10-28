/*
 * ------------------------------------------------------------------------------------
 *  File: dimsepeersdialog.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of DICOM peer management dialog.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "dimsepeersdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QProgressDialog>
#include <QApplication>
#include <QtConcurrent>

namespace isis::gui::dialogs
{
    DimsePeersDialog::DimsePeersDialog(QWidget* parent)
        : QDialog(parent)
    {
        setupUi();
        setWindowTitle("Manage PACS Peers");
        resize(800, 600);
    }

    void DimsePeersDialog::setDimseConfig(std::shared_ptr<core::network::DimseConfig> config)
    {
        m_config = config;
        refreshPeerTable();
    }

    void DimsePeersDialog::setConnectionPool(std::shared_ptr<core::network::DimseConnectionPool> pool)
    {
        m_connectionPool = pool;
    }

    void DimsePeersDialog::setupUi()
    {
        auto* mainLayout = new QVBoxLayout(this);

        // Peer table
        m_peerTable = new QTableWidget(this);
        m_peerTable->setColumnCount(6);
        m_peerTable->setHorizontalHeaderLabels({
            "Name", "AE Title", "Hostname", "Port", "Enabled", "ID"
        });
        m_peerTable->horizontalHeader()->setStretchLastSection(true);
        m_peerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_peerTable->setSelectionMode(QAbstractItemView::SingleSelection);
        m_peerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        connect(m_peerTable, &QTableWidget::itemSelectionChanged,
                this, &DimsePeersDialog::onPeerSelectionChanged);

        mainLayout->addWidget(m_peerTable);

        // Peer form group box
        auto* formGroup = new QGroupBox("Peer Configuration", this);
        auto* formLayout = new QFormLayout();

        m_nameEdit = new QLineEdit(this);
        formLayout->addRow("Name:", m_nameEdit);

        m_aeTitleEdit = new QLineEdit(this);
        m_aeTitleEdit->setMaxLength(16);
        formLayout->addRow("AE Title:", m_aeTitleEdit);

        m_hostnameEdit = new QLineEdit(this);
        formLayout->addRow("Hostname/IP:", m_hostnameEdit);

        m_portSpin = new QSpinBox(this);
        m_portSpin->setRange(1, 65535);
        m_portSpin->setValue(104);
        formLayout->addRow("Port:", m_portSpin);

        m_timeoutSpin = new QSpinBox(this);
        m_timeoutSpin->setRange(5, 300);
        m_timeoutSpin->setValue(30);
        m_timeoutSpin->setSuffix(" sec");
        formLayout->addRow("Timeout:", m_timeoutSpin);

        m_enabledCheck = new QCheckBox("Enabled", this);
        m_enabledCheck->setChecked(true);
        formLayout->addRow("Status:", m_enabledCheck);

        formGroup->setLayout(formLayout);
        mainLayout->addWidget(formGroup);

        // Button layout
        auto* buttonLayout = new QHBoxLayout();

        m_addButton = new QPushButton("Add Peer", this);
        connect(m_addButton, &QPushButton::clicked, this, &DimsePeersDialog::onAddPeer);
        buttonLayout->addWidget(m_addButton);

        m_editButton = new QPushButton("Update Peer", this);
        m_editButton->setEnabled(false);
        connect(m_editButton, &QPushButton::clicked, this, &DimsePeersDialog::onEditPeer);
        buttonLayout->addWidget(m_editButton);

        m_removeButton = new QPushButton("Remove Peer", this);
        m_removeButton->setEnabled(false);
        connect(m_removeButton, &QPushButton::clicked, this, &DimsePeersDialog::onRemovePeer);
        buttonLayout->addWidget(m_removeButton);

        m_testButton = new QPushButton("Test Connection", this);
        m_testButton->setEnabled(false);
        connect(m_testButton, &QPushButton::clicked, this, &DimsePeersDialog::onTestConnection);
        buttonLayout->addWidget(m_testButton);

        buttonLayout->addStretch();

        m_importButton = new QPushButton("Import...", this);
        connect(m_importButton, &QPushButton::clicked, this, &DimsePeersDialog::onImportConfig);
        buttonLayout->addWidget(m_importButton);

        m_exportButton = new QPushButton("Export...", this);
        connect(m_exportButton, &QPushButton::clicked, this, &DimsePeersDialog::onExportConfig);
        buttonLayout->addWidget(m_exportButton);

        mainLayout->addLayout(buttonLayout);

        // Dialog buttons
        auto* dialogButtonLayout = new QHBoxLayout();
        dialogButtonLayout->addStretch();

        m_saveButton = new QPushButton("Save && Close", this);
        connect(m_saveButton, &QPushButton::clicked, this, &DimsePeersDialog::onSaveChanges);
        dialogButtonLayout->addWidget(m_saveButton);

        m_cancelButton = new QPushButton("Cancel", this);
        connect(m_cancelButton, &QPushButton::clicked, this, &DimsePeersDialog::onCancel);
        dialogButtonLayout->addWidget(m_cancelButton);

        mainLayout->addLayout(dialogButtonLayout);
    }

    void DimsePeersDialog::refreshPeerTable()
    {
        if (!m_config)
            return;

        m_peerTable->setRowCount(0);

        auto peers = m_config->getAllPeers();
        for (const auto& peer : peers)
        {
            int row = m_peerTable->rowCount();
            m_peerTable->insertRow(row);

            m_peerTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(peer.name)));
            m_peerTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(peer.aeTitle)));
            m_peerTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(peer.hostname)));
            m_peerTable->setItem(row, 3, new QTableWidgetItem(QString::number(peer.port)));
            m_peerTable->setItem(row, 4, new QTableWidgetItem(peer.enabled ? "Yes" : "No"));
            m_peerTable->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(peer.id)));
        }
    }

    void DimsePeersDialog::onAddPeer()
    {
        QString errorMsg;
        if (!validatePeerInput(errorMsg))
        {
            QMessageBox::warning(this, "Invalid Input", errorMsg);
            return;
        }

        auto peer = getPeerFromForm();
        if (m_config->addPeer(peer))
        {
            refreshPeerTable();
            clearPeerForm();
            QMessageBox::information(this, "Success", "Peer added successfully");
        }
        else
        {
            QMessageBox::warning(this, "Error", "Failed to add peer. It may already exist.");
        }
    }

    void DimsePeersDialog::onEditPeer()
    {
        if (m_editingPeerId.empty())
            return;

        QString errorMsg;
        if (!validatePeerInput(errorMsg))
        {
            QMessageBox::warning(this, "Invalid Input", errorMsg);
            return;
        }

        auto peer = getPeerFromForm();
        peer.id = m_editingPeerId; // Keep the same ID

        if (m_config->updatePeer(peer))
        {
            refreshPeerTable();
            clearPeerForm();
            m_editingPeerId.clear();
            m_editButton->setEnabled(false);
            QMessageBox::information(this, "Success", "Peer updated successfully");
        }
        else
        {
            QMessageBox::warning(this, "Error", "Failed to update peer");
        }
    }

    void DimsePeersDialog::onRemovePeer()
    {
        auto selectedItems = m_peerTable->selectedItems();
        if (selectedItems.isEmpty())
            return;

        int row = selectedItems.first()->row();
        QString peerId = m_peerTable->item(row, 5)->text();
        QString peerName = m_peerTable->item(row, 0)->text();

        auto reply = QMessageBox::question(this, "Confirm Removal",
            QString("Are you sure you want to remove peer '%1'?").arg(peerName),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes)
        {
            if (m_config->removePeer(peerId.toStdString()))
            {
                refreshPeerTable();
                clearPeerForm();
                QMessageBox::information(this, "Success", "Peer removed successfully");
            }
            else
            {
                QMessageBox::warning(this, "Error", "Failed to remove peer");
            }
        }
    }

    void DimsePeersDialog::onTestConnection()
    {
        auto selectedItems = m_peerTable->selectedItems();
        if (selectedItems.isEmpty())
            return;

        int row = selectedItems.first()->row();
        QString peerId = m_peerTable->item(row, 5)->text();

        const auto* peer = m_config->getPeer(peerId.toStdString());
        if (!peer || !m_connectionPool)
        {
            QMessageBox::warning(this, "Error", "Invalid peer or connection pool");
            return;
        }

        // Create progress dialog
        QProgressDialog progress("Testing connection...", "Cancel", 0, 0, this);
        progress.setWindowModality(Qt::WindowModal);
        progress.show();

        // Create event manager for this operation
        if (!m_eventManager)
        {
            m_eventManager = std::make_shared<core::events::CallbackManager>();
        }

        // Perform echo asynchronously
        auto echoService = std::make_shared<core::network::DimseEchoService>(
            m_connectionPool, m_eventManager);

        auto future = QtConcurrent::run([echoService, peer, this]() {
            return echoService->performEcho(*peer, m_config->getLocalAEConfig());
        });

        // Wait for completion
        while (!future.isFinished())
        {
            QApplication::processEvents();
            if (progress.wasCanceled())
            {
                QMessageBox::information(this, "Cancelled", "Connection test cancelled");
                return;
            }
        }

        progress.close();

        auto status = future.result();
        if (status == core::network::DimseStatus::Success)
        {
            QMessageBox::information(this, "Success",
                QString("Connection to '%1' successful!").arg(QString::fromStdString(peer->name)));
        }
        else
        {
            QString errorMsg = QString::fromStdString(echoService->getLastError());
            QMessageBox::warning(this, "Connection Failed",
                QString("Failed to connect to '%1':\n%2")
                    .arg(QString::fromStdString(peer->name))
                    .arg(errorMsg));
        }
    }

    void DimsePeersDialog::onImportConfig()
    {
        QString filename = QFileDialog::getOpenFileName(this,
            "Import Configuration", "", "JSON Files (*.json)");

        if (filename.isEmpty())
            return;

        if (m_config->loadFromFile(filename.toStdString()))
        {
            refreshPeerTable();
            QMessageBox::information(this, "Success", "Configuration imported successfully");
        }
        else
        {
            QMessageBox::warning(this, "Error", "Failed to import configuration");
        }
    }

    void DimsePeersDialog::onExportConfig()
    {
        QString filename = QFileDialog::getSaveFileName(this,
            "Export Configuration", "dimse_config.json", "JSON Files (*.json)");

        if (filename.isEmpty())
            return;

        if (m_config->saveToFile(filename.toStdString()))
        {
            QMessageBox::information(this, "Success", "Configuration exported successfully");
        }
        else
        {
            QMessageBox::warning(this, "Error", "Failed to export configuration");
        }
    }

    void DimsePeersDialog::onSaveChanges()
    {
        if (m_config && m_config->saveToDefault())
        {
            accept();
        }
        else
        {
            QMessageBox::warning(this, "Error", "Failed to save configuration");
        }
    }

    void DimsePeersDialog::onCancel()
    {
        // Reload config from file to discard changes
        if (m_config)
        {
            m_config->loadFromDefault();
        }
        reject();
    }

    void DimsePeersDialog::onPeerSelectionChanged()
    {
        auto selectedItems = m_peerTable->selectedItems();
        bool hasSelection = !selectedItems.isEmpty();

        m_editButton->setEnabled(hasSelection);
        m_removeButton->setEnabled(hasSelection);
        m_testButton->setEnabled(hasSelection && m_connectionPool);

        if (hasSelection)
        {
            int row = selectedItems.first()->row();
            QString peerId = m_peerTable->item(row, 5)->text();
            const auto* peer = m_config->getPeer(peerId.toStdString());
            if (peer)
            {
                loadPeerToForm(*peer);
                m_editingPeerId = peer->id;
            }
        }
        else
        {
            clearPeerForm();
            m_editingPeerId.clear();
        }
    }

    bool DimsePeersDialog::validatePeerInput(QString& errorMsg)
    {
        if (m_nameEdit->text().trimmed().isEmpty())
        {
            errorMsg = "Peer name cannot be empty";
            return false;
        }

        QString aeTitle = m_aeTitleEdit->text().trimmed();
        if (!core::network::DimseConfig::validateAETitle(aeTitle.toStdString()))
        {
            errorMsg = "Invalid AE Title. Must be 1-16 alphanumeric characters, underscore, or hyphen.";
            return false;
        }

        QString hostname = m_hostnameEdit->text().trimmed();
        if (!core::network::DimseConfig::validateHostname(hostname.toStdString()))
        {
            errorMsg = "Invalid hostname or IP address";
            return false;
        }

        if (!core::network::DimseConfig::validatePort(m_portSpin->value()))
        {
            errorMsg = "Invalid port number";
            return false;
        }

        return true;
    }

    void DimsePeersDialog::clearPeerForm()
    {
        m_nameEdit->clear();
        m_aeTitleEdit->clear();
        m_hostnameEdit->clear();
        m_portSpin->setValue(104);
        m_timeoutSpin->setValue(30);
        m_enabledCheck->setChecked(true);
        m_editingPeerId.clear();
    }

    void DimsePeersDialog::loadPeerToForm(const core::network::DicomPeer& peer)
    {
        m_nameEdit->setText(QString::fromStdString(peer.name));
        m_aeTitleEdit->setText(QString::fromStdString(peer.aeTitle));
        m_hostnameEdit->setText(QString::fromStdString(peer.hostname));
        m_portSpin->setValue(peer.port);
        m_timeoutSpin->setValue(peer.timeout);
        m_enabledCheck->setChecked(peer.enabled);
    }

    core::network::DicomPeer DimsePeersDialog::getPeerFromForm()
    {
        core::network::DicomPeer peer;

        peer.name = m_nameEdit->text().trimmed().toStdString();
        peer.aeTitle = m_aeTitleEdit->text().trimmed().toStdString();
        peer.hostname = m_hostnameEdit->text().trimmed().toStdString();
        peer.port = m_portSpin->value();
        peer.timeout = m_timeoutSpin->value();
        peer.enabled = m_enabledCheck->isChecked();

        // Generate ID if not editing
        if (m_editingPeerId.empty())
        {
            peer.id = peer.aeTitle + "@" + peer.hostname;
        }
        else
        {
            peer.id = m_editingPeerId;
        }

        return peer;
    }

} // namespace isis::gui::dialogs
