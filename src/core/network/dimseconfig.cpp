/*
 * ------------------------------------------------------------------------------------
 *  File: dimseconfig.cpp
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Implementation of DIMSE configuration and peer management.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#include "dimseconfig.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcDimse)
Q_LOGGING_CATEGORY(lcDimse, "isis.core.dimse")

namespace isis::core::network
{
    DimseConfig::DimseConfig()
    {
        initializeDefaultConfig();
    }

    bool DimseConfig::addPeer(const DicomPeer& peer)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::string errorMsg;
        if (!validatePeer(peer, errorMsg))
        {
            qCWarning(lcDimse) << "Invalid peer configuration:" << errorMsg.c_str();
            return false;
        }

        if (m_peers.find(peer.id) != m_peers.end())
        {
            qCWarning(lcDimse) << "Peer with ID" << peer.id.c_str() << "already exists";
            return false;
        }

        m_peers[peer.id] = peer;
        qCInfo(lcDimse) << "Added peer:" << peer.name.c_str()
                        << "(" << peer.aeTitle.c_str() << ")";
        return true;
    }

    bool DimseConfig::removePeer(const std::string& peerId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_peers.find(peerId);
        if (it == m_peers.end())
        {
            qCWarning(lcDimse) << "Peer not found:" << peerId.c_str();
            return false;
        }

        qCInfo(lcDimse) << "Removed peer:" << it->second.name.c_str();
        m_peers.erase(it);
        return true;
    }

    bool DimseConfig::updatePeer(const DicomPeer& peer)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::string errorMsg;
        if (!validatePeer(peer, errorMsg))
        {
            qCWarning(lcDimse) << "Invalid peer configuration:" << errorMsg.c_str();
            return false;
        }

        auto it = m_peers.find(peer.id);
        if (it == m_peers.end())
        {
            qCWarning(lcDimse) << "Peer not found for update:" << peer.id.c_str();
            return false;
        }

        m_peers[peer.id] = peer;
        qCInfo(lcDimse) << "Updated peer:" << peer.name.c_str();
        return true;
    }

    DicomPeer* DimseConfig::getPeer(const std::string& peerId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_peers.find(peerId);
        return it != m_peers.end() ? &it->second : nullptr;
    }

    const DicomPeer* DimseConfig::getPeer(const std::string& peerId) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_peers.find(peerId);
        return it != m_peers.end() ? &it->second : nullptr;
    }

    std::vector<DicomPeer> DimseConfig::getAllPeers() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<DicomPeer> peers;
        peers.reserve(m_peers.size());
        for (const auto& pair : m_peers)
        {
            peers.push_back(pair.second);
        }
        return peers;
    }

    std::vector<DicomPeer> DimseConfig::getEnabledPeers() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<DicomPeer> peers;
        for (const auto& pair : m_peers)
        {
            if (pair.second.enabled)
            {
                peers.push_back(pair.second);
            }
        }
        return peers;
    }

    bool DimseConfig::peerExists(const std::string& peerId) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_peers.find(peerId) != m_peers.end();
    }

    void DimseConfig::setLocalAEConfig(const LocalAEConfig& config)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_localConfig = config;
        qCInfo(lcDimse) << "Local AE configured:" << config.aeTitle.c_str()
                        << "port:" << config.storagePort;
    }

    bool DimseConfig::loadFromFile(const std::string& filepath)
    {
        QFile file(QString::fromStdString(filepath));
        if (!file.open(QIODevice::ReadOnly))
        {
            qCWarning(lcDimse) << "Failed to open config file:" << filepath.c_str();
            return false;
        }

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        if (!doc.isObject())
        {
            qCWarning(lcDimse) << "Invalid JSON format in config file";
            return false;
        }

        QJsonObject root = doc.object();

        // Load local configuration
        if (root.contains("localAE"))
        {
            QJsonObject localAE = root["localAE"].toObject();
            LocalAEConfig config;
            config.aeTitle = localAE["aeTitle"].toString("ISIS_VIEWER").toStdString();
            config.storagePort = localAE["storagePort"].toInt(11112);
            config.tempStoragePath = localAE["tempStoragePath"].toString().toStdString();
            config.maxConnections = localAE["maxConnections"].toInt(5);
            config.maxPduSize = localAE["maxPduSize"].toInt(16384);
            config.enableStorage = localAE["enableStorage"].toBool(true);
            setLocalAEConfig(config);
        }

        // Load peers
        if (root.contains("peers") && root["peers"].isArray())
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_peers.clear();

            QJsonArray peersArray = root["peers"].toArray();
            for (const QJsonValue& val : peersArray)
            {
                QJsonObject peerObj = val.toObject();
                DicomPeer peer;
                peer.id = peerObj["id"].toString().toStdString();
                peer.name = peerObj["name"].toString().toStdString();
                peer.aeTitle = peerObj["aeTitle"].toString().toStdString();
                peer.hostname = peerObj["hostname"].toString().toStdString();
                peer.port = peerObj["port"].toInt(104);
                peer.enabled = peerObj["enabled"].toBool(true);
                peer.timeout = peerObj["timeout"].toInt(30);
                peer.maxPduSize = peerObj["maxPduSize"].toInt(16384);
                peer.moveDestinationAE = peerObj["moveDestinationAE"].toString().toStdString();

                if (peerObj.contains("sopClasses") && peerObj["sopClasses"].isArray())
                {
                    QJsonArray sopArray = peerObj["sopClasses"].toArray();
                    for (const QJsonValue& sopVal : sopArray)
                    {
                        peer.sopClasses.push_back(sopVal.toString().toStdString());
                    }
                }

                std::string errorMsg;
                if (validatePeer(peer, errorMsg))
                {
                    m_peers[peer.id] = peer;
                }
                else
                {
                    qCWarning(lcDimse) << "Skipping invalid peer:" << peer.id.c_str()
                                       << "-" << errorMsg.c_str();
                }
            }
        }

        qCInfo(lcDimse) << "Loaded configuration from:" << filepath.c_str()
                        << "- Peers:" << m_peers.size();
        return true;
    }

    bool DimseConfig::saveToFile(const std::string& filepath) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        QJsonObject root;

        // Save local configuration
        QJsonObject localAE;
        localAE["aeTitle"] = QString::fromStdString(m_localConfig.aeTitle);
        localAE["storagePort"] = m_localConfig.storagePort;
        localAE["tempStoragePath"] = QString::fromStdString(m_localConfig.tempStoragePath);
        localAE["maxConnections"] = m_localConfig.maxConnections;
        localAE["maxPduSize"] = m_localConfig.maxPduSize;
        localAE["enableStorage"] = m_localConfig.enableStorage;
        root["localAE"] = localAE;

        // Save peers
        QJsonArray peersArray;
        for (const auto& pair : m_peers)
        {
            const DicomPeer& peer = pair.second;
            QJsonObject peerObj;
            peerObj["id"] = QString::fromStdString(peer.id);
            peerObj["name"] = QString::fromStdString(peer.name);
            peerObj["aeTitle"] = QString::fromStdString(peer.aeTitle);
            peerObj["hostname"] = QString::fromStdString(peer.hostname);
            peerObj["port"] = peer.port;
            peerObj["enabled"] = peer.enabled;
            peerObj["timeout"] = peer.timeout;
            peerObj["maxPduSize"] = peer.maxPduSize;
            peerObj["moveDestinationAE"] = QString::fromStdString(peer.moveDestinationAE);

            QJsonArray sopArray;
            for (const std::string& sop : peer.sopClasses)
            {
                sopArray.append(QString::fromStdString(sop));
            }
            peerObj["sopClasses"] = sopArray;

            peersArray.append(peerObj);
        }
        root["peers"] = peersArray;

        QJsonDocument doc(root);
        QFile file(QString::fromStdString(filepath));
        if (!file.open(QIODevice::WriteOnly))
        {
            qCWarning(lcDimse) << "Failed to open config file for writing:" << filepath.c_str();
            return false;
        }

        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        qCInfo(lcDimse) << "Saved configuration to:" << filepath.c_str();
        return true;
    }

    bool DimseConfig::loadFromDefault()
    {
        std::string defaultPath = getDefaultConfigPath();
        if (!QFile::exists(QString::fromStdString(defaultPath)))
        {
            qCInfo(lcDimse) << "No default config found, using defaults";
            initializeDefaultConfig();
            return true;
        }
        return loadFromFile(defaultPath);
    }

    bool DimseConfig::saveToDefault() const
    {
        return saveToFile(getDefaultConfigPath());
    }

    bool DimseConfig::validatePeer(const DicomPeer& peer, std::string& errorMsg)
    {
        if (peer.id.empty())
        {
            errorMsg = "Peer ID cannot be empty";
            return false;
        }

        if (peer.name.empty())
        {
            errorMsg = "Peer name cannot be empty";
            return false;
        }

        if (!validateAETitle(peer.aeTitle))
        {
            errorMsg = "Invalid AE Title: " + peer.aeTitle;
            return false;
        }

        if (!validateHostname(peer.hostname))
        {
            errorMsg = "Invalid hostname: " + peer.hostname;
            return false;
        }

        if (!validatePort(peer.port))
        {
            errorMsg = "Invalid port: " + std::to_string(peer.port);
            return false;
        }

        return true;
    }

    bool DimseConfig::validateAETitle(const std::string& aeTitle)
    {
        // AE Title must be 1-16 characters, alphanumeric, underscore, hyphen
        if (aeTitle.empty() || aeTitle.length() > 16)
            return false;

        std::regex aeRegex("^[A-Za-z0-9_-]+$");
        return std::regex_match(aeTitle, aeRegex);
    }

    bool DimseConfig::validateHostname(const std::string& hostname)
    {
        if (hostname.empty())
            return false;

        // Basic hostname/IP validation
        std::regex ipRegex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
        std::regex hostnameRegex("^([a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?\\.)*[a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?$");

        return std::regex_match(hostname, ipRegex) || std::regex_match(hostname, hostnameRegex);
    }

    bool DimseConfig::validatePort(int port)
    {
        return port > 0 && port <= 65535;
    }

    std::string DimseConfig::getDefaultConfigPath()
    {
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(appDataPath);
        if (!dir.exists())
        {
            dir.mkpath(".");
        }
        return dir.filePath("dimse_config.json").toStdString();
    }

    std::string DimseConfig::getDefaultTempStoragePath()
    {
        QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QDir dir(tempPath);
        QString dimsePath = dir.filePath("IsisViewer_DIMSE");
        QDir dimseDir(dimsePath);
        if (!dimseDir.exists())
        {
            dimseDir.mkpath(".");
        }
        return dimsePath.toStdString();
    }

    std::string DimseConfig::generatePeerId(const std::string& aeTitle, const std::string& hostname) const
    {
        return aeTitle + "@" + hostname;
    }

    void DimseConfig::initializeDefaultConfig()
    {
        // Set default local configuration
        m_localConfig.aeTitle = "ISIS_VIEWER";
        m_localConfig.storagePort = 11112;
        m_localConfig.tempStoragePath = getDefaultTempStoragePath();
        m_localConfig.maxConnections = 5;
        m_localConfig.maxPduSize = 16384;
        m_localConfig.enableStorage = true;

        // Create temp storage directory if it doesn't exist
        QDir dir(QString::fromStdString(m_localConfig.tempStoragePath));
        if (!dir.exists())
        {
            dir.mkpath(".");
        }

        qCInfo(lcDimse) << "Initialized default DIMSE configuration";
    }

} // namespace isis::core::network
