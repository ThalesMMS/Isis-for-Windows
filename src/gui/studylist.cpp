/*
 * ------------------------------------------------------------------------------------
 *  File: studylist.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements data handling for the study list model and its Qt model/view integration.
 *
 *  License:
 *      This file is part of a derived work based on the Asclepios DICOM Viewer,
 *      licensed under the MIT License.
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a copy
 *      of this software and associated documentation files (the "Software"), to deal
 *      in the Software without restriction, including without limitation the rights
 *      to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *      copies of the Software, and to permit persons to whom the Software is
 *      furnished to do so, subject to the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included in
 *      all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *      FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *      AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *      LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *      OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *      SOFTWARE.
 * ------------------------------------------------------------------------------------
 */

#include "studylist.h"
#include "core/image.h"
#include "core/series.h"
#include <QColor>
#include <QDebug>
#include <QDir>
#include <QDrag>
#include <QFile>
#include <QFileDialog>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaMethod>
#include <QMetaObject>
#include <QMimeData>
#include <QPointer>
#include <QPixmap>
#include <QResizeEvent>
#include <QTemporaryFile>
#include <QtConcurrent/qtconcurrentrun.h>
#include <QStringList>
#include <algorithm>
#include <dcmtk/dcmimgle/dcmimage.h>
#include "smartdjdecoderregistration.h"

namespace
{
        constexpr int kThumbnailMinimumWidth = 180;
        constexpr int kThumbnailHeight = 160;
        constexpr int kThumbnailIconHeight = 140;
        constexpr int kThumbnailHorizontalPadding = 18;

        QString darkStudyListStylesheet()
        {
		static const QString darkStyle = QString::fromLatin1(
			"QListView {\n"
			"    background-color: #2e2e2e;\n"
			"    color: #f5f5f5;\n"
			"    border: none;\n"
			"    padding: 6px 8px;\n"
			"    font-family: \"Segoe UI\";\n"
			"    font-size: 9pt;\n"
			"}\n"
			"\n"
			"QListView::item {\n"
			"    border: 1px solid #4c4c4c;\n"
			"    border-radius: 6px;\n"
			"    margin: 4px 3px;\n"
			"    padding: 6px;\n"
			"    background-color: rgba(0, 0, 0, 0.0);\n"
			"}\n"
			"\n"
                        "QListView::item:hover {\n"
                        "    border-color: rgba(248, 213, 96, 0.5);\n"
                        "    background-color: rgba(248, 213, 96, 0.08);\n"
                        "}\n"
                        "\n"
                        "QListView::item:selected {\n"
                        "    border: 2px solid #f8d560;\n"
                        "    background-color: rgba(248, 213, 96, 0.18);\n"
                        "}\n");
                return darkStyle;
        }

}

isis::gui::StudyList::StudyList(QWidget* parent)
	: QListWidget(parent)
{
	initView();
}


//-----------------------------------------------------------------------------
void isis::gui::StudyList::initView()
{
	m_ui.setupUi(this);
	setAcceptDrops(true);
	setFlow(LeftToRight);
	setResizeMode(Adjust);
	setViewMode(IconMode);
	setSpacing(2);
        ensureDarkStyleSheet();
        setStyleSheet(m_darkStyleSheet);
}

//-----------------------------------------------------------------------------
void isis::gui::StudyList::insertNewSeries(core::Series* t_series, core::Image* t_image)
{
	auto* newSeriesItem = new SeriesItem(this);
	newSeriesItem->setSizeHint(QSize(desiredItemWidth(), kThumbnailHeight));
        newSeriesItem->setSeries(t_series);
        const int totalFrames = totalFramesForSeries(t_series);
        const int currentFrame = totalFrames > 0 ? 1 : 0;
        newSeriesItem->setText(seriesLegend(t_series, currentFrame, totalFrames));
        newSeriesItem->setForeground(QColor(QStringLiteral("#f5f5f5")));
	newSeriesItem->setData(Qt::UserRole, createMimeData(t_series, t_image));
	qRegisterMetaType<QVector<int>>("QVector<int>");
	m_futures.push_back(QtConcurrent::run(createImageForItem, this, t_image, newSeriesItem));
	if (!isSignalConnected(QMetaMethod::fromSignal(&StudyList::finishConcurrent)))
	{
		Q_UNUSED(connect(this, &StudyList::finishConcurrent, this, &StudyList::cleanUp))
	}
        updateItemSizes();
}

