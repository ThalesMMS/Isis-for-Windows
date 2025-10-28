/*
 * ------------------------------------------------------------------------------------
 *  File: studytreemodel.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Tree model for hierarchical DICOM data visualization
 *      Based on Qt's QAbstractItemModel for efficient tree navigation
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <QAbstractItemModel>
#include <memory>
#include "studytreeitem.h"

namespace isis::core
{
    class CoreRepository;
}

namespace isis::gui::tree
{
    /**
     * @brief Model for hierarchical DICOM study tree
     *
     * Implements Qt's model-view pattern for displaying DICOM data
     * in a hierarchical tree: Patient → Study → Series → Image
     */
    class StudyTreeModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        explicit StudyTreeModel(core::CoreRepository* repository, QObject* parent = nullptr);
        ~StudyTreeModel() override;

        // QAbstractItemModel interface
        [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
        [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
        [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                          int role = Qt::DisplayRole) const override;
        [[nodiscard]] QModelIndex index(int row, int column,
                                        const QModelIndex& parent = QModelIndex()) const override;
        [[nodiscard]] QModelIndex parent(const QModelIndex& index) const override;
        [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        [[nodiscard]] int columnCount(const QModelIndex& parent = QModelIndex()) const override;

        // Custom methods
        void refreshModel();
        [[nodiscard]] StudyTreeItem* getItem(const QModelIndex& index) const;

    private:
        void setupModelData();
        void buildTree();

        std::unique_ptr<StudyTreeItem> m_rootItem;
        core::CoreRepository* m_repository;
    };

} // namespace isis::gui::tree
