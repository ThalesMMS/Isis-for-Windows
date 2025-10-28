/*
 * ------------------------------------------------------------------------------------
 *  File: corecontroller.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the CoreController facade that orchestrates DICOM ingestion and
 *      exposes accessors for the in-memory repository state.
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

#include <memory>
#include <QLoggingCategory>
#include "corerepository.h"
#include "utils.h"
#include "dicomreader.h"

namespace isis::core
{
        Q_DECLARE_LOGGING_CATEGORY(lcCoreController)

        class export CoreController
        {
	public:
		CoreController();
		~CoreController() = default;

		void readData(const std::string& t_filepath) const;
		[[nodiscard]] std::vector<std::unique_ptr<Patient>>& getPatients() const { return m_coreRepository->getPatients(); }
		[[nodiscard]] Patient* getLastPatient() const { return m_coreRepository ? m_coreRepository->getLastPatient() : nullptr; }
		[[nodiscard]] int getLastPatientIndex() const { return m_coreRepository && m_coreRepository->getLastPatient() ? m_coreRepository->getLastPatient()->getIndex() : -1; }
		[[nodiscard]] Study* getLastStudy() const { return m_coreRepository ? m_coreRepository->getLastStudy() : nullptr; }
		[[nodiscard]] int getLastStudyIndex() const { return m_coreRepository && m_coreRepository->getLastStudy() ? m_coreRepository->getLastStudy()->getIndex() : -1; }
		[[nodiscard]] Series* getLastSeries() const { return m_coreRepository ? m_coreRepository->getLastSeries() : nullptr; }
		[[nodiscard]] int getLastSeriesSize() const;
		[[nodiscard]] int getLastSeriesIndex() const { return m_coreRepository && m_coreRepository->getLastSeries() ? m_coreRepository->getLastSeries()->getIndex() : -1; }
		[[nodiscard]] Image* getLastImage()  const { return m_coreRepository ? m_coreRepository->getLastImage() : nullptr; }
		[[nodiscard]] int getLastImageIndex() const { return m_coreRepository && m_coreRepository->getLastImage() ? m_coreRepository->getLastImage()->getIndex() : -1; }
		[[nodiscard]] bool newSeriesAdded() const { return m_coreRepository && m_coreRepository->newSeriesAdded(); }
		[[nodiscard]] bool newImageAdded() const { return m_coreRepository && m_coreRepository->newImageAdded(); }

		void resetData();

private:
		std::unique_ptr<CoreRepository> m_coreRepository = std::make_unique<CoreRepository>();
		std::unique_ptr<DicomReader> m_dicomReader = std::make_unique<DicomReader>();

		void initData();
		void insertDataInRepo() const;
	};
}