//-----------------------------------------------------------------------------
bool isis::gui::StudyList::updateSeriesProgress(core::Series* t_series, int frameIndex, int frameCount)
{
        if (!t_series)
        {
                return false;
        }

        const int totalFrames = (frameCount > 0)
                ? frameCount
                : totalFramesForSeries(t_series);
        const int normalizedTotal = std::max(totalFrames, 0);
        const int normalizedIndex = (normalizedTotal > 0)
                ? std::clamp(frameIndex + 1, 1, normalizedTotal)
                : 0;

        for (int i = 0; i < count(); ++i)
        {
                auto* const listItem = static_cast<SeriesItem*>(item(i));
                if (!listItem || !listItem->matchesSeries(t_series))
                {
                        continue;
                }

                listItem->setText(seriesLegend(t_series, normalizedIndex, normalizedTotal));
                return true;
        }

        return false;
}

//-----------------------------------------------------------------------------
void isis::gui::StudyList::applyDarkPalette()
{
        ensureDarkStyleSheet();
        setStyleSheet(m_darkStyleSheet);

        const QColor textColor(QStringLiteral("#f5f5f5"));
        for (int i = 0; i < count(); ++i)
        {
                if (auto* const listItem = static_cast<SeriesItem*>(item(i)))
                {
                        listItem->setForeground(textColor);
                }
        }
}

//-----------------------------------------------------------------------------
QString isis::gui::StudyList::createMimeData(core::Series* t_series, core::Image* t_image)
{
	const QString series = QString::asprintf("%16p", static_cast<const void*>(t_series));
	const QString image = QString::asprintf("%16p", static_cast<const void*>(t_image));
	QJsonDocument data;
	QJsonObject obj;
	obj.insert("Series", series);
	obj.insert("Image", image);
	data.setObject(obj);
	return data.toJson();
}

//-----------------------------------------------------------------------------
void isis::gui::StudyList::createImageForItem(StudyList* t_self, core::Image* t_image, SeriesItem* t_item)
{
        QPointer<StudyList> guardedSelf(t_self);
        const auto queueCleanup = [guardedSelf, t_item]()
        {
                if (!guardedSelf)
                {
                        delete t_item;
                        return;
                }

                QMetaObject::invokeMethod(guardedSelf, [guardedSelf, t_item]()
                {
                        if (!guardedSelf)
                        {
                                delete t_item;
                                return;
                        }
                        delete t_item;
                        emit guardedSelf->finishConcurrent();
                }, Qt::QueuedConnection);
        };

        if (!t_image || !t_item)
        {
                queueCleanup();
                return;
        }

        try
        {
                core::SmartDJDecoderRegistration::registerCodecs();
                const auto dcmImage = std::make_unique<DicomImage>(t_image->getImagePath().c_str(),
                        CIF_UsePartialAccessToPixelData | CIF_AcrNemaCompatibility,
                        0, 1);
                dcmImage->setWindow(t_image->getWindowCenter(), t_image->getWindowWidth());
                std::unique_ptr<DicomImage> scaledImage(dcmImage->createScaledImage(300UL));

                QImage thumbnail;
                if (scaledImage)
                {
                        QTemporaryFile tempFile(QDir::tempPath() + QLatin1String("/isis-thumb-XXXXXX.bmp"));
                        tempFile.setAutoRemove(false);
                        if (tempFile.open())
                        {
                                const QString tempPath = tempFile.fileName();
                                tempFile.close();
                                if (scaledImage->writeBMP(tempPath.toStdString().c_str(), 24))
                                {
                                        thumbnail = QImage(tempPath);
                                }
                                QFile::remove(tempPath);
                        }
                }

                const int imageColumns = t_image->getColumns();
                const int imageRows = t_image->getRows();
                if (!guardedSelf)
                {
                        delete t_item;
                        return;
                }

                QMetaObject::invokeMethod(guardedSelf, [guardedSelf, t_item, imageColumns, imageRows, thumb = std::move(thumbnail)]() mutable
                {
                        if (!guardedSelf)
                        {
                                delete t_item;
                                return;
                        }

                        t_item->setWidth(imageColumns);
                        t_item->setHeight(imageRows);
                        if (!thumb.isNull())
                        {
                                t_item->setSeriesPhoto(QPixmap::fromImage(thumb));
                        }
                        guardedSelf->addItem(t_item);
                        guardedSelf->updateItemSizes();
                        emit guardedSelf->finishConcurrent();
                }, Qt::QueuedConnection);
        }
        catch (const std::exception& ex)
        {
                qWarning() << "[StudyList] Failed to generate thumbnail:" << ex.what();
                queueCleanup();
        }
}

