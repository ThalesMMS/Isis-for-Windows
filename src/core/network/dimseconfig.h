/*
 * ------------------------------------------------------------------------------------
 *  File: dimseconfig.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Configuration and peer management for DIMSE operations.
 *      Handles persistence of DICOM peer settings using JSON.
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include "../utils.h"

namespace isis::core::network
{
    /**
     * @brief Represents a DICOM peer (PACS server or workstation)
     */
    struct DicomPeer
    {
        std::string id;                     // Unique identifier
        std::string name;                   // Friendly name
        std::string aeTitle;                // Application Entity Title
        std::string hostname;               // IP address or hostname
        int port = 104;                     // DICOM port (default 104)
        std::vector<std::string> sopClasses; // Supported SOP Classes
        bool enabled = true;                // Peer enabled/disabled
        int timeout = 30;                   // Connection timeout in seconds
        int maxPduSize = 16384;            // Maximum PDU size

        // Optional C-MOVE destination override
        std::string moveDestinationAE;

        DicomPeer() = default;

        DicomPeer(const std::string& peerId, const std::string& peerName,
                  const std::string& ae, const std::string& host, int p)
            : id(peerId), name(peerName), aeTitle(ae), hostname(host), port(p) {}
    };

    /**
     * @brief Local DICOM application entity configuration
     */
    struct LocalAEConfig
    {
        std::string aeTitle = "ISIS_VIEWER";  // Local AE Title
        int storagePort = 11112;              // Port for incoming C-STORE (C-MOVE)
        std::string tempStoragePath;          // Temporary storage for received files
        int maxConnections = 5;               // Max simultaneous connections
        int maxPduSize = 16384;               // Maximum PDU size
        bool enableStorage = true;            // Enable Storage SCP

        LocalAEConfig() = default;
    };

    /**
     * @brief DIMSE configuration manager
     */
    class export DimseConfig
    {
    public:
        DimseConfig();
        ~DimseConfig() = default;

        // Peer management
        bool addPeer(const DicomPeer& peer);
        bool removePeer(const std::string& peerId);
        bool updatePeer(const DicomPeer& peer);
        DicomPeer* getPeer(const std::string& peerId);
        const DicomPeer* getPeer(const std::string& peerId) const;
        std::vector<DicomPeer> getAllPeers() const;
        std::vector<DicomPeer> getEnabledPeers() const;
        bool peerExists(const std::string& peerId) const;

        // Local AE configuration
        void setLocalAEConfig(const LocalAEConfig& config);
        const LocalAEConfig& getLocalAEConfig() const { return m_localConfig; }
        LocalAEConfig& getLocalAEConfig() { return m_localConfig; }

        // Persistence
        bool loadFromFile(const std::string& filepath);
        bool saveToFile(const std::string& filepath) const;
        bool loadFromDefault();
        bool saveToDefault() const;

        // Validation
        static bool validatePeer(const DicomPeer& peer, std::string& errorMsg);
        static bool validateAETitle(const std::string& aeTitle);
        static bool validateHostname(const std::string& hostname);
        static bool validatePort(int port);

        // Configuration path management
        static std::string getDefaultConfigPath();
        static std::string getDefaultTempStoragePath();

    private:
        std::map<std::string, DicomPeer> m_peers;
        LocalAEConfig m_localConfig;
        mutable std::mutex m_mutex;

        // Helper methods
        std::string generatePeerId(const std::string& aeTitle, const std::string& hostname) const;
        void initializeDefaultConfig();
    };

} // namespace isis::core::network
