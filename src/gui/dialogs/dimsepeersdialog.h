/*
 * ------------------------------------------------------------------------------------
 *  File: dimsepeersdialog.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Dialog for managing DICOM peers (PACS servers) configuration.
 *      Allows add/edit/remove/test operations on peer configurations.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <memory>
#include "../../core/network/dimseconfig.h"
#include "../../core/network/dimseservices.h"
#include "../../core/network/dimseassociation.h"

namespace isis::gui::dialogs
{
    /**
     * @brief Dialog for DICOM peer management
     */
    class DimsePeersDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit DimsePeersDialog(QWidget* parent = nullptr);
        ~DimsePeersDialog() override = default;

        /**
         * @brief Set the DIMSE configuration to manage
         */
        void setDimseConfig(std::shared_ptr<core::network::DimseConfig> config);

        /**
         * @brief Set connection pool for testing
         */
        void setConnectionPool(std::shared_ptr<core::network::DimseConnectionPool> pool);

    private slots:
        void onAddPeer();
        void onEditPeer();
        void onRemovePeer();
        void onTestConnection();
        void onImportConfig();
        void onExportConfig();
        void onSaveChanges();
        void onCancel();
        void onPeerSelectionChanged();

    private:
        void setupUi();
        void refreshPeerTable();
        void editPeerAtRow(int row);
        bool validatePeerInput(QString& errorMsg);
        void clearPeerForm();
        void loadPeerToForm(const core::network::DicomPeer& peer);
        core::network::DicomPeer getPeerFromForm();

        // UI Components
        QTableWidget* m_peerTable = nullptr;
        QPushButton* m_addButton = nullptr;
        QPushButton* m_editButton = nullptr;
        QPushButton* m_removeButton = nullptr;
        QPushButton* m_testButton = nullptr;
        QPushButton* m_importButton = nullptr;
        QPushButton* m_exportButton = nullptr;
        QPushButton* m_saveButton = nullptr;
        QPushButton* m_cancelButton = nullptr;

        // Peer form inputs
        QLineEdit* m_nameEdit = nullptr;
        QLineEdit* m_aeTitleEdit = nullptr;
        QLineEdit* m_hostnameEdit = nullptr;
        QSpinBox* m_portSpin = nullptr;
        QSpinBox* m_timeoutSpin = nullptr;
        QCheckBox* m_enabledCheck = nullptr;

        // Data
        std::shared_ptr<core::network::DimseConfig> m_config;
        std::shared_ptr<core::network::DimseConnectionPool> m_connectionPool;
        std::shared_ptr<core::events::CallbackManager> m_eventManager;
        std::string m_editingPeerId; // ID of peer being edited, empty if adding new
    };

} // namespace isis::gui::dialogs
