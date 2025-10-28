/*
 * ------------------------------------------------------------------------------------
 *  File: dicompropertiesdialog.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of DICOM properties dialog
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "dicompropertiesdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>

namespace isis::gui::dialogs
{
    DicomPropertiesDialog::DicomPropertiesDialog(QWidget* parent)
        : QDialog(parent)
    {
        setupUi();
        setWindowTitle("DICOM Properties");
        resize(800, 600);
    }

    void DicomPropertiesDialog::setupUi()
    {
        auto* mainLayout = new QVBoxLayout(this);

        // Tree widget for properties
        m_propertiesTree = new QTreeWidget(this);
        m_propertiesTree->setColumnCount(4);
        m_propertiesTree->setHeaderLabels({"Group", "Element", "Name", "Value"});
        m_propertiesTree->setAlternatingRowColors(true);
        m_propertiesTree->header()->setStretchLastSection(true);

        connect(m_propertiesTree, &QTreeWidget::itemDoubleClicked,
                this, &DicomPropertiesDialog::onItemDoubleClicked);

        // Buttons
        auto* buttonLayout = new QHBoxLayout();

        m_exportButton = new QPushButton("Export...", this);
        m_saveButton = new QPushButton("Save Changes", this);
        m_closeButton = new QPushButton("Close", this);

        m_saveButton->setEnabled(false);

        connect(m_exportButton, &QPushButton::clicked,
                this, &DicomPropertiesDialog::onExportClicked);
        connect(m_saveButton, &QPushButton::clicked,
                this, &DicomPropertiesDialog::onSaveChanges);
        connect(m_closeButton, &QPushButton::clicked,
                this, &DicomPropertiesDialog::onClose);

        buttonLayout->addWidget(m_exportButton);
        buttonLayout->addStretch();
        buttonLayout->addWidget(m_saveButton);
        buttonLayout->addWidget(m_closeButton);

        mainLayout->addWidget(m_propertiesTree);
        mainLayout->addLayout(buttonLayout);
    }

    void DicomPropertiesDialog::setSeries(core::Series* series)
    {
        m_series = series;
        refreshProperties();
    }

    void DicomPropertiesDialog::refreshProperties()
    {
        m_propertiesTree->clear();
        m_modifiedTags.clear();

        if (!m_series)
        {
            return;
        }

        populateTree();
    }

    void DicomPropertiesDialog::populateTree()
    {
        // Placeholder: In a real implementation, this would iterate through
        // DICOM tags from the series and populate the tree

        // Example tags (would come from actual DICOM data)
        addTagToTree("0010", "0010", "Patient Name", "DOE^JOHN");
        addTagToTree("0010", "0020", "Patient ID", "12345");
        addTagToTree("0010", "0030", "Patient Birth Date", "19800101");
        addTagToTree("0020", "000D", "Study Instance UID", "1.2.3.4.5...");
        addTagToTree("0020", "000E", "Series Instance UID", "1.2.3.4.6...");
        addTagToTree("0028", "0010", "Rows", "512");
        addTagToTree("0028", "0011", "Columns", "512");
    }

    void DicomPropertiesDialog::addTagToTree(const QString& group, const QString& element,
                                             const QString& name, const QString& value)
    {
        auto* item = new QTreeWidgetItem(m_propertiesTree);
        item->setText(0, group);
        item->setText(1, element);
        item->setText(2, name);
        item->setText(3, value);

        // Mark editable tags
        if (isEditableTag(group, element))
        {
            item->setFlags(item->flags() | Qt::ItemIsEditable);
        }
    }

    bool DicomPropertiesDialog::isEditableTag(const QString& group, const QString& element)
    {
        // Only allow editing of non-critical tags
        // Patient name, comments, etc. are editable
        // UIDs, image data tags are not
        if (group == "0010" && element == "0010") return true; // Patient Name
        if (group == "0010" && element == "0020") return true; // Patient ID
        if (group == "0020" && element == "4000") return true; // Image Comments

        return false;
    }

    void DicomPropertiesDialog::onItemDoubleClicked(QTreeWidgetItem* item, int column)
    {
        if (column == 3 && isEditableTag(item->text(0), item->text(1)))
        {
            m_propertiesTree->editItem(item, column);
            m_saveButton->setEnabled(true);

            QString key = item->text(0) + "," + item->text(1);
            m_modifiedTags[key] = item->text(3);
        }
    }

    void DicomPropertiesDialog::onSaveChanges()
    {
        if (m_modifiedTags.isEmpty())
        {
            return;
        }

        // Placeholder: Would actually write back to DICOM files
        QMessageBox::information(this, "Save Changes",
                                 QString("Would save %1 modified tag(s).\n"
                                        "(Not implemented in placeholder)")
                                 .arg(m_modifiedTags.size()));

        m_modifiedTags.clear();
        m_saveButton->setEnabled(false);
    }

    void DicomPropertiesDialog::onExportClicked()
    {
        QString filename = QFileDialog::getSaveFileName(
            this, "Export Metadata",
            QString(), "CSV Files (*.csv);;JSON Files (*.json);;XML Files (*.xml)");

        if (!filename.isEmpty())
        {
            exportMetadata(filename);
        }
    }

    bool DicomPropertiesDialog::exportMetadata(const QString& filename)
    {
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            return false;
        }

        QTextStream out(&file);

        // Simple CSV export
        if (filename.endsWith(".csv", Qt::CaseInsensitive))
        {
            out << "Group,Element,Name,Value\n";

            for (int i = 0; i < m_propertiesTree->topLevelItemCount(); ++i)
            {
                auto* item = m_propertiesTree->topLevelItem(i);
                out << item->text(0) << ","
                    << item->text(1) << ","
                    << "\"" << item->text(2) << "\","
                    << "\"" << item->text(3) << "\"\n";
            }
        }

        file.close();
        QMessageBox::information(this, "Export Successful",
                                "Metadata exported successfully!");
        return true;
    }

    void DicomPropertiesDialog::onClose()
    {
        if (!m_modifiedTags.isEmpty())
        {
            auto reply = QMessageBox::question(
                this, "Unsaved Changes",
                "You have unsaved changes. Do you want to save before closing?",
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

            if (reply == QMessageBox::Yes)
            {
                onSaveChanges();
            }
            else if (reply == QMessageBox::Cancel)
            {
                return;
            }
        }

        accept();
    }

} // namespace isis::gui::dialogs
