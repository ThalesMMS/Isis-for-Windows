/*
 * ------------------------------------------------------------------------------------
 *  File: thumbnailswidget.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the ThumbnailsWidget that shows preview images for imported series.
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

#include <QTabWidget>
#include "ui_thumbnailswidget.h"


namespace isis::core
{
	class Patient;
	class Study;
	class Series;
	class Image;
}

namespace isis::gui
{
	class ThumbnailsWidget final : public QWidget
	{
	Q_OBJECT

	public:
		explicit ThumbnailsWidget(QWidget* parent = Q_NULLPTR);
		~ThumbnailsWidget() = default;

		void resetData() const;
                void applyDarkPalette();
                void updateSeriesProgress(core::Series* t_series, int frameIndex, int frameCount) const;

	public slots:
		void addThumbnail(core::Patient* t_patient,
			core::Study* t_study,
			core::Series* t_series,
			core::Image* t_image) const;

	private:
		Ui::ThumbnailsWidget m_ui = {};
		QTabWidget* m_patientsTabs = {};

		void initView();
		void initData();
		[[nodiscard]] bool tryInsertExistingItem(core::Patient* t_patient,
			core::Study* t_study,
			core::Series* t_series,
			core::Image* t_image) const;
		void insertNewItem(core::Patient* t_patient,
			core::Study* t_study,
			core::Series* t_series,
			core::Image* t_image) const;
	};
}

