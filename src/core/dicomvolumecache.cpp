/*
 * ------------------------------------------------------------------------------------
 *  File: dicomvolumecache.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Description:
 *      Implements the DICOM volume cache responsible for retaining assembled volumes
 *      across study and series loads.
 * ------------------------------------------------------------------------------------
 */

#include "dicomvolumecache.h"

#include <atomic>
#include <filesystem>
#include <list>
#include <mutex>
#include <system_error>
#include <unordered_map>

#include <QLoggingCategory>
#include <QString>

#include <vtkImageData.h>
#include <vtkPointData.h>

Q_DECLARE_LOGGING_CATEGORY(lcDicomVolumeLoader)

namespace
{
        constexpr std::size_t kDefaultCacheCapacityBytes = 512ULL * 1024ULL * 1024ULL;

        std::atomic<std::uint64_t> g_volumeCacheHits = 0;
        std::atomic<std::uint64_t> g_volumeCacheMisses = 0;

        void logVolumeCacheTelemetry(const char* event,
                const std::string& studyUid,
                const std::string& seriesUid)
        {
                const auto hits = g_volumeCacheHits.load(std::memory_order_relaxed);
                const auto misses = g_volumeCacheMisses.load(std::memory_order_relaxed);
                const auto total = hits + misses;
                const double hitRate = (total > 0)
                        ? (static_cast<double>(hits) * 100.0) / static_cast<double>(total)
                        : 0.0;
                const QString qStudyUid = studyUid.empty()
                        ? QStringLiteral("n/a")
                        : QString::fromStdString(studyUid);
                const QString qSeriesUid = seriesUid.empty()
                        ? QStringLiteral("n/a")
                        : QString::fromStdString(seriesUid);
                qCInfo(lcDicomVolumeLoader)
                        << "[Telemetry] Volume cache" << event
                        << "studyUid" << qStudyUid
                        << "seriesUid" << qSeriesUid
                        << "hits" << static_cast<unsigned long long>(hits)
                        << "misses" << static_cast<unsigned long long>(misses)
                        << "hitRatePct" << hitRate;
        }

        class DicomVolumeCacheImpl
        {
        public:
                using VolumePtr = isis::core::DicomVolumeCache::VolumePtr;

                VolumePtr get(const std::string& studyUid,
                        const std::string& seriesUid,
                        const std::vector<std::string>& paths,
                        const std::function<VolumePtr()>& loader)
                {
                        if (seriesUid.empty())
                        {
                                return loader();
                        }

                        std::unique_lock<std::mutex> lock(m_mutex);
                        onStudyAccessLocked(studyUid);
                        const std::string key = composeKey(studyUid, seriesUid);
                        auto mapIt = m_entries.find(key);
                        if (mapIt != m_entries.end())
                        {
                                if (isValidLocked(mapIt->second, paths))
                                {
                                        touchLocked(mapIt->second);
                                        g_volumeCacheHits.fetch_add(1, std::memory_order_relaxed);
                                        logVolumeCacheTelemetry("hit", studyUid, seriesUid);
                                        return mapIt->second.Volume;
                                }

                                removeEntryLocked(mapIt);
                        }

                        lock.unlock();

                        auto volume = loader();
                        if (!volume)
                        {
                                return nullptr;
                        }

                        auto timestamps = collectTimestamps(paths);
                        if (timestamps.size() != paths.size())
                        {
                                g_volumeCacheMisses.fetch_add(1, std::memory_order_relaxed);
                                logVolumeCacheTelemetry("miss", studyUid, seriesUid);
                                return volume;
                        }

                        CacheEntry entry;
                        entry.Volume = volume;
                        entry.Paths = paths;
                        entry.TimeStamps = std::move(timestamps);
                        entry.MemoryBytes = calculateMemoryUsage(volume);
                        entry.StudyUid = studyUid;
                        entry.SeriesUid = seriesUid;

                        lock.lock();
                        onStudyAccessLocked(studyUid);

                        mapIt = m_entries.find(key);
                        if (mapIt != m_entries.end())
                        {
                                removeEntryLocked(mapIt);
                        }

                        entry.LruIt = m_lru.emplace(m_lru.begin(), key);
                        m_memoryBytes += entry.MemoryBytes;
                        m_entries.emplace(key, std::move(entry));
                        g_volumeCacheMisses.fetch_add(1, std::memory_order_relaxed);
                        logVolumeCacheTelemetry("miss", studyUid, seriesUid);
                        evictIfNeededLocked();
                        return volume;
                }

                void invalidateSeries(const std::string& studyUid, const std::string& seriesUid)
                {
                        if (seriesUid.empty())
                        {
                                return;
                        }

                        std::lock_guard<std::mutex> lock(m_mutex);
                        const std::string key = composeKey(studyUid, seriesUid);
                        auto it = m_entries.find(key);
                        if (it != m_entries.end())
                        {
                                removeEntryLocked(it);
                                logVolumeCacheTelemetry("invalidate", studyUid, seriesUid);
                        }
                }

                void invalidateStudy(const std::string& studyUid)
                {
                        if (studyUid.empty())
                        {
                                return;
                        }

                        std::lock_guard<std::mutex> lock(m_mutex);
                        invalidateStudyLocked(studyUid);
                }

        private:
                struct CacheEntry
                {
                        VolumePtr Volume = {};
                        std::vector<std::string> Paths = {};
                        std::vector<std::filesystem::file_time_type> TimeStamps = {};
                        std::size_t MemoryBytes = 0;
                        std::list<std::string>::iterator LruIt = {};
                        std::string StudyUid = {};
                        std::string SeriesUid = {};
                };

