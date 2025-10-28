/*
 * ------------------------------------------------------------------------------------
 *  File: dicomreader.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the DicomReader utility that parses files with DCMTK and
 *      produces domain objects for the repository.
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

#include "patient.h"

#include <gdcmFile.h>
#include <gdcmTag.h>
#include <QLoggingCategory>

namespace isis::core
{
        Q_DECLARE_LOGGING_CATEGORY(lcDicomReader)

        class DicomReader
        {
	public:
		DicomReader() = default;
		~DicomReader() = default;

		void readFile(const std::string& t_filePath);
		[[nodiscard]] std::unique_ptr<Patient> getReadPatient() const;
		[[nodiscard]] std::unique_ptr<Study> getReadStudy() const;
		[[nodiscard]] std::unique_ptr<Series> getReadSeries() const;
		[[nodiscard]] std::unique_ptr<Image> getReadImage() const;
		[[nodiscard]] bool dataSetExists() const { return m_hasFile; }

	private:
		gdcm::File m_file = {};
		bool m_hasFile = false;
		std::string m_filePath = {};

		[[nodiscard]] std::string getTagValue(const gdcm::Tag& tag) const;
		[[nodiscard]] double getNumericValue(const gdcm::Tag& tag,
		        double defaultValue,
		        unsigned int index = 0) const;
		[[nodiscard]] static bool isModalitySupported(const std::string& t_modality);
		[[nodiscard]] std::tuple<int, int> getWindowLevel() const;
		[[nodiscard]] std::pair<double, double> computeWindowLevelFromPixels() const;
		[[nodiscard]] std::tuple<double, double> getPixelSpacing() const;
	};
}

