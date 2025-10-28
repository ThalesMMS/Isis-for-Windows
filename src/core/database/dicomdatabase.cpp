/*
 * ------------------------------------------------------------------------------------
 *  File: dicomdatabase.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of DICOM database
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "dicomdatabase.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

namespace isis::core::database
{
    DicomDatabase::DicomDatabase()
    {
        m_database = QSqlDatabase::addDatabase("QSQLITE", "DicomDB");
    }

    DicomDatabase::~DicomDatabase()
    {
        close();
    }

    bool DicomDatabase::open(const QString& path)
    {
        m_databasePath = path;
        m_database.setDatabaseName(path);

        if (!m_database.open())
        {
            qWarning() << "Failed to open database:" << m_database.lastError().text();
            return false;
        }

        m_isOpen = true;
        return initializeSchema();
    }

    void DicomDatabase::close()
    {
        if (m_isOpen)
        {
            m_database.close();
            m_isOpen = false;
        }
    }

    bool DicomDatabase::isOpen() const
    {
        return m_isOpen && m_database.isOpen();
    }

    bool DicomDatabase::initializeSchema()
    {
        if (!m_isOpen)
        {
            return false;
        }

        return createTables();
    }

    bool DicomDatabase::createTables()
    {
        QSqlQuery query(m_database);

        // Create Patients table
        if (!tableExists("Patients"))
        {
            const QString createPatientsTable = R"(
                CREATE TABLE Patients (
                    PatientID TEXT PRIMARY KEY,
                    PatientName TEXT,
                    PatientBirthDate TEXT,
                    PatientSex TEXT,
                    CreatedDate TEXT
                )
            )";

            if (!query.exec(createPatientsTable))
            {
                qWarning() << "Failed to create Patients table:" << query.lastError().text();
                return false;
            }
        }

        // Create Studies table
        if (!tableExists("Studies"))
        {
            const QString createStudiesTable = R"(
                CREATE TABLE Studies (
                    StudyID TEXT PRIMARY KEY,
                    PatientID TEXT,
                    StudyDate TEXT,
                    StudyTime TEXT,
                    StudyDescription TEXT,
                    FOREIGN KEY (PatientID) REFERENCES Patients(PatientID)
                )
            )";

            if (!query.exec(createStudiesTable))
            {
                qWarning() << "Failed to create Studies table:" << query.lastError().text();
                return false;
            }
        }

        // Create Series table
        if (!tableExists("Series"))
        {
            const QString createSeriesTable = R"(
                CREATE TABLE Series (
                    SeriesID TEXT PRIMARY KEY,
                    StudyID TEXT,
                    Modality TEXT,
                    SeriesNumber TEXT,
                    SeriesDescription TEXT,
                    ImageCount INTEGER,
                    FOREIGN KEY (StudyID) REFERENCES Studies(StudyID)
                )
            )";

            if (!query.exec(createSeriesTable))
            {
                qWarning() << "Failed to create Series table:" << query.lastError().text();
                return false;
            }
        }

        return true;
    }

    bool DicomDatabase::tableExists(const QString& tableName) const
    {
        QSqlQuery query(m_database);
        query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=?");
        query.addBindValue(tableName);

        if (query.exec() && query.next())
        {
            return true;
        }

        return false;
    }

    bool DicomDatabase::insertPatient(const Patient* patient)
    {
        // TODO: Implement patient insertion
        // Requires access to Patient class members
        return false;
    }

    bool DicomDatabase::insertStudy(const Study* study, const QString& patientId)
    {
        // TODO: Implement study insertion
        // Requires access to Study class members
        return false;
    }

    bool DicomDatabase::insertSeries(const Series* series, const QString& studyId)
    {
        // TODO: Implement series insertion
        // Requires access to Series class members
        return false;
    }

    QStringList DicomDatabase::queryPatients() const
    {
        QStringList patients;

        if (!m_isOpen)
        {
            return patients;
        }

        QSqlQuery query(m_database);
        if (query.exec("SELECT PatientID FROM Patients"))
        {
            while (query.next())
            {
                patients << query.value(0).toString();
            }
        }

        return patients;
    }

    QStringList DicomDatabase::queryStudies(const QString& patientId) const
    {
        QStringList studies;

        if (!m_isOpen)
        {
            return studies;
        }

        QSqlQuery query(m_database);
        query.prepare("SELECT StudyID FROM Studies WHERE PatientID = ?");
        query.addBindValue(patientId);

        if (query.exec())
        {
            while (query.next())
            {
                studies << query.value(0).toString();
            }
        }

        return studies;
    }

    bool DicomDatabase::clearAll()
    {
        if (!m_isOpen)
        {
            return false;
        }

        QSqlQuery query(m_database);

        query.exec("DELETE FROM Series");
        query.exec("DELETE FROM Studies");
        query.exec("DELETE FROM Patients");

        return true;
    }

} // namespace isis::core::database
