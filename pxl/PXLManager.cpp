#include "PXLManager.hpp"
#include <plist/plist++.h>
#include <iostream>
#include <fstream>
#include <unordered_map>

PXLManager::PXLManager(AppleFileConduitSession& afcSession) : m_afcSession(afcSession), m_verboseLoggingEnabled(false) {}

std::optional<std::vector<PXLManager::PXLApplication>> PXLManager::GetInstalledApplications(void) const {
    if (!IsDaemonInstalled()) {
        std::cerr << "[+] PXLdaemon is not installed! Install it in order to use this tool\n";
        return std::nullopt;
    }

    auto entries = m_afcSession.ContentsOfDirectory(PXLDatabasePath);
    if (!entries.has_value()) {
        return std::nullopt;
    }

    std::vector<PXLApplication> applications;
    applications.reserve(entries->size());

    static constexpr std::string_view PlistExtension = ".plist";

    for (const std::string& entry : *entries) {
        if (entry.empty() || entry.front() == '.') {
            continue;
        }

        if (entry.length() <= PlistExtension.length() || entry.compare(entry.length() - PlistExtension.length(), PlistExtension.length(), PlistExtension) != 0) {
            continue;
        }

        if (m_verboseLoggingEnabled) {
            std::cout << "[+] PXLManager::GetInstalledApplications(void) -- path: " << entry << "\n";
        }

        auto application = ApplicationWithBundleIdentifier(entry.substr(0, entry.length() - 6));
        if (application.has_value()) {
            applications.emplace_back(*application);
        }
    }

    return applications;
}

std::optional<PXLManager::PXLApplication> PXLManager::ApplicationWithBundleIdentifier(std::string_view bundleIdentifier) const {
    if (!IsDaemonInstalled()) {
        std::cerr << "[+] PXLdaemon is not installed! Install it in order to use this tool\n";
        return std::nullopt;
    }

    const std::string fullPath { std::string(PXLDatabasePath) + "/" + std::string(bundleIdentifier) + ".plist" };
    
    if (m_verboseLoggingEnabled) {
        std::cout << "[+] PXLManager::ApplicationWithBundleIdentifier(std::string_view) -- fullPath: " << fullPath << "\n";
    }

    auto fileContents = m_afcSession.ReadFile(fullPath);
    if (!fileContents.has_value()) {
        return std::nullopt;
    }

    std::unique_ptr<PList::Structure> structure(PList::Structure::FromMemory((const char *)fileContents->data(), fileContents->size()));
    if (!structure || structure->GetType() != PLIST_DICT) {
        return std::nullopt;
    }

    PList::Dictionary dictionary(*(PList::Dictionary *)structure.get());

    PXLApplication application;
    application.name = dictionary.Get<PList::String>("RDPxlPackageName")->GetValue();
    application.bundleIdentifier = bundleIdentifier;
    application.version = dictionary.Get<PList::String>("RDPxlPackageVersion")->GetValue();
    application.description = dictionary.Get<PList::String>("RDPxlPackageDesc")->GetValue();
    
    if (m_verboseLoggingEnabled) {
        std::cout << "[+] PXLManager::ApplicationWithBundleIdentifier(std::string_view) -- application.name: " << application.name << "\n";
        std::cout << "[+] PXLManager::ApplicationWithBundleIdentifier(std::string_view) -- application.bundleIdentifier: " << application.bundleIdentifier << "\n";
        std::cout << "[+] PXLManager::ApplicationWithBundleIdentifier(std::string_view) -- application.version: " << application.version << "\n";
        std::cout << "[+] PXLManager::ApplicationWithBundleIdentifier(std::string_view) -- application.description: " << application.description << "\n";
    }

    return application;
}

bool PXLManager::InstallDaemon(void) const {
    if (m_verboseLoggingEnabled) {
        std::cout << "[+] PXLManager::InstallDaemon(void) -- Start\n";
    }

    if (!m_afcSession.PathExists("/etc/init.d")) {
        if (m_verboseLoggingEnabled) {
            std::cout << "[+] PXLManager::InstallDaemon(void) -- /etc/init.d doesn't exist, creating\n";
        }

        bool success = m_afcSession.CreateDirectory("/etc/init.d");
        if (m_verboseLoggingEnabled) {
            std::cout << "[+] PXLManager::InstallDaemon(void) -- Status of creation: " << success << "\n";
        }
    }

    std::unordered_map<std::string, std::string> fileMap = {
        { "files/hackinit.sh", "/etc/hackinit.sh" },
        { "files/pxl.sh", "/etc/init.d/pxl.sh" },
        { "files/PXLdaemon", PXLDaemonPath.data() },
        { "files/com.apple.update.plist.hackinit", "/System/Library/LaunchDaemons/com.apple.update.plist" },
    };
    
    for (const auto& [localPath, remotePath] : fileMap) {
        if (m_verboseLoggingEnabled) {
            std::cout << "[+] PXLManager::InstallDaemon(void) -- " << localPath << " -> " << remotePath << "\n";
        }

        std::ifstream file(localPath, std::ios::binary);
        std::vector<uint8_t> fileData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        if (fileData.empty()) {
            std::cerr << "[-] Failed to read file " << localPath << "\n";
            return false;
        }
        
        if (!m_afcSession.WriteFile(remotePath, fileData.data(), fileData.size())) {
            std::cerr << "[-] Failed to push file to " << remotePath << "\n";
            return false;
        }
    }

    return true;
}

