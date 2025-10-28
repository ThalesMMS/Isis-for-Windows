/*
 * ------------------------------------------------------------------------------------
 *  File: dicompropertiesdialog.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Dialog for viewing and editing DICOM metadata tags
 *      with structured visualization and safe tag editing
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <QDialog>
#include <QTreeWidget>
#include <QPushButton>
#include <QString>
#include <QMap>

namespace isis::core
{
    class Series;
}

namespace isis::gui::dialogs
{
    /**
     * @brief Dialog for DICOM properties visualization and editing
     *
     * Displays DICOM tags in a tree structure with support for
     * viewing and editing non-critical metadata.
     */
    class DicomPropertiesDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit DicomPropertiesDialog(QWidget* parent = nullptr);
        ~DicomPropertiesDialog() override = default;

        /**
         * @brief Set the series to display properties for
         * @param series DICOM series
         */
        void setSeries(core::Series* series);

        /**
         * @brief Refresh the properties tree
         */
        void refreshProperties();

        /**
         * @brief Export metadata to file
         * @param filename Output filename (CSV, JSON, or XML)
         * @return true if successful
         */
        bool exportMetadata(const QString& filename);

    private slots:
        void onExportClicked();
        void onItemDoubleClicked(QTreeWidgetItem* item, int column);
        void onSaveChanges();
        void onClose();

    private:
        void setupUi();
        void populateTree();
        void addTagToTree(const QString& group, const QString& element,
                         const QString& name, const QString& value);
        bool isEditableTag(const QString& group, const QString& element);

        QTreeWidget* m_propertiesTree = nullptr;
        QPushButton* m_exportButton = nullptr;
        QPushButton* m_saveButton = nullptr;
        QPushButton* m_closeButton = nullptr;

        core::Series* m_series = nullptr;
        QMap<QString, QString> m_modifiedTags;
    };

} // namespace isis::gui::dialogs
