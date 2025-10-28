/*
 * ------------------------------------------------------------------------------------
 *  File: corecontroller.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements CoreController routines for reading DICOM files and
 *      synchronizing parsed data with the repository.
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

#include "corecontroller.h"
#include <QDebug>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QString>
#include <stdexcept>

namespace isis::core
{
        Q_LOGGING_CATEGORY(lcCoreController, "isis.core.controller")
}

namespace
{
        QString sanitizePath(const std::string& path)
        {
                const QFileInfo fileInfo(QString::fromStdString(path));
                return fileInfo.fileName().isEmpty() ? fileInfo.filePath() : fileInfo.fileName();
        }
}

isis::core::CoreController::CoreController()
{
        initData();
}

//-----------------------------------------------------------------------------
void isis::core::CoreController::readData(const std::string& t_filepath) const
{
        const auto sanitizedPath = sanitizePath(t_filepath);
        qInfo(lcCoreController) << "Request to read data from" << sanitizedPath;
        try
        {
                m_dicomReader->readFile(t_filepath);
                if (m_dicomReader->dataSetExists())
                {
                        insertDataInRepo();
                }
                else
                {
                        throw std::runtime_error("File not supported");
                }
        }
        catch (const std::exception& ex)
        {
                qCritical(lcCoreController) << "Exception while reading data from" << sanitizedPath << ":" << ex.what();
                throw;
        }
}

//-----------------------------------------------------------------------------
int isis::core::CoreController::getLastSeriesSize() const
{
        auto* const image = m_coreRepository->getLastImage();
        auto* const series = m_coreRepository->getLastSeries();
        if (!image || !series)
        {
                return 0;
        }
        return image->getIsMultiFrame()
                ? image->getNumberOfFrames()
                : static_cast<int>(series->snapshotSingleFramePaths().size());
}

//-----------------------------------------------------------------------------
void isis::core::CoreController::resetData()
{
	m_coreRepository = std::make_unique<CoreRepository>();
	m_dicomReader = std::make_unique<DicomReader>();
}

//-----------------------------------------------------------------------------
void isis::core::CoreController::initData()
{
	m_coreRepository = std::make_unique<CoreRepository>();
	m_dicomReader = std::make_unique<DicomReader>();
}

//-----------------------------------------------------------------------------
void isis::core::CoreController::insertDataInRepo() const
{
	m_coreRepository->resetLastPatientData();
        m_coreRepository->addPatient(m_dicomReader->getReadPatient());
        m_coreRepository->addStudy(m_dicomReader->getReadStudy());
	auto newSeries = m_dicomReader->getReadSeries();
	if (!newSeries) return;
	m_coreRepository->addSeries(std::move(newSeries));
	m_coreRepository->addImage(m_dicomReader->getReadImage());
        const auto* const lastImage = m_coreRepository->getLastImage();
        auto* const lastSeries = m_coreRepository->getLastSeries();
        const auto frameCount = (!lastImage || !lastSeries)
                ? 0
                : (lastImage->getIsMultiFrame() ? lastImage->getNumberOfFrames()
                                                : static_cast<int>(lastSeries->snapshotSingleFramePaths().size()));
	qInfo(lcCoreController) << "Repository updated with" << frameCount
	                        << (lastImage->getIsMultiFrame() ? "frames from multi-frame image" : "single-frame images");
}
