/*
 * ------------------------------------------------------------------------------------
 *  File: studytreeitem.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of tree item for hierarchical representation
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "studytreeitem.h"
#include <QFileInfo>
#include "../../core/patient.h"
#include "../../core/study.h"
#include "../../core/series.h"
#include "../../core/image.h"

namespace
{
    const isis::core::Image* representativeImage(const isis::core::Series* series)
    {
        if (!series)
        {
            return nullptr;
        }

        const auto& singleFrames = series->getSingleFrameImages();
        if (!singleFrames.empty())
        {
            return singleFrames.begin()->get();
        }

        const auto& multiFrames = series->getMultiFrameImages();
        if (!multiFrames.empty())
        {
            return multiFrames.begin()->get();
        }

        return nullptr;
    }

    std::size_t seriesImageCount(const isis::core::Series* series)
    {
        if (!series)
        {
            return 0;
        }

        return series->getSingleFrameImages().size() + series->getMultiFrameImages().size();
    }
} // namespace

namespace isis::gui::tree
{
    StudyTreeItem::StudyTreeItem(TreeItemType type, void* data, StudyTreeItem* parent)
        : m_parent(parent)
        , m_data(data)
        , m_type(type)
    {
    }

    StudyTreeItem::~StudyTreeItem()
    {
        m_children.clear();
    }

    void StudyTreeItem::appendChild(std::unique_ptr<StudyTreeItem> child)
    {
        m_children.emplace_back(std::move(child));
    }

    StudyTreeItem* StudyTreeItem::child(int row)
    {
        if (row < 0 || row >= static_cast<int>(m_children.size()))
        {
            return nullptr;
        }
        return m_children.at(static_cast<std::size_t>(row)).get();
    }

    int StudyTreeItem::childCount() const
    {
        return static_cast<int>(m_children.size());
    }

    int StudyTreeItem::row() const
    {
        if (m_parent)
        {
            const int childCount = static_cast<int>(m_parent->m_children.size());
            for (int i = 0; i < childCount; ++i)
            {
                if (m_parent->m_children.at(static_cast<std::size_t>(i)).get() == this)
                {
                    return i;
                }
            }
        }
        return 0;
    }

    QVariant StudyTreeItem::data(int column) const
    {
        switch (m_type)
        {
        case TreeItemType::Root:
            return column == 0 ? QVariant("DICOM Studies") : QVariant();

        case TreeItemType::Patient:
            return QVariant(getPatientDisplay(column));

        case TreeItemType::Study:
            return QVariant(getStudyDisplay(column));

        case TreeItemType::Series:
            return QVariant(getSeriesDisplay(column));

        case TreeItemType::Image:
            return QVariant(getImageDisplay(column));

        default:
            return QVariant();
        }
    }

    core::Patient* StudyTreeItem::getPatient() const
    {
        if (m_type == TreeItemType::Patient && m_data)
        {
            return static_cast<core::Patient*>(m_data);
        }
        return nullptr;
    }

    core::Study* StudyTreeItem::getStudy() const
    {
        if (m_type == TreeItemType::Study && m_data)
        {
            return static_cast<core::Study*>(m_data);
        }
        return nullptr;
    }

    core::Series* StudyTreeItem::getSeries() const
    {
        if (m_type == TreeItemType::Series && m_data)
        {
            return static_cast<core::Series*>(m_data);
        }
        return nullptr;
    }

    core::Image* StudyTreeItem::getImage() const
    {
        if (m_type == TreeItemType::Image && m_data)
        {
            return static_cast<core::Image*>(m_data);
        }
        return nullptr;
    }

    QString StudyTreeItem::getPatientDisplay(int column) const
    {
        auto* patient = getPatient();
        if (!patient)
        {
            return QString();
        }

        switch (column)
        {
        case 0: // Name
            return QString::fromStdString(patient->getName());
        case 1: // Description (ID)
            return QString::fromStdString(patient->getID());
        case 2: // Count (studies)
        {
            const auto studyCount = static_cast<int>(patient->getStudies().size());
            return QStringLiteral("%1 %2").arg(studyCount).arg(studyCount == 1 ? QStringLiteral("study") : QStringLiteral("studies"));
        }
        default:
            return QString();
        }
    }

    QString StudyTreeItem::getStudyDisplay(int column) const
    {
        auto* study = getStudy();
        if (!study)
        {
            return QString();
        }

        switch (column)
        {
        case 0: // Name (Date)
            return QString::fromStdString(study->getDate());
        case 1: // Description
            return QString::fromStdString(study->getDescription());
        case 2: // Count (series)
        {
            const auto seriesCount = static_cast<int>(study->getSeries().size());
            return QStringLiteral("%1 series").arg(seriesCount);
        }
        default:
            return QString();
        }
    }

    QString StudyTreeItem::getSeriesDisplay(int column) const
    {
        auto* series = getSeries();
        if (!series)
        {
            return QString();
        }

        switch (column)
        {
        case 0: // Name (Modality + Number)
        {
            const auto* image = representativeImage(series);
            const QString modality = image ? QString::fromStdString(image->getModality()) : QString();
            const QString seriesNumber = QString::fromStdString(series->getNumber());

            if (!modality.isEmpty() && !seriesNumber.isEmpty())
            {
                return QStringLiteral("%1 %2").arg(modality, seriesNumber);
            }
            if (!modality.isEmpty())
            {
                return modality;
            }
            if (!seriesNumber.isEmpty())
            {
                return QStringLiteral("Series %1").arg(seriesNumber);
            }
            return QStringLiteral("Series");
        }
        case 1: // Description
            return QString::fromStdString(series->getDescription());
        case 2: // Count (images)
        {
            const auto imageCount = static_cast<int>(seriesImageCount(series));
            if (imageCount == 0)
            {
                return QStringLiteral("No images");
            }
            return QStringLiteral("%1 image%2").arg(imageCount).arg(imageCount == 1 ? QString() : QStringLiteral("s"));
        }
        default:
            return QString();
        }
    }

    QString StudyTreeItem::getImageDisplay(int column) const
    {
        auto* image = getImage();
        if (!image)
        {
            return QString();
        }

        switch (column)
        {
        case 0: // Name (Instance number)
        {
            const int instanceNumber = image->getInstanceNumber();
            if (instanceNumber > 0)
            {
                return QStringLiteral("Image %1").arg(instanceNumber);
            }
            return QStringLiteral("Image #%1").arg(image->getIndex() + 1);
        }
        case 1: // Description (file name)
        {
            const QFileInfo info(QString::fromStdString(image->getImagePath()));
            return info.fileName().isEmpty() ? info.filePath() : info.fileName();
        }
        case 2: // Modality
            return QString::fromStdString(image->getModality());
        default:
            return QString();
        }
    }

} // namespace isis::gui::tree
