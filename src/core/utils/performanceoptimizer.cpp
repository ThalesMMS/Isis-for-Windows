/*
 * ------------------------------------------------------------------------------------
 *  File: performanceoptimizer.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of performance optimization utilities
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "performanceoptimizer.h"
#include <algorithm>
#include <iostream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace isis::core::utils
{
    // ImageCache implementation

    ImageCache::ImageCache(size_t maxSizeBytes)
        : m_maxSizeBytes(maxSizeBytes)
    {}

    bool ImageCache::put(const std::string& key, vtkImageData* image)
    {
        if (!image)
        {
            return false;
        }

        std::lock_guard<std::mutex> lock(m_mutex);

        size_t imageSize = calculateImageSize(image);

        // Evict if necessary
        while (m_currentSizeBytes + imageSize > m_maxSizeBytes && !m_cache.empty())
        {
            evictLRU();
        }

        // Check if still doesn't fit
        if (imageSize > m_maxSizeBytes)
        {
            return false;
        }

        // Remove existing entry if present
        if (contains(key))
        {
            remove(key);
        }

        // Create new entry
        CacheEntry entry;
        entry.data = vtkSmartPointer<vtkImageData>::New();
        entry.data->DeepCopy(image);
        entry.sizeBytes = imageSize;
        entry.lastAccessed = std::chrono::system_clock::now();
        entry.accessCount = 1;

        m_cache[key] = entry;
        m_currentSizeBytes += imageSize;

        return true;
    }

    vtkSmartPointer<vtkImageData> ImageCache::get(const std::string& key)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_cache.find(key);
        if (it != m_cache.end())
        {
            // Update access time and count
            it->second.lastAccessed = std::chrono::system_clock::now();
            it->second.accessCount++;

            m_hits++;
            return it->second.data;
        }

        m_misses++;
        return nullptr;
    }

    bool ImageCache::contains(const std::string& key) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_cache.find(key) != m_cache.end();
    }

    void ImageCache::remove(const std::string& key)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_cache.find(key);
        if (it != m_cache.end())
        {
            m_currentSizeBytes -= it->second.sizeBytes;
            m_cache.erase(it);
        }
    }

    void ImageCache::clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache.clear();
        m_currentSizeBytes = 0;
    }

    size_t ImageCache::getCurrentSize() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_currentSizeBytes;
    }

    size_t ImageCache::getItemCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_cache.size();
    }

    void ImageCache::setMaxSize(size_t maxSizeBytes)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_maxSizeBytes = maxSizeBytes;

        // Evict if necessary
        while (m_currentSizeBytes > m_maxSizeBytes && !m_cache.empty())
        {
            evictLRU();
        }
    }

    ImageCache::Statistics ImageCache::getStatistics() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        Statistics stats;
        stats.hits = m_hits;
        stats.misses = m_misses;
        stats.evictions = m_evictions;

        size_t total = m_hits + m_misses;
        stats.hitRate = total > 0 ? static_cast<double>(m_hits) / total : 0.0;

        return stats;
    }

    void ImageCache::resetStatistics()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_hits = 0;
        m_misses = 0;
        m_evictions = 0;
    }

    void ImageCache::evictLRU()
    {
        if (m_cache.empty())
        {
            return;
        }

        // Find LRU entry
        auto lru = m_cache.begin();
        for (auto it = m_cache.begin(); it != m_cache.end(); ++it)
        {
            if (it->second.lastAccessed < lru->second.lastAccessed)
            {
                lru = it;
            }
        }

        m_currentSizeBytes -= lru->second.sizeBytes;
        m_cache.erase(lru);
        m_evictions++;
    }

    size_t ImageCache::calculateImageSize(vtkImageData* image) const
    {
        if (!image)
        {
            return 0;
        }

        int* dims = image->GetDimensions();
        int numComponents = image->GetNumberOfScalarComponents();
        int scalarSize = image->GetScalarSize();

        return static_cast<size_t>(dims[0]) * dims[1] * dims[2] * numComponents * scalarSize;
    }

    // PerformanceProfiler implementation

    void PerformanceProfiler::startSection(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        TimingData& timing = m_activeTimings[name];
        timing.startTime = std::chrono::high_resolution_clock::now();
        timing.active = true;
    }

    void PerformanceProfiler::endSection(const std::string& name)
    {
        auto endTime = std::chrono::high_resolution_clock::now();

        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_activeTimings.find(name);
        if (it == m_activeTimings.end() || !it->second.active)
        {
            return;
        }

        std::chrono::duration<double, std::milli> duration = endTime - it->second.startTime;
        double ms = duration.count();

        ProfileEntry& entry = m_profiles[name];
        entry.name = name;
        entry.totalTime += ms;
        entry.callCount++;
        entry.minTime = std::min(entry.minTime, ms);
        entry.maxTime = std::max(entry.maxTime, ms);
        entry.avgTime = entry.totalTime / entry.callCount;

        it->second.active = false;
    }

    std::map<std::string, PerformanceProfiler::ProfileEntry> PerformanceProfiler::getResults() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_profiles;
    }

    void PerformanceProfiler::printResults() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::cout << "\n========== Performance Profile ==========\n" << std::endl;
        std::cout << std::setw(30) << std::left << "Section"
                 << std::setw(10) << "Calls"
                 << std::setw(12) << "Total (ms)"
                 << std::setw(12) << "Avg (ms)"
                 << std::setw(12) << "Min (ms)"
                 << std::setw(12) << "Max (ms)" << std::endl;
        std::cout << std::string(88, '-') << std::endl;

        for (const auto& [name, entry] : m_profiles)
        {
            std::cout << std::setw(30) << std::left << entry.name
                     << std::setw(10) << entry.callCount
                     << std::setw(12) << std::fixed << std::setprecision(2) << entry.totalTime
                     << std::setw(12) << entry.avgTime
                     << std::setw(12) << entry.minTime
                     << std::setw(12) << entry.maxTime << std::endl;
        }

        std::cout << "========================================\n" << std::endl;
    }

    void PerformanceProfiler::reset()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_profiles.clear();
        m_activeTimings.clear();
    }

    // MemoryTracker implementation

    size_t MemoryTracker::getCurrentMemoryUsage()
    {
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
        {
            return pmc.WorkingSetSize;
        }
#else
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0)
        {
            return usage.ru_maxrss * 1024;  // Convert to bytes
        }
#endif
        return 0;
    }

    size_t MemoryTracker::getPeakMemoryUsage()
    {
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
        {
            return pmc.PeakWorkingSetSize;
        }
#else
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0)
        {
            return usage.ru_maxrss * 1024;
        }
#endif
        return 0;
    }

    size_t MemoryTracker::getAvailableMemory()
    {
#ifdef _WIN32
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        if (GlobalMemoryStatusEx(&status))
        {
            return status.ullAvailPhys;
        }
#else
        long pages = sysconf(_SC_AVPHYS_PAGES);
        long page_size = sysconf(_SC_PAGE_SIZE);
        if (pages > 0 && page_size > 0)
        {
            return pages * page_size;
        }
#endif
        return 0;
    }

    void MemoryTracker::printMemoryStatistics()
    {
        size_t current = getCurrentMemoryUsage();
        size_t peak = getPeakMemoryUsage();
        size_t available = getAvailableMemory();

        std::cout << "\n========== Memory Statistics ==========\n" << std::endl;
        std::cout << "Current Usage:   " << std::setw(10) << (current / (1024 * 1024)) << " MB" << std::endl;
        std::cout << "Peak Usage:      " << std::setw(10) << (peak / (1024 * 1024)) << " MB" << std::endl;
        std::cout << "Available:       " << std::setw(10) << (available / (1024 * 1024)) << " MB" << std::endl;
        std::cout << "=======================================\n" << std::endl;
    }

} // namespace isis::core::utils
