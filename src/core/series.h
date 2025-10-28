/*
 * ------------------------------------------------------------------------------------
 *  File: series.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the Series model responsible for organizing images, managing
 *      single and multi-frame collections, and exposing cached volumes.
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
#include <set>
#include <shared_mutex>
#include <vector>

#include "dicomvolume.h"
#include "image.h"
#include "utils.h"

namespace isis::core
{
	class Study;

	class Series
	{
	public:
		Series() = default;
		~Series() = default;

		//getters
		[[nodiscard]] export Study* getParentObject() const { return m_parent; }
		[[nodiscard]] export std::string getUID() const { return m_uid; }
		[[nodiscard]] export std::string getDescription() const { return m_desctiption; }
		[[nodiscard]] export std::string getDate() const { return m_date; }
		[[nodiscard]] export std::string getNumber() const { return m_number; }
                [[nodiscard]] export Image* getNextSingleFrameImage(Image* t_image);
                [[nodiscard]] export Image* getPreviousSingleFrameImage(Image* t_image);
                [[nodiscard]] export Image* getSingleFrameImageByIndex(const int& t_index);
                [[nodiscard]] export std::set<std::unique_ptr<Image>, Image::imageCompare>& getSinlgeFrameImages();
                [[nodiscard]] export std::set<std::unique_ptr<Image>, Image::imageCompare>& getSingleFrameImages();
                [[nodiscard]] export const std::set<std::unique_ptr<Image>, Image::imageCompare>& getSingleFrameImages() const;
                [[nodiscard]] export std::set<std::unique_ptr<Image>, Image::imageCompare>& getMultiFrameImages();
                [[nodiscard]] export const std::set<std::unique_ptr<Image>, Image::imageCompare>& getMultiFrameImages() const;
                [[nodiscard]] export int getIndex() const { return m_index; }
                [[nodiscard]] export std::shared_ptr<DicomVolume> getVolumeForSingleFrameSeries();
                [[nodiscard]] export const DicomMetadata* getMetadataForSeries();
                [[nodiscard]] export std::vector<std::string> snapshotSingleFramePaths() const;
		
		//setters
		export void setParentObject(Study* t_parent) { m_parent = t_parent; }
		export void setUID(const std::string& t_uid) { m_uid = t_uid; }
		export void setDescription(const std::string& t_description) { m_desctiption = t_description; }
		export void setDate(const std::string& t_date) { m_date = t_date; }
		export void setNumber(const std::string& t_number) { m_number = t_number; }
		export void setIndex(const int& t_index) { m_index = t_index; }
		
		[[nodiscard]] export Image* addSingleFrameImage(std::unique_ptr<Image> t_image, bool& t_newImage);
		[[nodiscard]] export Image* addMultiFrameImage(std::unique_ptr<Image> t_image, bool& t_newImage);
		[[nodiscard]] export std::size_t findImageIndex(const std::set<std::unique_ptr<Image>, Image::imageCompare>& t_images,
			Image* t_image);

		/**
		* Functor for set compare
		*/
		struct seriesCompare
		{
			bool operator()(const std::unique_ptr<Series>& t_lhs, const std::unique_ptr<Series>& t_rhs) const
			{
				return isLess(t_lhs.get(), t_rhs.get());
			}
		};

        private:
                std::size_t m_index = -1;
                Study* m_parent = {};
                std::string m_uid = {};
                std::string m_desctiption = {};
                std::string m_date = {};
                std::string m_number = {};
                std::set<std::unique_ptr<Image>, Image::imageCompare> m_singleFrameImages = {};
                std::set<std::unique_ptr<Image>, Image::imageCompare> m_multiFrameImages = {};
                mutable std::shared_mutex m_imagesMutex = {};
                std::unique_ptr<DicomMetadata> m_cachedMetadata = {};

                static bool isLess(Series* t_lhs, Series* t_rhs);

        };
}