bool PXLManager::IsDaemonInstalled(void) const {
    return m_afcSession.PathExists(PXLFolderPath) && m_afcSession.PathExists(PXLDaemonPath);
}

bool PXLManager::RemoveApplication(const PXLApplication& application) const {
    if (!IsDaemonInstalled()) {
        std::cerr << "[+] PXLdaemon is not installed! Install it in order to use this tool\n";
        return false;
    }

    if (!m_afcSession.PathExists(PXLDropoffPath)) {
        if (m_verboseLoggingEnabled) {
            std::cout << "[+] PXLManager::RemoveApplication(void) -- No folder found at " << PXLDropoffPath << ", creating\n";
        }

        bool success = m_afcSession.CreateDirectory(PXLDropoffPath);
        if (m_verboseLoggingEnabled) {
            std::cout << "[+] PXLManager::RemoveApplication(void) -- Status of creation: " << success << "\n";
        }
    }

    PList::Dictionary commandsDictionary;
    commandsDictionary.Set("command", PList::String("remove"));
    commandsDictionary.Set("package", PList::String(application.bundleIdentifier.c_str()));
    
    if (m_verboseLoggingEnabled) {
        std::cout << "[+] PXLManager::RemoveApplication(void) -- Removing application with bundle identifier " << application.bundleIdentifier << "\n";
    }

    PList::Array commandsArray;
    commandsArray.Append(commandsDictionary);

    PList::Dictionary rootDictionary;
    rootDictionary.Set("commands", commandsArray);

    const std::string xmlString = rootDictionary.ToXml();
    
    if (xmlString.empty()) {
        return false;
    }
    
    const bool success = m_afcSession.WriteFile(PXLTriggerFilePath, (const uint8_t *)xmlString.data(), xmlString.size());
    
    if (m_verboseLoggingEnabled) {
        std::cout << "[+] PXLManager::RemoveApplication(void) -- Write file to " << PXLTriggerFilePath << " success: " << success << "\n";
    }

    return success;
}

bool PXLManager::InstallApplication(const std::vector<uint8_t>& pxlData) const {
    if (!IsDaemonInstalled()) {
        std::cerr << "[+] PXLdaemon is not installed! Install it in order to use this tool\n";
        return false;
    }

    if (!m_afcSession.PathExists(PXLDropoffPath)) {
        if (m_verboseLoggingEnabled) {
            std::cout << "[+] PXLManager::InstallApplication(void) -- No folder found at " << PXLDropoffPath << ", creating\n";
        }

        bool success = m_afcSession.CreateDirectory(PXLDropoffPath);
        if (m_verboseLoggingEnabled) {
            std::cout << "[+] PXLManager::InstallApplication(void) -- Status of creation: " << success << "\n";
        }
    }

    const std::string appPxlPath = std::string(PXLDropoffPath) + "/app.pxl";
    bool writeFileSuccess = m_afcSession.WriteFile(appPxlPath, pxlData.data(), pxlData.size());
    
    if (m_verboseLoggingEnabled) {
        std::cout << "[+] PXLManager::InstallApplication(void) -- Write file to " << appPxlPath << " status: " << writeFileSuccess << "\n";
    }

    if (!writeFileSuccess) {
        return false;
    }

    PList::Dictionary commandsDictionary;
    commandsDictionary.Set("command", PList::String("install"));
    commandsDictionary.Set("package", PList::String("app.pxl"));

    PList::Array commandsArray;
    commandsArray.Append(commandsDictionary);

    PList::Dictionary rootDictionary;
    rootDictionary.Set("commands", commandsArray);

    const std::string xmlString = rootDictionary.ToXml();
    
    if (xmlString.empty()) {
        return false;
    }
    
    const bool success = m_afcSession.WriteFile(PXLTriggerFilePath, (const uint8_t *)xmlString.data(), xmlString.size());
    
    if (m_verboseLoggingEnabled) {
        std::cout << "[+] PXLManager::InstallApplication(void) -- Write file to " << PXLTriggerFilePath << " success: " << success << "\n";
    }

    return success;
}