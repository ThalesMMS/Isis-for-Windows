/*
 * ------------------------------------------------------------------------------------
 *  File: studytreemodel.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of tree model for DICOM data hierarchy
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "studytreemodel.h"
#include "../../core/corerepository.h"
#include "../../core/patient.h"
#include "../../core/study.h"
#include "../../core/series.h"
#include "../../core/image.h"

namespace isis::gui::tree
{
    StudyTreeModel::StudyTreeModel(core::CoreRepository* repository, QObject* parent)
        : QAbstractItemModel(parent)
        , m_repository(repository)
    {
        m_rootItem = std::make_unique<StudyTreeItem>(TreeItemType::Root);
        if (m_repository)
        {
            setupModelData();
        }
    }

    StudyTreeModel::~StudyTreeModel() = default;

    QModelIndex StudyTreeModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        StudyTreeItem* parentItem = nullptr;

        if (!parent.isValid())
        {
            parentItem = m_rootItem.get();
        }
        else
        {
            parentItem = static_cast<StudyTreeItem*>(parent.internalPointer());
        }

        StudyTreeItem* childItem = parentItem->child(row);
        if (childItem)
        {
            return createIndex(row, column, childItem);
        }

        return QModelIndex();
    }

    QModelIndex StudyTreeModel::parent(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return QModelIndex();
        }

        auto* childItem = static_cast<StudyTreeItem*>(index.internalPointer());
        StudyTreeItem* parentItem = childItem->getParent();

        if (parentItem == m_rootItem.get() || !parentItem)
        {
            return QModelIndex();
        }

        return createIndex(parentItem->row(), 0, parentItem);
    }

    int StudyTreeModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.column() > 0)
        {
            return 0;
        }

        StudyTreeItem* parentItem = nullptr;
        if (!parent.isValid())
        {
            parentItem = m_rootItem.get();
        }
        else
        {
            parentItem = static_cast<StudyTreeItem*>(parent.internalPointer());
        }

        return parentItem ? parentItem->childCount() : 0;
    }

    int StudyTreeModel::columnCount(const QModelIndex& parent) const
    {
        if (parent.isValid())
        {
            return static_cast<StudyTreeItem*>(parent.internalPointer())->columnCount();
        }
        return m_rootItem ? m_rootItem->columnCount() : 0;
    }

    QVariant StudyTreeModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        if (role != Qt::DisplayRole)
        {
            return QVariant();
        }

        auto* item = static_cast<StudyTreeItem*>(index.internalPointer());
        return item->data(index.column());
    }

    Qt::ItemFlags StudyTreeModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return Qt::NoItemFlags;
        }

        return QAbstractItemModel::flags(index);
    }

    QVariant StudyTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section)
            {
            case 0:
                return tr("Name");
            case 1:
                return tr("Description");
            case 2:
                return tr("Info");
            default:
                return QVariant();
            }
        }

        return QVariant();
    }

    StudyTreeItem* StudyTreeModel::getItem(const QModelIndex& index) const
    {
        if (index.isValid())
        {
            auto* item = static_cast<StudyTreeItem*>(index.internalPointer());
            if (item)
            {
                return item;
            }
        }
        return m_rootItem.get();
    }

    void StudyTreeModel::refreshModel()
    {
        beginResetModel();
        m_rootItem = std::make_unique<StudyTreeItem>(TreeItemType::Root);
        setupModelData();
        endResetModel();
    }

    void StudyTreeModel::setupModelData()
    {
        if (!m_repository)
        {
            return;
        }

        buildTree();
    }

    void StudyTreeModel::buildTree()
    {
        auto& patients = m_repository->getPatients();

        for (auto& patientPtr : patients)
        {
            core::Patient* patient = patientPtr.get();
            auto patientItem = std::make_unique<StudyTreeItem>(
                TreeItemType::Patient, patient, m_rootItem.get());

            for (auto& studyPtr : patient->getStudies())
            {
                core::Study* study = studyPtr.get();
                auto studyItem = std::make_unique<StudyTreeItem>(
                    TreeItemType::Study, study, patientItem.get());

                for (auto& seriesPtr : study->getSeries())
                {
                    core::Series* series = seriesPtr.get();
                    auto seriesItem = std::make_unique<StudyTreeItem>(
                        TreeItemType::Series, series, studyItem.get());
                    auto* const seriesNode = seriesItem.get();

                    const auto appendImages = [seriesNode](auto& images)
                    {
                        for (auto& imagePtr : images)
                        {
                            if (!imagePtr)
                            {
                                continue;
                            }

                            auto imageItem = std::make_unique<StudyTreeItem>(
                                TreeItemType::Image, imagePtr.get(), seriesNode);
                            seriesNode->appendChild(std::move(imageItem));
                        }
                    };

                    appendImages(series->getSingleFrameImages());
                    appendImages(series->getMultiFrameImages());

                    studyItem->appendChild(std::move(seriesItem));
                }

                patientItem->appendChild(std::move(studyItem));
            }

            m_rootItem->appendChild(std::move(patientItem));
        }
    }

} // namespace isis::gui::tree