//-----------------------------------------------------------------------------
void isis::gui::StudyList::startDrag(Qt::DropActions supportedActions)
{
	auto* const drag = new QDrag(this);
	const auto icon = selectedItems().at(0)->icon();
	drag->setPixmap(icon.pixmap(100, 100));
	auto* const mimeData = new QMimeData();
	mimeData->setText(selectedItems().at(0)->data(Qt::UserRole).toString());
	drag->setMimeData(mimeData);
	drag->exec();
}

//-----------------------------------------------------------------------------
void isis::gui::StudyList::cleanUp()
{
	core::SmartDJDecoderRegistration::cleanup();
}

//-----------------------------------------------------------------------------
void isis::gui::StudyList::resizeEvent(QResizeEvent* event)
{
        QListWidget::resizeEvent(event);
        updateItemSizes();
}

//-----------------------------------------------------------------------------
void isis::gui::StudyList::updateItemSizes()
{
        const int targetWidth = desiredItemWidth();
        const int iconWidth = std::max(96, targetWidth - kThumbnailHorizontalPadding);
        const QSize iconDimensions(iconWidth, kThumbnailIconHeight);
        if (iconSize() != iconDimensions)
        {
                setIconSize(iconDimensions);
        }

        for (int i = 0; i < count(); ++i)
        {
                if (auto* const seriesItem = dynamic_cast<SeriesItem*>(item(i)))
                {
                        const QSize current = seriesItem->sizeHint();
                        if (current.width() != targetWidth || current.height() != kThumbnailHeight)
                        {
                                seriesItem->setSizeHint(QSize(targetWidth, kThumbnailHeight));
                        }
                }
        }

        doItemsLayout();
}

//-----------------------------------------------------------------------------
void isis::gui::StudyList::ensureDarkStyleSheet()
{
        if (m_darkStyleSheet.isEmpty())
        {
                m_darkStyleSheet = darkStudyListStylesheet();
        }
}

//-----------------------------------------------------------------------------
int isis::gui::StudyList::desiredItemWidth() const
{
        const auto* const listViewport = viewport();
        const int viewportWidth = listViewport ? listViewport->width() : width();
        const int frame = frameWidth();
        const int spacingValue = spacing();
        const int available = viewportWidth - (2 * frame) - (2 * spacingValue);
        if (available > 0)
        {
                return available;
        }
        return kThumbnailMinimumWidth;
}

//-----------------------------------------------------------------------------
QString isis::gui::StudyList::seriesLegend(core::Series* t_series, int currentFrame, int totalFrames) const
{
        if (!t_series)
        {
                return QStringLiteral("Se: --\nIm: --");
        }

        const QString seriesNumber = QString::fromStdString(t_series->getNumber()).trimmed();
        const QString seriesDescription = QString::fromStdString(t_series->getDescription()).trimmed();
        const QString studyDescription = (m_study)
                ? QString::fromStdString(m_study->getDescription()).trimmed()
                : QString();

        QStringList lines;
        lines << (seriesNumber.isEmpty()
                        ? QStringLiteral("Se: --")
                        : QStringLiteral("Se: %1").arg(seriesNumber));

        QString frameLine = QStringLiteral("Im: --");
        if (totalFrames > 0)
        {
                const int boundedCurrent = std::clamp(currentFrame, 1, totalFrames);
                frameLine = QStringLiteral("Im: %1/%2").arg(boundedCurrent).arg(totalFrames);
        }
        lines << frameLine;

        const QString descriptor = !seriesDescription.isEmpty()
                ? seriesDescription
                : studyDescription;
        if (!descriptor.isEmpty())
        {
                lines << descriptor;
        }

        return lines.join(QLatin1Char('\n'));
}

//-----------------------------------------------------------------------------
int isis::gui::StudyList::totalFramesForSeries(core::Series* t_series) const
{
        if (!t_series)
        {
                return 0;
        }

        const auto& singleFrameImages = t_series->getSingleFrameImages();
        if (!singleFrameImages.empty())
        {
                return static_cast<int>(singleFrameImages.size());
        }

        auto& multiFrameImages = t_series->getMultiFrameImages();
        int total = 0;
        for (const auto& frameImage : multiFrameImages)
        {
                if (!frameImage)
                {
                        continue;
                }
                const int frameCount = frameImage->getNumberOfFrames();
                if (frameCount > 0)
                {
                        total += frameCount;
                }
        }
        return total;
}
