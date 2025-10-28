/*
 * ------------------------------------------------------------------------------------
 *  File: studytreeitem.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Tree item for hierarchical study/series/image representation
 *      Optimized for radiological workflow: Study → Series → Image
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <QString>
#include <QVariant>
#include <memory>
#include <vector>

namespace isis::core
{
    class Patient;
    class Study;
    class Series;
    class Image;
}

namespace isis::gui::tree
{
    /**
     * @brief Type of tree item
     */
    enum class TreeItemType
    {
        Root,
        Patient,
        Study,
        Series,
        Image
    };

    /**
     * @brief Tree item for hierarchical DICOM data representation
     *
     * Represents a node in the study tree, which can be a patient,
     * study, series, or image.
     */
    class StudyTreeItem
    {
    public:
        explicit StudyTreeItem(TreeItemType type, void* data = nullptr, StudyTreeItem* parent = nullptr);
        ~StudyTreeItem();

        // Child management
        void appendChild(std::unique_ptr<StudyTreeItem> child);
        StudyTreeItem* child(int row);
        int childCount() const;
        int row() const;

        // Data access
        [[nodiscard]] TreeItemType getType() const { return m_type; }
        [[nodiscard]] void* getData() const { return m_data; }
        [[nodiscard]] StudyTreeItem* getParent() const { return m_parent; }

        // Display data
        [[nodiscard]] QVariant data(int column) const;
        [[nodiscard]] int columnCount() const { return 3; } // Name, Description, Count

        // Patient data accessors
        [[nodiscard]] core::Patient* getPatient() const;
        [[nodiscard]] core::Study* getStudy() const;
        [[nodiscard]] core::Series* getSeries() const;
        [[nodiscard]] core::Image* getImage() const;

    private:
        std::vector<std::unique_ptr<StudyTreeItem>> m_children;
        StudyTreeItem* m_parent;
        void* m_data;
        TreeItemType m_type;

        // Helper methods
        QString getPatientDisplay(int column) const;
        QString getStudyDisplay(int column) const;
        QString getSeriesDisplay(int column) const;
        QString getImageDisplay(int column) const;
    };

} // namespace isis::gui::tree