                using EntryMap = std::unordered_map<std::string, CacheEntry>;

                static std::string composeKey(const std::string& studyUid, const std::string& seriesUid)
                {
                        return studyUid + '|' + seriesUid;
                }

                static std::vector<std::filesystem::file_time_type> collectTimestamps(const std::vector<std::string>& paths)
                {
                        std::vector<std::filesystem::file_time_type> timestamps;
                        timestamps.reserve(paths.size());
                        for (const auto& path : paths)
                        {
                                std::error_code errorCode;
                                const auto timestamp = std::filesystem::last_write_time(path, errorCode);
                                if (errorCode)
                                {
                                        qCWarning(lcDicomVolumeLoader)
                                                << "Failed to query last write time for"
                                                << QString::fromStdString(path)
                                                << ':'
                                                << QString::fromStdString(errorCode.message())
                                                << ". Volume will not be cached.";
                                        return {};
                                }
                                timestamps.emplace_back(timestamp);
                        }
                        return timestamps;
                }

                static std::size_t calculateMemoryUsage(const VolumePtr& volume)
                {
                        if (!volume || !volume->ImageData)
                        {
                                return 0;
                        }

                        const vtkIdType kiloBytes = volume->ImageData->GetActualMemorySize();
                        if (kiloBytes <= 0)
                        {
                                return 0;
                        }

                        return static_cast<std::size_t>(kiloBytes) * 1024ULL;
                }

                bool isValidLocked(const CacheEntry& entry, const std::vector<std::string>& paths)
                {
                        if (entry.Paths.size() != paths.size())
                        {
                                return false;
                        }

                        for (std::size_t index = 0; index < paths.size(); ++index)
                        {
                                if (entry.Paths[index] != paths[index])
                                {
                                        return false;
                                }

                                std::error_code errorCode;
                                const auto timestamp = std::filesystem::last_write_time(paths[index], errorCode);
                                if (errorCode || timestamp != entry.TimeStamps[index])
                                {
                                        return false;
                                }
                        }

                        return true;
                }

                void touchLocked(CacheEntry& entry)
                {
                        m_lru.splice(m_lru.begin(), m_lru, entry.LruIt);
                        entry.LruIt = m_lru.begin();
                }

                EntryMap::iterator removeEntryLocked(EntryMap::iterator it)
                {
                        if (it == m_entries.end())
                        {
                                return it;
                        }

                        if (m_memoryBytes >= it->second.MemoryBytes)
                        {
                                m_memoryBytes -= it->second.MemoryBytes;
                        }
                        else
                        {
                                m_memoryBytes = 0;
                        }

                        m_lru.erase(it->second.LruIt);
                        return m_entries.erase(it);
                }

                void evictIfNeededLocked()
                {
                        while (!m_lru.empty() && m_memoryBytes > m_capacityBytes)
                        {
                                auto backIt = std::prev(m_lru.end());
                                const std::string& key = *backIt;
                                auto mapIt = m_entries.find(key);
                                if (mapIt == m_entries.end())
                                {
                                        m_lru.erase(backIt);
                                        continue;
                                }

                                logVolumeCacheTelemetry("evict", mapIt->second.StudyUid, mapIt->second.SeriesUid);
                                removeEntryLocked(mapIt);
                        }
                }

                void invalidateStudyLocked(const std::string& studyUid)
                {
                        for (auto it = m_entries.begin(); it != m_entries.end();)
                        {
                                if (it->second.StudyUid == studyUid)
                                {
                                        logVolumeCacheTelemetry("purge", it->second.StudyUid, it->second.SeriesUid);
                                        it = removeEntryLocked(it);
                                }
                                else
                                {
                                        ++it;
                                }
                        }

                        if (!m_activeStudyUid.empty() && m_activeStudyUid == studyUid)
                        {
                                m_activeStudyUid.clear();
                        }
                }

                void onStudyAccessLocked(const std::string& studyUid)
                {
                        if (studyUid.empty())
                        {
                                return;
                        }

                        if (!m_activeStudyUid.empty() && m_activeStudyUid != studyUid)
                        {
                                invalidateStudyLocked(m_activeStudyUid);
                        }

                        m_activeStudyUid = studyUid;
                }

                std::mutex m_mutex = {};
                EntryMap m_entries = {};
                std::list<std::string> m_lru = {};
                std::size_t m_memoryBytes = 0;
                std::size_t m_capacityBytes = kDefaultCacheCapacityBytes;
                std::string m_activeStudyUid = {};
        };

        DicomVolumeCacheImpl& cacheImpl()
        {
                static DicomVolumeCacheImpl cache;
                return cache;
        }
}

namespace isis::core
{
        DicomVolumeCache::VolumePtr DicomVolumeCache::get(const std::string& studyUid,
                const std::string& seriesUid,
                const std::vector<std::string>& paths,
                const std::function<VolumePtr()>& loader)
        {
                return cacheImpl().get(studyUid, seriesUid, paths, loader);
        }

        void DicomVolumeCache::invalidateSeries(const std::string& studyUid,
                const std::string& seriesUid)
        {
                cacheImpl().invalidateSeries(studyUid, seriesUid);
        }

        void DicomVolumeCache::invalidateStudy(const std::string& studyUid)
        {
                cacheImpl().invalidateStudy(studyUid);
        }

        DicomVolumeCache& volumeCache()
        {
                static DicomVolumeCache cache;
                return cache;
        }
}
