/*
 * ------------------------------------------------------------------------------------
 *  File: dicomdatabase.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Local SQLite database for persistent DICOM data storage
 *      Provides indexing, caching, and fast retrieval of DICOM metadata
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <QString>
#include <QStringList>
#include <QSqlDatabase>
#include <memory>

namespace isis::core
{
    class Patient;
    class Study;
    class Series;
    class Image;
}

namespace isis::core::database
{
    /**
     * @brief Local database for DICOM data persistence
     *
     * Provides SQLite-based storage for DICOM metadata, thumbnails,
     * and cached data for improved performance.
     */
    class DicomDatabase
    {
    public:
        DicomDatabase();
        ~DicomDatabase();

        /**
         * @brief Open or create database at specified path
         * @param path Database file path
         * @return true if successful, false otherwise
         */
        bool open(const QString& path);

        /**
         * @brief Close the database
         */
        void close();

        /**
         * @brief Check if database is open
         * @return true if open, false otherwise
         */
        [[nodiscard]] bool isOpen() const;

        /**
         * @brief Initialize database schema
         * @return true if successful, false otherwise
         */
        bool initializeSchema();

        /**
         * @brief Insert patient into database
         * @param patient Patient to insert
         * @return true if successful, false otherwise
         */
        bool insertPatient(const Patient* patient);

        /**
         * @brief Insert study into database
         * @param study Study to insert
         * @param patientId Patient ID
         * @return true if successful, false otherwise
         */
        bool insertStudy(const Study* study, const QString& patientId);

        /**
         * @brief Insert series into database
         * @param series Series to insert
         * @param studyId Study ID
         * @return true if successful, false otherwise
         */
        bool insertSeries(const Series* series, const QString& studyId);

        /**
         * @brief Query patients from database
         * @return List of patient IDs
         */
        [[nodiscard]] QStringList queryPatients() const;

        /**
         * @brief Query studies for a patient
         * @param patientId Patient ID
         * @return List of study IDs
         */
        [[nodiscard]] QStringList queryStudies(const QString& patientId) const;

        /**
         * @brief Clear all data from database
         * @return true if successful, false otherwise
         */
        bool clearAll();

    private:
        bool createTables();
        bool tableExists(const QString& tableName) const;

        QSqlDatabase m_database;
        QString m_databasePath;
        bool m_isOpen = false;
    };

} // namespace isis::core::database
