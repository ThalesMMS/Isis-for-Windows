/*
 * ------------------------------------------------------------------------------------
 *  File: performanceoptimizer.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Performance optimization utilities and cache management
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <chrono>

namespace isis::core::utils
{
    /**
     * @brief Cache entry with metadata
     */
    struct CacheEntry
    {
        vtkSmartPointer<vtkImageData> data;
        size_t sizeBytes;
        std::chrono::system_clock::time_point lastAccessed;
        int accessCount = 0;

        CacheEntry() : sizeBytes(0), lastAccessed(std::chrono::system_clock::now()) {}
    };

    /**
     * @brief Simple image cache with LRU eviction
     */
    class ImageCache
    {
    public:
        explicit ImageCache(size_t maxSizeBytes = 1024 * 1024 * 1024);  // 1GB default
        ~ImageCache() = default;

        /**
         * @brief Add image to cache
         * @param key Cache key
         * @param image Image to cache
         * @return true if added successfully
         */
        bool put(const std::string& key, vtkImageData* image);

        /**
         * @brief Get image from cache
         * @param key Cache key
         * @return Cached image or nullptr if not found
         */
        vtkSmartPointer<vtkImageData> get(const std::string& key);

        /**
         * @brief Check if key exists in cache
         */
        bool contains(const std::string& key) const;

        /**
         * @brief Remove entry from cache
         */
        void remove(const std::string& key);

        /**
         * @brief Clear entire cache
         */
        void clear();

        /**
         * @brief Get current cache size in bytes
         */
        [[nodiscard]] size_t getCurrentSize() const;

        /**
         * @brief Get number of cached items
         */
        [[nodiscard]] size_t getItemCount() const;

        /**
         * @brief Set maximum cache size
         */
        void setMaxSize(size_t maxSizeBytes);

        /**
         * @brief Get cache statistics
         */
        struct Statistics
        {
            size_t hits = 0;
            size_t misses = 0;
            size_t evictions = 0;
            double hitRate = 0.0;
        };

        [[nodiscard]] Statistics getStatistics() const;

        /**
         * @brief Reset statistics
         */
        void resetStatistics();

    private:
        void evictLRU();
        size_t calculateImageSize(vtkImageData* image) const;

        std::map<std::string, CacheEntry> m_cache;
        size_t m_maxSizeBytes;
        size_t m_currentSizeBytes = 0;
        mutable std::mutex m_mutex;

        // Statistics
        mutable size_t m_hits = 0;
        mutable size_t m_misses = 0;
        size_t m_evictions = 0;
    };

    /**
     * @brief Performance profiler for measuring execution time
     */
    class PerformanceProfiler
    {
    public:
        struct ProfileEntry
        {
            std::string name;
            double totalTime = 0.0;  // milliseconds
            size_t callCount = 0;
            double minTime = std::numeric_limits<double>::max();
            double maxTime = 0.0;
            double avgTime = 0.0;
        };

        /**
         * @brief Start profiling a section
         */
        void startSection(const std::string& name);

        /**
         * @brief End profiling a section
         */
        void endSection(const std::string& name);

        /**
         * @brief Get profile results
         */
        [[nodiscard]] std::map<std::string, ProfileEntry> getResults() const;

        /**
         * @brief Print profile results
         */
        void printResults() const;

        /**
         * @brief Reset all profiling data
         */
        void reset();

        /**
         * @brief RAII helper for automatic section timing
         */
        class SectionTimer
        {
        public:
            SectionTimer(PerformanceProfiler& profiler, const std::string& name)
                : m_profiler(profiler), m_name(name)
            {
                m_profiler.startSection(m_name);
            }

            ~SectionTimer()
            {
                m_profiler.endSection(m_name);
            }

        private:
            PerformanceProfiler& m_profiler;
            std::string m_name;
        };

    private:
        struct TimingData
        {
            std::chrono::high_resolution_clock::time_point startTime;
            bool active = false;
        };

        std::map<std::string, ProfileEntry> m_profiles;
        std::map<std::string, TimingData> m_activeTimings;
        mutable std::mutex m_mutex;
    };

    /**
     * @brief Memory usage tracker
     */
    class MemoryTracker
    {
    public:
        /**
         * @brief Get current process memory usage in bytes
         */
        static size_t getCurrentMemoryUsage();

        /**
         * @brief Get peak memory usage in bytes
         */
        static size_t getPeakMemoryUsage();

        /**
         * @brief Get available system memory in bytes
         */
        static size_t getAvailableMemory();

        /**
         * @brief Print memory statistics
         */
        static void printMemoryStatistics();
    };

} // namespace isis::core::utils
