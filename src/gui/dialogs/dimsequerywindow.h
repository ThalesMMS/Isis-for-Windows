/*
 * ------------------------------------------------------------------------------------
 *  File: dimsequerywindow.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Window for querying and retrieving DICOM studies from PACS servers.
 *      Provides search filters, results display, and retrieve functionality.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QProgressBar>
#include <QLabel>
#include <memory>
#include <vector>
#include "../../core/network/dimseconfig.h"
#include "../../core/network/dimseservices.h"
#include "../../core/network/dimseassociation.h"

namespace isis::gui::dialogs
{
    /**
     * @brief Window for DICOM Query/Retrieve operations
     */
    class DimseQueryWindow : public QDialog
    {
        Q_OBJECT

    public:
        explicit DimseQueryWindow(QWidget* parent = nullptr);
        ~DimseQueryWindow() override = default;

        /**
         * @brief Set DIMSE configuration
         */
        void setDimseConfig(std::shared_ptr<core::network::DimseConfig> config);

        /**
         * @brief Set connection pool
         */
        void setConnectionPool(std::shared_ptr<core::network::DimseConnectionPool> pool);

        /**
         * @brief Set callback for retrieved studies
         */
        void setRetrieveCallback(std::function<void(const std::string& studyPath)> callback);

    signals:
        void studyRetrieved(const QString& studyPath);

    private slots:
        void onPeerChanged(int index);
        void onQuery();
        void onClearFilters();
        void onRetrieve();
        void onRetrieveAll();
        void onCancel();
        void onResultSelectionChanged();
        void onResultDoubleClicked(int row, int column);
        void onManagePeers();
        void updateProgressStatus(const QString& message);

    private:
        void setupUi();
        void refreshPeerList(const QString& preferredPeerId = QString());
        void populateResults(const std::vector<core::network::RemoteStudyInfo>& studies);
        void clearResults();
        core::network::QueryFilter getQueryFilter();
        std::vector<int> getSelectedStudyRows();
        void retrieveStudy(const core::network::RemoteStudyInfo& study);
        void setQueryEnabled(bool enabled);

        // UI Components - Filters
        QComboBox* m_peerCombo = nullptr;
        QLineEdit* m_patientNameEdit = nullptr;
        QLineEdit* m_patientIdEdit = nullptr;
        QDateEdit* m_studyDateFromEdit = nullptr;
        QDateEdit* m_studyDateToEdit = nullptr;
        QLineEdit* m_studyDescEdit = nullptr;
        QLineEdit* m_accessionEdit = nullptr;
        QCheckBox* m_modalityCT = nullptr;
        QCheckBox* m_modalityMR = nullptr;
        QCheckBox* m_modalityUS = nullptr;
        QCheckBox* m_modalityXA = nullptr;
        QCheckBox* m_modalityCR = nullptr;
        QSpinBox* m_maxResultsSpin = nullptr;

        // UI Components - Results
        QTableWidget* m_resultsTable = nullptr;
        QLabel* m_statusLabel = nullptr;
        QProgressBar* m_progressBar = nullptr;

        // UI Components - Buttons
        QPushButton* m_queryButton = nullptr;
        QPushButton* m_clearButton = nullptr;
        QPushButton* m_retrieveButton = nullptr;
        QPushButton* m_retrieveAllButton = nullptr;
        QPushButton* m_cancelButton = nullptr;
        QPushButton* m_managePeersButton = nullptr;

        // Data
        std::shared_ptr<core::network::DimseConfig> m_config;
        std::shared_ptr<core::network::DimseConnectionPool> m_connectionPool;
        std::shared_ptr<core::events::CallbackManager> m_eventManager;
        std::shared_ptr<core::network::DimseQueryService> m_queryService;
        std::shared_ptr<core::network::DimseRetrieveService> m_retrieveService;
        std::vector<core::network::RemoteStudyInfo> m_queryResults;
        std::function<void(const std::string&)> m_retrieveCallback;
        bool m_queryInProgress = false;
    };

} // namespace isis::gui::dialogs
