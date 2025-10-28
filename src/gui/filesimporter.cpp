/*
 * ------------------------------------------------------------------------------------
 *  File: filesimporter.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Handles asynchronous discovery and import of DICOM files and notifies the GUI of new data.
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

#include "filesimporter.h"
#include <QApplication>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include <QFileInfo>
#include <QMutexLocker>
#include <QStringList>

namespace
{
	bool isLikelyDicomPath(const QString& path)
	{
		const QFileInfo info(path);
		const QStringList suffixes = info.completeSuffix().split(QLatin1Char('.'), Qt::SkipEmptyParts);
		if (suffixes.isEmpty())
		{
			return true;
		}

		const auto lastSuffix = suffixes.back().toLower();
		if (lastSuffix == QStringLiteral("dcm")
			|| lastSuffix == QStringLiteral("dicom")
			|| lastSuffix == QStringLiteral("ima"))
		{
			return true;
		}

		if (lastSuffix == QStringLiteral("gz") && suffixes.size() > 1)
		{
			const auto penultimate = suffixes.at(suffixes.size() - 2).toLower();
			if (penultimate == QStringLiteral("dcm")
				|| penultimate == QStringLiteral("dicom")
				|| penultimate == QStringLiteral("ima"))
			{
				return true;
			}
		}

		return false;
	}
}

void isis::gui::FilesImporter::startImporter()
{
	qInfo() << "[FilesImporter] Starting importer thread";
	{
		QMutexLocker locker(&m_filesMutex);
		m_isWorking = true;
	}
	m_filesCondition.wakeAll();
	if (!isRunning())
	{
		start();
	}
}

//-----------------------------------------------------------------------------
void isis::gui::FilesImporter::stopImporter(const QString& reason)
{
	const QString context = reason.isEmpty()
		? QStringLiteral("generic request")
		: reason;
	qInfo() << "[FilesImporter] Stop requested (" << context << ")";
	m_futureFolders.waitForFinished();
	{
		QMutexLocker locker(&m_filesMutex);
		m_isWorking = false;
	}
	m_filesCondition.wakeAll();
	wait();
	{
		QMutexLocker filesLocker(&m_filesMutex);
		m_filesPaths.clear();
	}
	{
		QMutexLocker foldersLocker(&m_foldersMutex);
		m_foldersPaths.clear();
	}
	qInfo() << "[FilesImporter] Importer stopped. Queues cleared.";
}

//-----------------------------------------------------------------------------
void isis::gui::FilesImporter::addFiles(const QStringList& t_paths)
{
	QApplication::setOverrideCursor(Qt::WaitCursor);
	{
		QMutexLocker locker(&m_filesMutex);
		for (const auto& path : t_paths)
		{
			if (!isLikelyDicomPath(path))
			{
				qDebug() << "[FilesImporter] Skipping non-DICOM candidate" << path;
				continue;
			}
			qInfo() << "[FilesImporter] Queueing file" << path;
			m_filesPaths.push_back(path);
		}
	}
	m_filesCondition.wakeAll();
	QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void isis::gui::FilesImporter::addFolders(const QStringList& t_paths)
{
	{
		QMutexLocker foldersLocker(&m_foldersMutex);
		m_foldersPaths.append(t_paths);
	}
	qInfo() << "[FilesImporter] Queueing folders" << t_paths;
	m_futureFolders = QtConcurrent::run(parseFolders, this);
	Q_UNUSED(connect(&m_futureWatcherFolders,
		&QFutureWatcher<void>::finished, this,
		&FilesImporter::parseFoldersFinished));
	m_futureWatcherFolders.setFuture(m_futureFolders);
	m_filesCondition.wakeAll();
}

//-----------------------------------------------------------------------------
void isis::gui::FilesImporter::parseFolders(FilesImporter* t_self)
{
	constexpr int batchSize = 32;
	while (true)
	{
		QString folderPath;
		{
			QMutexLocker folderLocker(&t_self->m_foldersMutex);
			if (t_self->m_foldersPaths.isEmpty())
			{
				break;
			}
			folderPath = t_self->m_foldersPaths.front();
			t_self->m_foldersPaths.pop_front();
		}

		qInfo() << "[FilesImporter] Parsing folder" << folderPath;
		QStringList discoveredFiles;
		const auto flushDiscovered = [&](QStringList& batch)
		{
			if (batch.isEmpty())
			{
				return;
			}
			{
				QMutexLocker filesLocker(&t_self->m_filesMutex);
				for (const auto& filePath : batch)
				{
					t_self->m_filesPaths.push_back(filePath);
				}
			}
			t_self->m_filesCondition.wakeAll();
			qInfo() << "[FilesImporter] Enqueued batch of" << batch.size()
				<< "files from" << folderPath;
			batch.clear();
		};

		for (QDirIterator it(folderPath, QDir::Files, QDirIterator::Subdirectories);
			it.hasNext();)
		{
			const QString nextPath = it.next();
			if (!isLikelyDicomPath(nextPath))
			{
				qDebug() << "[FilesImporter] Skipping non-DICOM candidate" << nextPath;
				continue;
			}
			discoveredFiles.push_back(nextPath);
			if (discoveredFiles.size() >= batchSize)
			{
				flushDiscovered(discoveredFiles);
			}
		}

		flushDiscovered(discoveredFiles);
		qInfo() << "[FilesImporter] Finished folder" << folderPath;
	}
}

//-----------------------------------------------------------------------------
bool isis::gui::FilesImporter::newSeries() const
{
	const auto seriesAdded = m_coreController->newSeriesAdded();
	const auto imageAdded = m_coreController->newImageAdded();
	const auto lastImage = m_coreController->getLastImage();
	const auto isMultiFrame = lastImage && lastImage->getIsMultiFrame();
	const auto result = seriesAdded || (imageAdded && isMultiFrame);
	qInfo() << "[FilesImporter] newSeries check -> seriesAdded:" << seriesAdded
		<< "imageAdded:" << imageAdded << "multiFrame:" << isMultiFrame
		<< "result:" << result;
	return result;
}

//-----------------------------------------------------------------------------
void isis::gui::FilesImporter::run()
{
	while (true)
	{
		QString nextFile;
		{
			QMutexLocker locker(&m_filesMutex);
			while (m_filesPaths.empty() && m_isWorking)
			{
				m_filesCondition.wait(&m_filesMutex);
			}
			if (!m_isWorking)
			{
				return;
			}
			nextFile = m_filesPaths.front();
			m_filesPaths.pop_front();
		}
		importFile(nextFile);
	}
}

//-----------------------------------------------------------------------------
void isis::gui::FilesImporter::parseFoldersFinished() const
{
	disconnect(&m_futureWatcherFolders,
		&QFutureWatcher<void>::finished, this,
		&FilesImporter::parseFoldersFinished);
}

//-----------------------------------------------------------------------------
void isis::gui::FilesImporter::importFile(const QString& t_path)
{
	if (!isLikelyDicomPath(t_path))
	{
		qWarning() << "[FilesImporter] Ignoring non-DICOM path" << t_path;
		return;
	}
	m_coreController->readData(t_path.toStdString());
	qInfo() << "[FilesImporter] Processed file" << t_path;
        const bool createdNewSeries = newSeries();
        if (createdNewSeries)
        {
                qInfo() << "[FilesImporter] Emitting populate signals "
                        << "Patient index:" << m_coreController->getLastPatientIndex()
			<< "Study index:" << m_coreController->getLastStudyIndex()
			<< "Series index:" << m_coreController->getLastSeriesIndex()
			<< "Image index:" << m_coreController->getLastImageIndex();
		emit addNewThumbnail(m_coreController->getLastPatient(),
			m_coreController->getLastStudy(),
			m_coreController->getLastSeries(),
			m_coreController->getLastImage());
		emit populateWidget(m_coreController->getLastSeries(),
			m_coreController->getLastImage());
        }
        const bool hasDisplayableSeries = m_coreController->getLastSeries() != nullptr;
        if (hasDisplayableSeries)
        {
                emit showThumbnailsWidget(true);
        }
        auto* const lastImage = m_coreController->getLastImage();
        if(lastImage && !lastImage->getIsMultiFrame())
        {
		emit refreshScrollValues(m_coreController->getLastSeries(),
			m_coreController->getLastImage());
	}
}
