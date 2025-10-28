/*
 * ------------------------------------------------------------------------------------
 *  File: series.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implements Series operations for navigating images, maintaining cache
 *      state, and creating volumetric representations.
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

#include "series.h"
#include "study.h"

#include <algorithm>
#include <QLoggingCategory>
#include <QString>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>

Q_LOGGING_CATEGORY(lcSeries, "isis.core.series")

isis::core::Image* isis::core::Series::getNextSingleFrameImage(Image* t_image)
{
        std::shared_lock lock(m_imagesMutex);
        if (!t_image || m_singleFrameImages.empty())
        {
                return t_image;
        }
        if (t_image->equal(m_singleFrameImages.rbegin()->get()))
        {
                return t_image;
        }
        const auto index = findImageIndex(m_singleFrameImages, t_image);
        auto it = m_singleFrameImages.begin();
        std::advance(it, index + 1);
        return it->get();
}

//-----------------------------------------------------------------------------
isis::core::Image* isis::core::Series::getPreviousSingleFrameImage(Image* t_image)
{
        std::shared_lock lock(m_imagesMutex);
        if (!t_image || m_singleFrameImages.empty())
        {
                return t_image;
        }
        if (t_image->equal(m_singleFrameImages.begin()->get()))
        {
                return t_image;
        }
        const auto index = findImageIndex(m_singleFrameImages, t_image);
        auto it = m_singleFrameImages.begin();
        std::advance(it, index - 1);
        return it->get();
}

//-----------------------------------------------------------------------------
isis::core::Image* isis::core::Series::getSingleFrameImageByIndex(const int& t_index)
{
        std::shared_lock lock(m_imagesMutex);
        if (t_index < 0 || static_cast<std::size_t>(t_index) >= m_singleFrameImages.size())
        {
                throw std::runtime_error("Index is out of bounds!");
        }
        auto it = m_singleFrameImages.begin();
        std::advance(it, t_index);
        return it->get();
}

//-----------------------------------------------------------------------------
std::set<std::unique_ptr<isis::core::Image>, isis::core::Image::imageCompare>&
isis::core::Series::getSinlgeFrameImages()
{
        [[maybe_unused]] std::shared_lock lock(m_imagesMutex);
        return m_singleFrameImages;
}

//-----------------------------------------------------------------------------
std::set<std::unique_ptr<isis::core::Image>, isis::core::Image::imageCompare>&
isis::core::Series::getSingleFrameImages()
{
        [[maybe_unused]] std::shared_lock lock(m_imagesMutex);
        return m_singleFrameImages;
}

//-----------------------------------------------------------------------------
const std::set<std::unique_ptr<isis::core::Image>, isis::core::Image::imageCompare>&
isis::core::Series::getSingleFrameImages() const
{
        [[maybe_unused]] std::shared_lock lock(m_imagesMutex);
        return m_singleFrameImages;
}

//-----------------------------------------------------------------------------
std::set<std::unique_ptr<isis::core::Image>, isis::core::Image::imageCompare>&
isis::core::Series::getMultiFrameImages()
{
        [[maybe_unused]] std::shared_lock lock(m_imagesMutex);
        return m_multiFrameImages;
}

//-----------------------------------------------------------------------------
const std::set<std::unique_ptr<isis::core::Image>, isis::core::Image::imageCompare>&
isis::core::Series::getMultiFrameImages() const
{
        [[maybe_unused]] std::shared_lock lock(m_imagesMutex);
        return m_multiFrameImages;
}

//-----------------------------------------------------------------------------
std::shared_ptr<isis::core::DicomVolume> isis::core::Series::getVolumeForSingleFrameSeries()
{
        auto paths = snapshotSingleFramePaths();
        if (paths.empty())
        {
                qCWarning(lcSeries) << "No valid file paths were collected for the series volume.";
                return nullptr;
        }
        const std::string studyUid = (m_parent) ? m_parent->getUID() : std::string();
        const std::string& seriesUid = getUID();
        try
        {
                return DicomVolumeLoader::loadSeries(studyUid, seriesUid, paths);
        }
        catch (const std::exception& ex)
        {
                const auto uidLabel = seriesUid.empty()
                        ? QStringLiteral("<empty>")
                        : QString::fromStdString(seriesUid);
                qCCritical(lcSeries)
                        << "Failed to load series volume for"
                        << uidLabel
                        << ":" << ex.what();
        }
        return nullptr;
}

//-----------------------------------------------------------------------------
const isis::core::DicomMetadata* isis::core::Series::getMetadataForSeries()
{
        if (m_cachedMetadata)
        {
                return m_cachedMetadata.get();
        }

        const auto volume = getVolumeForSingleFrameSeries();
        if (!volume)
        {
                m_cachedMetadata.reset();
                return nullptr;
        }

        m_cachedMetadata = std::make_unique<DicomMetadata>(volume->Metadata);
        return m_cachedMetadata.get();
}

//-----------------------------------------------------------------------------
isis::core::Image* isis::core::Series::addSingleFrameImage(std::unique_ptr<Image> t_image, bool& t_newImage)
{
        std::unique_lock lock(m_imagesMutex);
        auto index = findImageIndex(m_singleFrameImages, t_image.get());
        t_newImage = false;
        if (index == m_singleFrameImages.size())
        {
                m_singleFrameImages.emplace(std::move(t_image));
                index = m_singleFrameImages.size() - 1;
                t_newImage = true;
                const std::string studyUid = (m_parent) ? m_parent->getUID() : std::string();
                DicomVolumeLoader::invalidateSeriesCache(studyUid, getUID());
                m_cachedMetadata.reset();
        }

        std::size_t currentIndex = 0;
        for (auto& imageEntry : m_singleFrameImages)
        {
                if (imageEntry)
                {
                        imageEntry->setIndex(static_cast<int>(currentIndex));
                }
                ++currentIndex;
        }

	auto it = m_singleFrameImages.begin();
	std::advance(it, index);
	return it->get();
}

//-----------------------------------------------------------------------------
isis::core::Image* isis::core::Series::addMultiFrameImage(std::unique_ptr<Image> t_image, bool& t_newImage)
{
        std::unique_lock lock(m_imagesMutex);
        auto index = findImageIndex(m_multiFrameImages, t_image.get());
        t_newImage = false;
        if (index == m_multiFrameImages.size())
        {
                m_multiFrameImages.emplace(std::move(t_image));
                index = m_multiFrameImages.size() - 1;
                t_newImage = true;
                const std::string studyUid = (m_parent) ? m_parent->getUID() : std::string();
                DicomVolumeLoader::invalidateSeriesCache(studyUid, getUID());
                m_cachedMetadata.reset();
        }

        std::size_t currentIndex = 0;
        for (auto& imageEntry : m_multiFrameImages)
        {
                if (imageEntry)
                {
                        imageEntry->setIndex(static_cast<int>(currentIndex));
                }
                ++currentIndex;
        }

	auto it = m_multiFrameImages.begin();
	std::advance(it, index);
	return it->get();
}

//-----------------------------------------------------------------------------
bool isis::core::Series::isLess(Series* t_lhs, Series* t_rhs)
{
	return t_lhs->getUID() < t_rhs->getUID();
}

//-----------------------------------------------------------------------------
std::size_t isis::core::Series::findImageIndex(
        const std::set<std::unique_ptr<Image>, Image::imageCompare>& t_images, Image* t_image)
{
        auto const it = std::find_if(t_images.begin(), t_images.end(),
                                     [&](const std::unique_ptr<Image>& image) { return image->equal(t_image); });
        return std::distance(t_images.begin(), it);
}

//-----------------------------------------------------------------------------
std::vector<std::string> isis::core::Series::snapshotSingleFramePaths() const
{
        std::shared_lock lock(m_imagesMutex);
        std::vector<std::string> paths;
        paths.reserve(m_singleFrameImages.size());
        for (const auto& image : m_singleFrameImages)
        {
                if (!image)
                {
                        continue;
                }
                const auto& path = image->getImagePath();
                if (!path.empty())
                {
                        paths.emplace_back(path);
                }
        }
        return paths;
}

