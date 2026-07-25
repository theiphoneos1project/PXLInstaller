#include "PXLManager.hpp"
#include <plist/plist.h>
#include <iostream>
#include <fstream>
#include <unordered_map>

static std::string PlistStringForKey(plist_t dictionary, const char *key) {
    plist_t node = plist_dict_get_item(dictionary, key);
    if (!node || plist_get_node_type(node) != PLIST_STRING) {
        return "";
    }

    char *value = nullptr;
    plist_get_string_val(node, &value);
    
    std::string result = value ? value : "";
    free(value);
    
    return result;
}

static std::optional<std::vector<uint8_t>> SerializePlist(plist_t plist) {
    char *buffer = nullptr;
    uint32_t length = 0;
    
    plist_to_xml(plist, &buffer, &length);

    if (!buffer) {
        return std::nullopt;
    }

    std::vector<uint8_t> result((uint8_t *)buffer, (uint8_t *)buffer + length);
    free(buffer);

    return result;
}

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
    
    plist_t dictionary = nullptr;
    plist_from_memory((const char *)fileContents->data(), (uint32_t)fileContents->size(), &dictionary, nullptr);
    if (!dictionary || plist_get_node_type(dictionary) != PLIST_DICT) {
        plist_free(dictionary);
        return std::nullopt;
    }

    PXLApplication application;
    application.name = PlistStringForKey(dictionary, "RDPxlPackageName");
    application.bundleIdentifier = bundleIdentifier;
    application.version = PlistStringForKey(dictionary, "RDPxlPackageVersion");
    application.description = PlistStringForKey(dictionary, "RDPxlPackageDesc");
    
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

    plist_t commandsDictionary = plist_new_dict();
    plist_dict_set_item(commandsDictionary, "command", plist_new_string("remove"));
    plist_dict_set_item(commandsDictionary, "package", plist_new_string(application.bundleIdentifier.c_str()));
    
    if (m_verboseLoggingEnabled) {
        std::cout << "[+] PXLManager::RemoveApplication(void) -- Removing application with bundle identifier " << application.bundleIdentifier << "\n";
    }

    plist_t commandsArray = plist_new_array();
    plist_array_append_item(commandsArray, commandsDictionary);

    plist_t rootDictionary = plist_new_dict();
    plist_dict_set_item(rootDictionary, "commands", commandsArray);

    auto xmlData = SerializePlist(rootDictionary);
    plist_free(rootDictionary);
    
    if (!xmlData.has_value()) {
        return false;
    }
    
    const bool success = m_afcSession.WriteFile(PXLTriggerFilePath, xmlData->data(), xmlData->size());
    
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

    plist_t commandsDictionary = plist_new_dict();
    plist_dict_set_item(commandsDictionary, "command", plist_new_string("install"));
    plist_dict_set_item(commandsDictionary, "package", plist_new_string("app.pxl"));

    plist_t commandsArray = plist_new_array();
    plist_array_append_item(commandsArray, commandsDictionary);

    plist_t rootDictionary = plist_new_dict();
    plist_dict_set_item(rootDictionary, "commands", commandsArray);

    auto xmlData = SerializePlist(rootDictionary);
    plist_free(rootDictionary);
    
    if (!xmlData.has_value()) {
        return false;
    }
    
    const bool success = m_afcSession.WriteFile(PXLTriggerFilePath, xmlData->data(), xmlData->size());
    
    if (m_verboseLoggingEnabled) {
        std::cout << "[+] PXLManager::InstallApplication(void) -- Write file to " << PXLTriggerFilePath << " success: " << success << "\n";
    }

    return success;
}