/*
 * ------------------------------------------------------------------------------------
 *  File: dicomvolume.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares DICOM volume data structures and the loader interface used to
 *      assemble voxel data, metadata, and geometry.
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
#include <string>
#include <unordered_map>
#include <vector>

#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkSmartPointer.h>

namespace isis::core
{
	struct DicomTag
	{
		unsigned short Group = 0;
		unsigned short Element = 0;

		constexpr DicomTag() = default;

		constexpr DicomTag(unsigned short t_group, unsigned short t_element)
			: Group(t_group)
			, Element(t_element)
		{
		}

		[[nodiscard]] constexpr unsigned int packed() const
		{
			return (static_cast<unsigned int>(Group) << 16) | static_cast<unsigned int>(Element);
		}
	};

	struct DicomMetadata
	{
		std::unordered_map<unsigned int, std::string> Values;

		[[nodiscard]] std::string getString(const DicomTag& tag) const
		{
			const auto it = Values.find(tag.packed());
			return it != Values.end() ? it->second : std::string();
		}
	};

	struct DicomGeometry
	{
		double Origin[3] = {0.0, 0.0, 0.0};
		double Spacing[3] = {1.0, 1.0, 1.0};
		double RowDirection[3] = {1.0, 0.0, 0.0};
		double ColumnDirection[3] = {0.0, 1.0, 0.0};
		double NormalDirection[3] = {0.0, 0.0, 1.0};
	};

	struct DicomPixelInfo
	{
		int SamplesPerPixel = 1;
		int BitsAllocated = 16;
		bool IsSigned = false;
		bool OriginalIsSigned = false;
		bool IsPlanar = false;
		bool InvertMonochrome = false;
		double RescaleSlope = 1.0;
		double RescaleIntercept = 0.0;
		double WindowCenter = 0.0;
		double WindowWidth = 0.0;
                struct WindowPreset
                {
                        double Center = 0.0;
                        double Width = 0.0;
                        std::string Explanation;
                };
                std::vector<WindowPreset> WindowPresets = {};
	};

        using DicomWindowPreset = DicomPixelInfo::WindowPreset;

	struct DicomVolume
	{
		vtkSmartPointer<vtkImageData> ImageData = {};
		vtkSmartPointer<vtkMatrix4x4> Direction = {};
		DicomGeometry Geometry = {};
		DicomPixelInfo PixelInfo = {};
		DicomMetadata Metadata = {};
		int NumberOfFrames = 1;
		std::vector<std::string> SourceFiles = {};
	};

        class DicomVolumeLoader
        {
        public:
                static std::shared_ptr<DicomVolume> loadImage(const std::string& path);
                static std::shared_ptr<DicomVolume> loadSeries(
                        const std::string& studyInstanceUid,
                        const std::string& seriesInstanceUid,
                        const std::vector<std::string>& slicePaths);
                static void invalidateSeriesCache(const std::string& studyInstanceUid,
                        const std::string& seriesInstanceUid);
                static void invalidateStudyCache(const std::string& studyInstanceUid);
                static bool diagnoseStudy(const std::string& path);

        private:
                static void populateDirectionMatrix(DicomVolume& volume);
        };
}

