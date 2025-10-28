/*
 * ------------------------------------------------------------------------------------
 *  File: studylist.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the StudyList model storing studies grouped under patients for the viewer.
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

#include <QFuture>
#include <QString>
class QResizeEvent;
#include "patient.h"
#include "ui_studylist.h"
#include "seriesitem.h"


namespace isis::gui
{
	class StudyList final : public QListWidget
	{
	Q_OBJECT

	public:
		explicit StudyList(QWidget* parent = Q_NULLPTR);
		~StudyList() = default;

		//getters
		[[nodiscard]] core::Patient* getPatient() const { return m_patient; }
		[[nodiscard]] core::Study* getStudy() const { return m_study; }
		[[nodiscard]] std::vector<QFuture<void>> getFutures() const { return m_futures; }

		//setters
		void setPatient(core::Patient* t_patient) { m_patient = t_patient; }
		void setStudy(core::Study* t_study) { m_study = t_study; }

		void insertNewSeries(core::Series* t_series, core::Image* t_image);
                bool updateSeriesProgress(core::Series* t_series, int frameIndex, int frameCount);
                void applyDarkPalette();

	signals:
		void finishConcurrent();

	protected:
		void startDrag(Qt::DropActions supportedActions) override;
                void resizeEvent(QResizeEvent* event) override;

	private slots:
		void cleanUp();

	private:
		Ui::StudyList m_ui = {};
		core::Patient* m_patient = {};
		core::Study* m_study = {};
		std::vector<QFuture<void>> m_futures = {};
                QString m_darkStyleSheet = {};

		void initView();
                void ensureDarkStyleSheet();
                void updateItemSizes();
                [[nodiscard]] int desiredItemWidth() const;
		[[nodiscard]] QString seriesLegend(core::Series* t_series, int currentFrame, int totalFrames) const;
                [[nodiscard]] int totalFramesForSeries(core::Series* t_series) const;
		[[nodiscard]] QString createMimeData(core::Series* t_series, core::Image* t_image);
		static void createImageForItem(StudyList* t_self, core::Image* t_image, SeriesItem* t_item);
	};
}

