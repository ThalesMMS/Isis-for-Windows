/*
 * ------------------------------------------------------------------------------------
 *  File: dimsesenddialog.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Dialog for sending DICOM files to PACS servers (C-STORE SCU).
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QListWidget>
#include <QProgressBar>
#include <QLabel>
#include <memory>
#include "../../core/network/dimseconfig.h"
#include "../../core/network/dimseassociation.h"
#include "../../core/network/dimseservices.h"

namespace isis::gui::dialogs
{
    /**
     * @brief Dialog for sending DICOM files to PACS (C-STORE SCU)
     */
    class DimseSendDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit DimseSendDialog(QWidget* parent = nullptr);
        ~DimseSendDialog() override = default;

        /**
         * @brief Set DIMSE configuration
         */
        void setDimseConfig(std::shared_ptr<core::network::DimseConfig> config);

        /**
         * @brief Set connection pool
         */
        void setConnectionPool(std::shared_ptr<core::network::DimseConnectionPool> pool);

        /**
         * @brief Set store service
         */
        void setStoreService(std::shared_ptr<core::network::DimseStoreService> service);

        /**
         * @brief Set initial files to send
         */
        void setInitialFiles(const QStringList& files);

    private slots:
        void onAddFiles();
        void onAddDirectory();
        void onRemoveFiles();
        void onClearFiles();
        void onSend();
        void onCancel();
        void onPeerChanged(int index);

    private:
        void setupUI();
        void updateUI();
        void updatePeersList();
        bool validateInputs();
        void startSending();
        void onSendProgress(double progress, const std::string& message);
        void onSendCompleted();

        // UI elements
        QComboBox* m_peerCombo = nullptr;
        QListWidget* m_filesList = nullptr;
        QPushButton* m_addFilesBtn = nullptr;
        QPushButton* m_addDirBtn = nullptr;
        QPushButton* m_removeBtn = nullptr;
        QPushButton* m_clearBtn = nullptr;
        QPushButton* m_sendBtn = nullptr;
        QPushButton* m_cancelBtn = nullptr;
        QProgressBar* m_progressBar = nullptr;
        QLabel* m_statusLabel = nullptr;
        QLabel* m_filesCountLabel = nullptr;

        // DIMSE components
        std::shared_ptr<core::network::DimseConfig> m_config;
        std::shared_ptr<core::network::DimseConnectionPool> m_connectionPool;
        std::shared_ptr<core::network::DimseStoreService> m_storeService;

        // State
        bool m_sending = false;
        QStringList m_files;
    };

} // namespace isis::gui::dialogs
