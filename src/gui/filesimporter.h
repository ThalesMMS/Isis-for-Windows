/*
 * ------------------------------------------------------------------------------------
 *  File: filesimporter.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the FilesImporter worker that scans folders and feeds DICOM data to the core controller.
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

#include <deque>
#include <qfuture.h>
#include <qfuturewatcher.h>
#include <qmutex.h>
#include <QThread>
#include <QWaitCondition>
#include "corecontroller.h"

namespace isis::gui
{
	class FilesImporter final : public QThread
	{
	Q_OBJECT

	public:
		explicit FilesImporter(QObject* parent) : QThread(Q_NULLPTR),
			m_coreController(std::make_unique<core::CoreController>()) {}
		~FilesImporter() = default;

		void startImporter();
		void stopImporter(const QString& reason = {});
		void addFiles(const QStringList& t_paths);
		void addFolders(const QStringList& t_paths);
		core::CoreController* getCoreController() const { return m_coreController.get(); }

	signals:
		void addNewThumbnail(core::Patient* t_patient,
			core::Study* t_study, core::Series* t_series, core::Image* t_image);
		void populateWidget(core::Series* t_series, core::Image* t_image);
		void refreshScrollValues(core::Series* t_series, core::Image* t_image);
		void showThumbnailsWidget(const bool& t_flag);

	protected:
		void run() override;

	private slots:
		void parseFoldersFinished() const;

	private:
		QMutex m_filesMutex;
		QMutex m_foldersMutex;
		QWaitCondition m_filesCondition;
		QFuture<void> m_futureFolders;
		QFutureWatcher<void> m_futureWatcherFolders;
		std::unique_ptr<core::CoreController> m_coreController = {};
		std::deque<QString> m_filesPaths;
		QStringList m_foldersPaths;
		bool m_isWorking = false;

		void importFile(const QString& t_path);
		static void parseFolders(FilesImporter* t_self);
		[[nodiscard]] bool newSeries() const;
	};
}

