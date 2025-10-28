/*
 * ------------------------------------------------------------------------------------
 *  File: dcmtkoverlaywidget.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements the Qt wrapper that connects DICOM metadata overlays to VTK renderers.
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

#include "dcmtkoverlaywidget.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QPainter>
#include <QStringList>

#include <algorithm>
#include <cmath>

#include "utils.h"

Q_LOGGING_CATEGORY(lcDcmtkOverlayWidget, "isis.gui.dcmtkoverlay")

using isis::core::DicomMetadata;
using isis::core::DicomTag;

namespace
{
        constexpr int cornerCount = 4;
}

isis::gui::DcmtkOverlayWidget::DcmtkOverlayWidget(QWidget* parent)
        : QWidget(parent)
{
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_OpaquePaintEvent, false);
        loadConfiguration();
}

void isis::gui::DcmtkOverlayWidget::setMetadata(const DicomMetadata* t_metadata)
{
        loadConfiguration();
        m_entryTexts.clear();
        populateStaticEntries(t_metadata);
        update();
}

void isis::gui::DcmtkOverlayWidget::setSeriesInformation(const QString& t_seriesNumber,
        int t_imageIndex, int t_imageCount)
{
        loadConfiguration();
        const unsigned int seriesKey = static_cast<unsigned int>(overlayKey::series);
        for (const auto& entry : m_entries)
        {
                if (entry.TagKey != seriesKey)
                {
                        continue;
                }

                if (t_seriesNumber.isEmpty() && t_imageCount <= 0)
                {
                        setEntryText(entry.TagKey, {});
                        update();
                        return;
                }

                const int clampedIndex = std::max(t_imageIndex, 0);
                const int clampedCount = std::max(t_imageCount, 0);
                const int normalizedIndex = (clampedCount > 0)
                        ? std::min(clampedIndex, clampedCount - 1)
                        : clampedIndex;
                QStringList fragments;
                if (clampedCount > 0)
                {
                        fragments << tr("Im: %1/%2").arg(normalizedIndex + 1).arg(clampedCount);
                }
                if (!t_seriesNumber.isEmpty())
                {
                        fragments << tr("Se: %1").arg(t_seriesNumber);
                }

                QString text = fragments.join(QStringLiteral("\n"));
                if (!text.isEmpty())
                {
                        text.append(entry.TextAfter.isEmpty() ? QStringLiteral("\n") : entry.TextAfter);
                }
                setEntryText(entry.TagKey, text);
                update();
                return;
        }
}

void isis::gui::DcmtkOverlayWidget::setZoom(double t_zoomFactor)
{
        loadConfiguration();
        const unsigned int zoomKey = static_cast<unsigned int>(overlayKey::zoom);
        for (const auto& entry : m_entries)
        {
                if (entry.TagKey != zoomKey)
                {
                        continue;
                }

                if (t_zoomFactor <= 0.0 || !std::isfinite(t_zoomFactor))
                {
                        setEntryText(entry.TagKey, {});
                        update();
                        return;
                }

                QString prefix = entry.TextBefore;
                if (prefix.isEmpty())
                {
                        prefix = QStringLiteral("Zoom: ");
                }
                QString text = prefix;
                text.append(formatZoom(t_zoomFactor));
                text.append(entry.TextAfter);
                setEntryText(entry.TagKey, text);
                update();
                return;
        }
}

void isis::gui::DcmtkOverlayWidget::setWindowLevel(double t_windowCenter, double t_windowWidth)
{
        loadConfiguration();
        const unsigned int levelKey = static_cast<unsigned int>(overlayKey::level);
        const unsigned int windowKey = static_cast<unsigned int>(overlayKey::window);

        if (!std::isfinite(t_windowCenter) || !std::isfinite(t_windowWidth) || t_windowWidth <= 0.0)
        {
                setEntryText(windowKey, {});
                setEntryText(levelKey, {});
                update();
                return;
        }

        QString prefix = QStringLiteral("WL: ");
        QString suffix;
        for (const auto& entry : m_entries)
        {
                if (entry.TagKey == windowKey)
                {
                        if (!entry.TextBefore.isEmpty())
                        {
                                prefix = entry.TextBefore;
                        }
                        suffix = entry.TextAfter;
                        break;
                }
        }

        QString text = prefix;
        text.append(formatWindowLevel(t_windowCenter));
        text.append(QStringLiteral("  WW: "));
        text.append(formatWindowLevel(t_windowWidth));
        text.append(suffix.isEmpty() ? QStringLiteral("\n") : suffix);

        setEntryText(windowKey, text);
        setEntryText(levelKey, {});
        update();
}

void isis::gui::DcmtkOverlayWidget::clear()
{
        m_entryTexts.clear();
        update();
}

void isis::gui::DcmtkOverlayWidget::paintEvent(QPaintEvent* t_event)
{
        QWidget::paintEvent(t_event);
        if (!m_configurationLoaded || m_entries.isEmpty())
        {
                return;
        }

        QStringList cornerTexts;
        cornerTexts.reserve(cornerCount);
        bool hasText = false;
        for (int index = 0; index < cornerCount; ++index)
        {
                QString corner = aggregatedTextForCorner(index).trimmed();
                if (!corner.isEmpty())
                {
                        hasText = true;
                }
                cornerTexts.append(corner);
        }

        if (!hasText)
        {
                return;
        }

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        QFont font(QStringLiteral("Segoe UI Semibold"));
        font.setPointSize(11);
        font.setWeight(QFont::DemiBold);
        font.setStyleStrategy(QFont::PreferAntialias);
        painter.setFont(font);

        const QRect contentRect = rect().adjusted(24, 20, -24, -20);
        const int halfHeight = contentRect.height() / 2;
        const QRect topRect(contentRect.left(), contentRect.top(), contentRect.width(), halfHeight);
        const QRect bottomRect(contentRect.left(), contentRect.top() + halfHeight, contentRect.width(),
                contentRect.height() - halfHeight);

        const QColor accentYellow(248, 213, 96);
        const QColor neutralWhite(245, 245, 245);
        const QColor mutedLeft(232, 232, 232);
        const QColor mutedRight(220, 220, 220);
        const QColor mutedTop(210, 210, 210);
        const QColor warningRed(255, 90, 74);

        const auto drawBlock =
                [&](const QRect& target,
                        Qt::Alignment alignment,
                        const QString& text,
                        const QColor& primaryColor, const QColor& secondaryColor)
        {
                if (text.isEmpty())
                {
                        return;
                }

                const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                if (lines.isEmpty())
                {
                        return;
                }

                QFontMetrics metrics(painter.font());
                int lineSpacing = metrics.ascent() + metrics.descent();
                if (metrics.leading() > 0)
                {
                        lineSpacing += metrics.leading() / 2;
                }
                lineSpacing = std::max(lineSpacing, metrics.height() - 2);
                const int totalHeight = lineSpacing * lines.size();
                int y = (alignment & Qt::AlignBottom)
                        ? target.bottom() - totalHeight + metrics.ascent()
                        : target.top() + metrics.ascent();

                for (int i = 0; i < lines.size(); ++i)
                {
                        const QString& line = lines.at(i);
                        const int lineWidth = metrics.horizontalAdvance(line);
                        int x = target.left();
                        if (alignment & Qt::AlignRight)
                        {
                                x = target.right() - lineWidth;
                        }

                        QColor lineColor = (i == 0) ? primaryColor : secondaryColor;
                        const QString trimmed = line.trimmed();
                        if (trimmed.contains(QStringLiteral("Spacing"), Qt::CaseInsensitive)
                                || trimmed.contains(QStringLiteral("Slice"), Qt::CaseInsensitive)
                                || trimmed.startsWith(QStringLiteral("T:"), Qt::CaseInsensitive)
                                || trimmed.startsWith(QStringLiteral("L:"), Qt::CaseInsensitive))
                        {
                                lineColor = warningRed;
                        }

                        painter.setPen(lineColor);
                        painter.drawText(x, y, line);
                        y += lineSpacing;
                }
        };

        drawBlock(topRect.adjusted(0, 0, 0, -8),
                Qt::AlignLeft | Qt::AlignTop,
                cornerTexts.value(0),
                accentYellow,
                accentYellow);

        drawBlock(bottomRect.adjusted(0, 8, 0, 0),
                Qt::AlignLeft | Qt::AlignBottom,
                cornerTexts.value(1),
                accentYellow,
                mutedLeft);

        drawBlock(bottomRect.adjusted(0, 8, 0, 0),
                Qt::AlignRight | Qt::AlignBottom,
                cornerTexts.value(2),
                accentYellow,
                mutedRight);

        drawBlock(topRect.adjusted(0, 0, 0, -8),
                Qt::AlignRight | Qt::AlignTop,
                cornerTexts.value(3),
                neutralWhite,
                mutedTop);
}

void isis::gui::DcmtkOverlayWidget::loadConfiguration()
{
        if (m_configurationLoaded)
        {
                return;
        }

        const auto attemptLoad = [this](const QString& candidate, const bool logFailure) -> bool
        {
                QFile source(candidate);
                if (!source.open(QIODevice::ReadOnly | QIODevice::Text))
                {
                        if (logFailure)
                        {
                                qCWarning(lcDcmtkOverlayWidget)
                                        << "Failed to open overlays configuration" << candidate << source.errorString();
                        }
                        return false;
                }

                const auto document = QJsonDocument::fromJson(source.readAll());
                const auto root = document.object();
                const auto overlays = root.value(QStringLiteral("Overlay")).toArray();
                m_entries.clear();
                m_entryTexts.clear();
                m_entries.reserve(overlays.size());
                for (const auto& item : overlays)
                {
                        const QJsonObject object = item.toObject();
                        OverlayEntry entry;
                        entry.TextBefore = object.value(QStringLiteral("TextBefore")).toString();
                        entry.TextAfter = object.value(QStringLiteral("TextAfter")).toString();
                        entry.TagKey = static_cast<unsigned int>(object.value(QStringLiteral("TagKey")).toInt());
                        entry.Corner = object.value(QStringLiteral("Corner")).toInt();
                        entry.Dynamic = isDynamicTag(entry.TagKey);
                        m_entries.append(entry);
                }
                return true;
        };

        const QString relativePath = QString::fromUtf8(overlaysInformation);
        QStringList searchCandidates;
        searchCandidates.reserve(8);
        searchCandidates.append(QDir::current().filePath(relativePath));

        QDir probeDir(QCoreApplication::applicationDirPath());
        for (int depth = 0; depth < 6; ++depth)
        {
                const QString candidate = probeDir.filePath(relativePath);
                if (!searchCandidates.contains(candidate))
                {
                        searchCandidates.append(candidate);
                }
                if (!probeDir.cdUp())
                {
                        break;
                }
        }

        QString resolvedPath;
        QStringList attemptedPaths;
        attemptedPaths.reserve(searchCandidates.size());
        for (const QString& candidate : searchCandidates)
        {
                attemptedPaths.append(candidate);
                QFileInfo info(candidate);
                if (!info.exists() || !info.isFile())
                {
                        continue;
                }
                if (attemptLoad(candidate, true))
                {
                        resolvedPath = candidate;
                        break;
                }
        }

        if (resolvedPath.isEmpty())
        {
                if (!attemptLoad(QStringLiteral(":/res/overlays.json"), true))
                {
                        qCWarning(lcDcmtkOverlayWidget)
                                << "Failed to load overlays configuration. Search paths:"
                                << attemptedPaths;
                        m_entries.clear();
                        m_entryTexts.clear();
                        m_configurationLoaded = true;
                        return;
                }
        }
        else
        {
                qCInfo(lcDcmtkOverlayWidget)
                        << "Loaded overlays configuration from" << resolvedPath;
        }

        m_configurationLoaded = true;
}

void isis::gui::DcmtkOverlayWidget::populateStaticEntries(const DicomMetadata* t_metadata)
{
        if (!t_metadata)
        {
                return;
        }

        for (const auto& entry : m_entries)
        {
                if (entry.Dynamic)
                {
                        continue;
                }

                const QString text = composeStaticText(entry, t_metadata);
                if (!text.isEmpty())
                {
                        setEntryText(entry.TagKey, text);
                }
        }
}

void isis::gui::DcmtkOverlayWidget::setToolName(const QString& t_toolName)
{
        loadConfiguration();
        const unsigned int toolKey = static_cast<unsigned int>(overlayKey::tool);
        for (const auto& entry : m_entries)
        {
                if (entry.TagKey != toolKey)
                {
                        continue;
                }

                if (t_toolName.isEmpty())
                {
                        setEntryText(entry.TagKey, {});
                        update();
                        return;
                }

                QString prefix = entry.TextBefore;
                if (prefix.isEmpty())
                {
                        prefix = QStringLiteral("Tool: ");
                }
                QString text = prefix;
                text.append(t_toolName);
                text.append(entry.TextAfter);
                setEntryText(entry.TagKey, text);
                update();
                return;
        }
}

void isis::gui::DcmtkOverlayWidget::setSpacing(double t_spacingX, double t_spacingY)
{
        loadConfiguration();
        const unsigned int spacingKey = static_cast<unsigned int>(overlayKey::spacing);
        for (const auto& entry : m_entries)
        {
                if (entry.TagKey != spacingKey)
                {
                        continue;
                }

                const bool hasSpacingX = std::isfinite(t_spacingX) && t_spacingX > 0.0;
                const bool hasSpacingY = std::isfinite(t_spacingY) && t_spacingY > 0.0;
                if (!hasSpacingX && !hasSpacingY)
                {
                        setEntryText(entry.TagKey, {});
                        update();
                        return;
                }

                QStringList parts;
                if (hasSpacingX)
                {
                        parts << formatSpacingValue(t_spacingX);
                }
                if (hasSpacingY)
                {
                        parts << formatSpacingValue(t_spacingY);
                }

                if (parts.isEmpty())
                {
                        setEntryText(entry.TagKey, {});
                        update();
                        return;
                }

                QString text = entry.TextBefore;
                text.append(tr("Spacing %1 mm").arg(parts.join(QStringLiteral(" x "))));
                text.append(entry.TextAfter.isEmpty() ? QStringLiteral("\n") : entry.TextAfter);
                setEntryText(entry.TagKey, text);
                update();
                return;
        }
}

void isis::gui::DcmtkOverlayWidget::setSliceLocation(double t_sliceLocation)
{
        loadConfiguration();
        const unsigned int sliceKey = static_cast<unsigned int>(overlayKey::sliceLocation);
        for (const auto& entry : m_entries)
        {
                if (entry.TagKey != sliceKey)
                {
                        continue;
                }

                if (!std::isfinite(t_sliceLocation))
                {
                        setEntryText(entry.TagKey, {});
                        update();
                        return;
                }

                QString text = entry.TextBefore;
                text.append(tr("L: %1 mm").arg(QString::number(t_sliceLocation, 'f', 2)));
                text.append(entry.TextAfter.isEmpty() ? QStringLiteral("\n") : entry.TextAfter);
                setEntryText(entry.TagKey, text);
                update();
                return;
        }
}

void isis::gui::DcmtkOverlayWidget::setEntryText(unsigned int t_tagKey, const QString& t_text)
{
        if (t_text.trimmed().isEmpty())
        {
                m_entryTexts.remove(t_tagKey);
        }
        else
        {
                m_entryTexts.insert(t_tagKey, t_text);
        }
}

QString isis::gui::DcmtkOverlayWidget::aggregatedTextForCorner(int t_corner) const
{
        QString result;
        for (const auto& entry : m_entries)
        {
                if (entry.Corner != t_corner)
                {
                        continue;
                }
                const QString value = m_entryTexts.value(entry.TagKey);
                if (value.isEmpty())
                {
                        continue;
                }
                result.append(value);
        }
        return result;
}

QString isis::gui::DcmtkOverlayWidget::composeStaticText(const OverlayEntry& t_entry,
        const DicomMetadata* t_metadata) const
{
        if (!t_metadata)
        {
                return {};
        }

        const unsigned short group = static_cast<unsigned short>((t_entry.TagKey >> 16) & 0xFFFF);
        const unsigned short element = static_cast<unsigned short>(t_entry.TagKey & 0xFFFF);
        const DicomTag tag(group, element);
        std::string value = t_metadata->getString(tag);
        core::Utils::processTagFormat(tag, value);
        QString text = t_entry.TextBefore;
        const QString valueText = QString::fromStdString(value).trimmed();
        if (valueText.isEmpty())
        {
                return {};
        }
        text.append(valueText);
        text.append(t_entry.TextAfter);
        if (text.isEmpty() || text == QStringLiteral("\n") || text == QStringLiteral("Series: \n")
                || text == QStringLiteral("Zoom: \n") || text == QStringLiteral("Tool: \n"))
        {
                return {};
        }
        return text;
}

QString isis::gui::DcmtkOverlayWidget::formatSpacingValue(double t_spacing)
{
        if (!std::isfinite(t_spacing) || t_spacing <= 0.0)
        {
                return {};
        }

        const double magnitude = std::abs(t_spacing);
        int precision = 3;
        if (magnitude >= 100.0)
        {
                precision = 1;
        }
        else if (magnitude >= 10.0)
        {
                precision = 2;
        }
        else if (magnitude < 1.0)
        {
                precision = 4;
        }

        return QString::number(t_spacing, 'f', precision);
}

bool isis::gui::DcmtkOverlayWidget::isDynamicTag(unsigned int t_tagKey)
{
        switch (static_cast<overlayKey>(t_tagKey))
        {
        case overlayKey::zoom:
        case overlayKey::series:
        case overlayKey::tool:
        case overlayKey::window:
        case overlayKey::level:
        case overlayKey::spacing:
        case overlayKey::sliceLocation:
                return true;
        default:
                return false;
        }
}

QString isis::gui::DcmtkOverlayWidget::formatZoom(double t_zoom)
{
        if (!std::isfinite(t_zoom))
        {
                return QStringLiteral("NA");
        }
        return QString::number(t_zoom, 'f', 2);
}

QString isis::gui::DcmtkOverlayWidget::formatWindowLevel(double t_value)
{
        if (!std::isfinite(t_value))
        {
                return QStringLiteral("NA");
        }
        const double rounded = std::round(t_value);
        if (std::abs(t_value - rounded) < 0.5)
        {
                return QString::number(static_cast<long long>(rounded));
        }
        return QString::number(t_value, 'f', 1);
}

