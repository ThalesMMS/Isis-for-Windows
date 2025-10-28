/*
 * ------------------------------------------------------------------------------------
 *  File: dimsesenddialog.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of Send to PACS dialog.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "dimsesenddialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QLoggingCategory>
#include <QApplication>
#include <QtConcurrent>

namespace isis::gui::dialogs
{
    DimseSendDialog::DimseSendDialog(QWidget* parent)
        : QDialog(parent)
    {
        setupUI();
        setWindowTitle("Send to PACS");
        resize(600, 500);
    }

    void DimseSendDialog::setupUI()
    {
        auto* mainLayout = new QVBoxLayout(this);

        // Destination group
        auto* destGroup = new QGroupBox("Destination PACS", this);
        auto* destLayout = new QVBoxLayout(destGroup);

        m_peerCombo = new QComboBox(this);
        connect(m_peerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &DimseSendDialog::onPeerChanged);
        destLayout->addWidget(m_peerCombo);

        mainLayout->addWidget(destGroup);

        // Files group
        auto* filesGroup = new QGroupBox("Files to Send", this);
        auto* filesLayout = new QVBoxLayout(filesGroup);

        m_filesCountLabel = new QLabel("0 files selected", this);
        filesLayout->addWidget(m_filesCountLabel);

        m_filesList = new QListWidget(this);
        m_filesList->setSelectionMode(QAbstractItemView::ExtendedSelection);
        filesLayout->addWidget(m_filesList);

        auto* filesButtonLayout = new QHBoxLayout();
        m_addFilesBtn = new QPushButton("Add Files...", this);
        connect(m_addFilesBtn, &QPushButton::clicked, this, &DimseSendDialog::onAddFiles);
        filesButtonLayout->addWidget(m_addFilesBtn);

        m_addDirBtn = new QPushButton("Add Directory...", this);
        connect(m_addDirBtn, &QPushButton::clicked, this, &DimseSendDialog::onAddDirectory);
        filesButtonLayout->addWidget(m_addDirBtn);

        m_removeBtn = new QPushButton("Remove Selected", this);
        connect(m_removeBtn, &QPushButton::clicked, this, &DimseSendDialog::onRemoveFiles);
        filesButtonLayout->addWidget(m_removeBtn);

        m_clearBtn = new QPushButton("Clear All", this);
        connect(m_clearBtn, &QPushButton::clicked, this, &DimseSendDialog::onClearFiles);
        filesButtonLayout->addWidget(m_clearBtn);

        filesLayout->addLayout(filesButtonLayout);
        mainLayout->addWidget(filesGroup);

        // Progress section
        m_progressBar = new QProgressBar(this);
        m_progressBar->setVisible(false);
        mainLayout->addWidget(m_progressBar);

        m_statusLabel = new QLabel("Ready", this);
        m_statusLabel->setWordWrap(true);
        mainLayout->addWidget(m_statusLabel);

        // Buttons
        auto* buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();

        m_sendBtn = new QPushButton("Send to PACS", this);
        m_sendBtn->setDefault(true);
        connect(m_sendBtn, &QPushButton::clicked, this, &DimseSendDialog::onSend);
        buttonLayout->addWidget(m_sendBtn);

        m_cancelBtn = new QPushButton("Close", this);
        connect(m_cancelBtn, &QPushButton::clicked, this, &DimseSendDialog::onCancel);
        buttonLayout->addWidget(m_cancelBtn);

        mainLayout->addLayout(buttonLayout);

        updateUI();
    }

    void DimseSendDialog::setDimseConfig(std::shared_ptr<core::network::DimseConfig> config)
    {
        m_config = config;
        updatePeersList();
    }

    void DimseSendDialog::setConnectionPool(std::shared_ptr<core::network::DimseConnectionPool> pool)
    {
        m_connectionPool = pool;
    }

    void DimseSendDialog::setStoreService(std::shared_ptr<core::network::DimseStoreService> service)
    {
        m_storeService = service;
    }

    void DimseSendDialog::setInitialFiles(const QStringList& files)
    {
        m_files = files;
        m_filesList->clear();
        m_filesList->addItems(files);
        updateUI();
    }

    void DimseSendDialog::updatePeersList()
    {
        m_peerCombo->clear();

        if (!m_config)
        {
            return;
        }

        auto peers = m_config->getEnabledPeers();
        for (const auto& peer : peers)
        {
            if (peer.enabled)
            {
                QString itemText = QString::fromStdString(peer.name) + " (" +
                                  QString::fromStdString(peer.aeTitle) + ")";
                m_peerCombo->addItem(itemText, QString::fromStdString(peer.id));
            }
        }

        updateUI();
    }

    void DimseSendDialog::onAddFiles()
    {
        QStringList files = QFileDialog::getOpenFileNames(
            this,
            "Select DICOM Files",
            QString(),
            "DICOM Files (*.dcm *.dicom *.DCM *.DICOM);;All Files (*)");

        if (!files.isEmpty())
        {
            for (const auto& file : files)
            {
                if (!m_files.contains(file))
                {
                    m_files.append(file);
                    m_filesList->addItem(file);
                }
            }
            updateUI();
        }
    }

    void DimseSendDialog::onAddDirectory()
    {
        QString dir = QFileDialog::getExistingDirectory(
            this,
            "Select Directory with DICOM Files");

        if (!dir.isEmpty())
        {
            // Show progress while scanning
            m_statusLabel->setText("Scanning directory for DICOM files...");
            QApplication::processEvents();

            QDir directory(dir);
            QStringList filters;
            filters << "*.dcm" << "*.dicom" << "*.DCM" << "*.DICOM";

            QFileInfoList fileList = directory.entryInfoList(
                filters,
                QDir::Files,
                QDir::Name);

            int addedCount = 0;
            for (const auto& fileInfo : fileList)
            {
                QString filepath = fileInfo.absoluteFilePath();
                if (!m_files.contains(filepath))
                {
                    m_files.append(filepath);
                    m_filesList->addItem(filepath);
                    addedCount++;
                }
            }

            m_statusLabel->setText(QString("Added %1 files from directory").arg(addedCount));
            updateUI();
        }
    }

    void DimseSendDialog::onRemoveFiles()
    {
        auto selectedItems = m_filesList->selectedItems();
        for (auto* item : selectedItems)
        {
            QString filepath = item->text();
            m_files.removeAll(filepath);
            delete m_filesList->takeItem(m_filesList->row(item));
        }
        updateUI();
    }

    void DimseSendDialog::onClearFiles()
    {
        m_files.clear();
        m_filesList->clear();
        updateUI();
    }

    void DimseSendDialog::updateUI()
    {
        bool hasConfig = (m_config != nullptr);
        bool hasPeers = (m_peerCombo->count() > 0);
        bool hasFiles = (!m_files.isEmpty());

        m_sendBtn->setEnabled(!m_sending && hasConfig && hasPeers && hasFiles);
        m_addFilesBtn->setEnabled(!m_sending);
        m_addDirBtn->setEnabled(!m_sending);
        m_removeBtn->setEnabled(!m_sending && hasFiles);
        m_clearBtn->setEnabled(!m_sending && hasFiles);
        m_peerCombo->setEnabled(!m_sending);

        m_filesCountLabel->setText(QString("%1 file(s) selected").arg(m_files.size()));

        if (!hasConfig)
        {
            m_statusLabel->setText("Error: DIMSE configuration not available");
        }
        else if (!hasPeers)
        {
            m_statusLabel->setText("No PACS servers configured. Please add peers in Manage PACS Peers dialog.");
        }
        else if (!hasFiles)
        {
            m_statusLabel->setText("Ready - add files to send");
        }
        else if (!m_sending)
        {
            m_statusLabel->setText("Ready to send");
        }
    }

    void DimseSendDialog::onPeerChanged(int index)
    {
        updateUI();
    }

    bool DimseSendDialog::validateInputs()
    {
        if (!m_config)
        {
            QMessageBox::critical(this, "Error", "DIMSE configuration not available");
            return false;
        }

        if (m_peerCombo->currentIndex() < 0)
        {
            QMessageBox::warning(this, "No Peer Selected",
                               "Please select a destination PACS server");
            return false;
        }

        if (m_files.isEmpty())
        {
            QMessageBox::warning(this, "No Files",
                               "Please add DICOM files to send");
            return false;
        }

        if (!m_storeService)
        {
            QMessageBox::critical(this, "Error", "Store service not available");
            return false;
        }

        return true;
    }

    void DimseSendDialog::onSend()
    {
        if (!validateInputs())
        {
            return;
        }

        // Confirm with user
        const QString peerDisplay = m_peerCombo->currentText();
        int result = QMessageBox::question(
            this,
            "Confirm Send",
            QString("Send %1 file(s) to %2?").arg(m_files.size()).arg(peerDisplay),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);

        if (result != QMessageBox::Yes)
        {
            return;
        }

        startSending();
    }

    void DimseSendDialog::startSending()
    {
        m_sending = true;
        m_progressBar->setValue(0);
        m_progressBar->setVisible(true);
        m_statusLabel->setText("Preparing to send...");
        updateUI();

        // Get selected peer
        QString peerId = m_peerCombo->currentData().toString();
        if (peerId.isEmpty())
        {
            QMessageBox::critical(this, "Error", "Failed to get peer configuration");
            m_sending = false;
            updateUI();
            return;
        }

        auto peer = m_config->getPeer(peerId.toStdString());
        if (peer == nullptr)
        {
            QMessageBox::critical(this, "Error", "Failed to get peer configuration");
            m_sending = false;
            updateUI();
            return;
        }

        auto localAE = m_config->getLocalAEConfig();

        // Convert QStringList to std::vector<std::string>
        std::vector<std::string> files;
        for (const auto& file : m_files)
        {
            files.push_back(file.toStdString());
        }

        // Create progress callback
        auto progressCallback = [this](double progress, const std::string& message) {
            QMetaObject::invokeMethod(this, [this, progress, message]() {
                onSendProgress(progress, message);
            }, Qt::QueuedConnection);
        };

        // Send asynchronously
        auto future = m_storeService->storeFilesAsync(*peer, localAE, files, progressCallback);

        // Monitor completion
        auto* watcher = new QFutureWatcher<core::network::DimseStatus>(this);
        connect(watcher, &QFutureWatcher<core::network::DimseStatus>::finished,
                this, [this, watcher]() {
            auto status = watcher->result();
            watcher->deleteLater();
            onSendCompleted();
        });
        watcher->setFuture(future);
    }

    void DimseSendDialog::onSendProgress(double progress, const std::string& message)
    {
        m_progressBar->setValue(static_cast<int>(progress * 100));
        m_statusLabel->setText(QString::fromStdString(message));
    }

    void DimseSendDialog::onSendCompleted()
    {
        m_sending = false;
        m_progressBar->setVisible(false);

        int succeeded = m_storeService->getStoredCount();
        int failed = m_storeService->getFailedCount();

        QString statusMsg;
        QMessageBox::Icon icon;

        if (failed == 0)
        {
            statusMsg = QString("Successfully sent all %1 file(s) to PACS").arg(succeeded);
            icon = QMessageBox::Information;
            m_statusLabel->setText("Send completed successfully");
        }
        else if (succeeded == 0)
        {
            statusMsg = QString("Failed to send all files. Error: %1")
                       .arg(QString::fromStdString(m_storeService->getLastError()));
            icon = QMessageBox::Critical;
            m_statusLabel->setText("Send failed");
        }
        else
        {
            statusMsg = QString("Partial success: %1 file(s) sent, %2 failed")
                       .arg(succeeded).arg(failed);
            icon = QMessageBox::Warning;
            m_statusLabel->setText("Send completed with errors");
        }

        QMessageBox msgBox(icon, "Send Complete", statusMsg, QMessageBox::Ok, this);
        msgBox.exec();

        updateUI();
    }

    void DimseSendDialog::onCancel()
    {
        if (m_sending)
        {
            int result = QMessageBox::question(
                this,
                "Cancel Send",
                "Send operation in progress. Are you sure you want to cancel?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);

            if (result == QMessageBox::Yes && m_storeService)
            {
                m_storeService->cancelStore();
                m_sending = false;
                m_statusLabel->setText("Send cancelled by user");
                updateUI();
            }
        }
        else
        {
            reject();
        }
    }

} // namespace isis::gui::dialogs
