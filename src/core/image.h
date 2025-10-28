/*
 * ------------------------------------------------------------------------------------
 *  File: image.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Defines the Image domain object that stores DICOM instance metadata and
 *      provides access to associated voxel data.
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

#include <array>
#include <limits>
#include <memory>

#include <QString>
#include <vtkSmartPointer.h>
#include "utils.h"
#include "dicomvolume.h"

namespace isis::core
{
	class Series;

	class Image
	{
	public:
		Image() = default;
		~Image() = default;

		//getters
		[[nodiscard]] export Series* getParentObject() const { return m_parent; }
		[[nodiscard]] export std::string getImagePath() const { return m_path; }
		[[nodiscard]] export std::string getSOPInstanceUID() const { return m_sopInstanceUid; }
		[[nodiscard]] export std::string getClassUID() const { return m_classUid; }
		[[nodiscard]] export std::string getFrameOfRefernceID() const { return m_frameOfReferenceId; }
		[[nodiscard]] export std::string getModality() const { return m_modality; }
		[[nodiscard]] export int getWindowCenter() const { return m_windowsCenter; }
		[[nodiscard]] export int getWindowWidth() const { return m_windowWidth; }
		[[nodiscard]] export int getRows() const { return m_rows; }
		[[nodiscard]] export int getColumns() const { return m_columns; }
		[[nodiscard]] export int getNumberOfFrames() const { return m_numberOfFrames; }
		[[nodiscard]] export double getSliceLocation() const { return m_sliceLocation; }
		[[nodiscard]] export int getAcquisitionNumber() const { return m_acquisitionNumber; }
		[[nodiscard]] export bool getIsMultiFrame() const { return m_isMultiframe; }
		[[nodiscard]] export int getIndex() const { return m_index; }
		[[nodiscard]] export double getPixelSpacingX() const { return m_pixelSpacingX; }
		[[nodiscard]] export double getPixelSpacingY() const { return m_pixelSpacingY; }
		[[nodiscard]] export int getInstanceNumber() const { return m_instanceNumber; }
		[[nodiscard]] export const std::array<double, 3>& getImagePositionPatient() const { return m_imagePositionPatient; }
                [[nodiscard]] export bool hasImagePositionPatient() const { return m_hasImagePositionPatient; }
                [[nodiscard]] export const std::array<double, 3>& getImageOrientationRow() const { return m_imageOrientationRow; }
                [[nodiscard]] export const std::array<double, 3>& getImageOrientationColumn() const { return m_imageOrientationColumn; }
                [[nodiscard]] export bool hasImageOrientationPatient() const { return m_hasImageOrientationPatient; }
		
                [[nodiscard]] export std::shared_ptr<DicomVolume> getDicomVolume(
                        QString* t_failureReason = nullptr) const;

		//setters
		void export setParentObject(Series* t_parent) { m_parent = t_parent; }
		void export setImagePath(const std::string& t_path) { m_path = t_path; }
		void export setSOPInstanceUID(const std::string& t_sopInstanceUid) { m_sopInstanceUid = t_sopInstanceUid; }
		void export setClassUID(const std::string& t_classUid) { m_classUid = t_classUid; }
		void export setFrameOfRefernceID(const std::string& t_frameOfReferenceId) { m_frameOfReferenceId = t_frameOfReferenceId; }
		void export setModality(const std::string& t_modality) { m_modality = t_modality; }
		void export setWindowCenter(const int& t_windowsCenter) { m_windowsCenter = t_windowsCenter; }
		void export setWindowWidth(const int& t_windowWidth) { m_windowWidth = t_windowWidth; }
		void export setRows(const int& t_rows) { m_rows = t_rows; }
		void export setColumns(const int& t_columns) { m_columns = t_columns; }
		void export setNumberOfFrames(const int& t_numberOfFrames) { m_numberOfFrames = t_numberOfFrames; }
		void export setSliceLocation(const double& t_sliceLocation) { m_sliceLocation = t_sliceLocation; }
		void export setAcquisitionNumber(const int& t_acquisitionNumber) { m_acquisitionNumber = t_acquisitionNumber; }
		void export setIsMultiFrame(const bool& t_isMultiframe) { m_isMultiframe = t_isMultiframe; }
		void export setIndex(const int& t_index) { m_index = t_index; }
		void export setPixelSpacingX(const double& t_spacing) { m_pixelSpacingX = t_spacing; }
		void export setPixelSpacingY(const double& t_spacing) { m_pixelSpacingY = t_spacing; }
		void export setInstanceNumber(const int& t_number) { m_instanceNumber = t_number; }
		void export setImagePositionPatient(const std::array<double, 3>& t_position)
		{
			m_imagePositionPatient = t_position;
			m_hasImagePositionPatient = true;
		}
		void export clearImagePositionPatient()
		{
			m_imagePositionPatient = {std::numeric_limits<double>::quiet_NaN(),
				std::numeric_limits<double>::quiet_NaN(),
				std::numeric_limits<double>::quiet_NaN()};
			m_hasImagePositionPatient = false;
		}
		void export setImageOrientationPatient(const std::array<double, 3>& t_row,
			const std::array<double, 3>& t_column)
		{
			m_imageOrientationRow = t_row;
			m_imageOrientationColumn = t_column;
			m_hasImageOrientationPatient = true;
		}
		void export clearImageOrientationPatient()
		{
			m_imageOrientationRow = {
				std::numeric_limits<double>::quiet_NaN(),
				std::numeric_limits<double>::quiet_NaN(),
				std::numeric_limits<double>::quiet_NaN()};
			m_imageOrientationColumn = {
				std::numeric_limits<double>::quiet_NaN(),
				std::numeric_limits<double>::quiet_NaN(),
				std::numeric_limits<double>::quiet_NaN()};
			m_hasImageOrientationPatient = false;
		}
		
		/**
		* Functor for set compare
		*/
		struct imageCompare
		{
			bool operator()(const std::unique_ptr<Image>& t_lhs, const std::unique_ptr<Image>& t_rhs) const
			{
				return isLess(t_lhs.get(), t_rhs.get());
			}
		};

		export bool equal(Image* t_image) const;

	private:
		std::size_t m_index = -1;
		Series* m_parent = {};
		std::string m_path = {};
		std::string m_sopInstanceUid = {};
		std::string m_classUid = {};
		std::string m_frameOfReferenceId = {};
		std::string m_modality = {};
		int m_instanceNumber = {};
		int m_windowsCenter = {};
		int m_windowWidth = {};
		int m_rows = {};
		int m_columns = {};
		int m_numberOfFrames = {};
		double m_sliceLocation = {};
		double m_pixelSpacingX = -1;
		double m_pixelSpacingY = -1;
		int m_acquisitionNumber = {};
		bool m_isMultiframe = false;
		bool m_hasImagePositionPatient = false;
		bool m_hasImageOrientationPatient = false;
		std::array<double, 3> m_imagePositionPatient = {
			std::numeric_limits<double>::quiet_NaN(),
			std::numeric_limits<double>::quiet_NaN(),
			std::numeric_limits<double>::quiet_NaN()};
		std::array<double, 3> m_imageOrientationRow = {
			std::numeric_limits<double>::quiet_NaN(),
			std::numeric_limits<double>::quiet_NaN(),
			std::numeric_limits<double>::quiet_NaN()};
		std::array<double, 3> m_imageOrientationColumn = {
			std::numeric_limits<double>::quiet_NaN(),
			std::numeric_limits<double>::quiet_NaN(),
			std::numeric_limits<double>::quiet_NaN()};
		mutable std::shared_ptr<DicomVolume> m_volume = {};

		static bool isLess(Image* t_lhs, Image* t_rhs);
	};
}

