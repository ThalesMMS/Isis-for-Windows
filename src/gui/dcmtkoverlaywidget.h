/*
 * ------------------------------------------------------------------------------------
 *  File: dcmtkoverlaywidget.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the DICOM overlay widget responsible for showing corner annotations in VTK views.
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

#pragma once

#include <QHash>
#include <QVector>
#include <QWidget>

#include "dicomvolume.h"

namespace isis::gui
{
        class DcmtkOverlayWidget final : public QWidget
        {
        Q_OBJECT
        public:
                explicit DcmtkOverlayWidget(QWidget* parent = nullptr);
                ~DcmtkOverlayWidget() override = default;

                void setMetadata(const core::DicomMetadata* t_metadata);
                void setSeriesInformation(const QString& t_seriesNumber, int t_imageIndex, int t_imageCount);
                void setZoom(double t_zoomFactor);
                void setWindowLevel(double t_windowCenter, double t_windowWidth);
                void setToolName(const QString& t_toolName);
                void setSpacing(double t_spacingX, double t_spacingY);
                void setSliceLocation(double t_sliceLocation);
                void clear();

        protected:
                void paintEvent(QPaintEvent* t_event) override;

        private:
                struct OverlayEntry
                {
                        QString TextBefore = {};
                        QString TextAfter = {};
                        unsigned int TagKey = 0;
                        int Corner = 0;
                        bool Dynamic = false;
                };

                QVector<OverlayEntry> m_entries = {};
                QHash<unsigned int, QString> m_entryTexts = {};
                bool m_configurationLoaded = false;

                void loadConfiguration();
                void populateStaticEntries(const core::DicomMetadata* t_metadata);
                void setEntryText(unsigned int t_tagKey, const QString& t_text);
                [[nodiscard]] QString aggregatedTextForCorner(int t_corner) const;
                [[nodiscard]] QString composeStaticText(const OverlayEntry& t_entry,
                        const core::DicomMetadata* t_metadata) const;
                [[nodiscard]] static bool isDynamicTag(unsigned int t_tagKey);
                [[nodiscard]] static QString formatZoom(double t_zoom);
                [[nodiscard]] static QString formatWindowLevel(double t_value);
                [[nodiscard]] static QString formatSpacingValue(double t_spacing);
        };
}

