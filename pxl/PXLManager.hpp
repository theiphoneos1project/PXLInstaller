#ifndef PXLMANAGER_H
#define PXLMANAGER_H

#include <optional>
#include <vector>
#include <string>
#include "afc/AppleFileConduitSession.hpp"

class PXLManager {
public:
    static constexpr std::string_view PXLDaemonPath = "/usr/sbin/PXLdaemon";
    static constexpr std::string_view PXLFolderPath = "/var/root/Media/PXL";
    static constexpr std::string_view PXLDatabasePath = "/var/root/Media/PXL/DB";
    static constexpr std::string_view PXLDropoffPath = "/var/root/Media/PXL/Dropoff";
    static constexpr std::string_view PXLTriggerFilePath = "/var/root/Media/PXL/Dropoff/PxlPickup";

    struct PXLApplication {
        std::string name;
        std::string bundleIdentifier;
        std::string version;
        std::string description;

        std::string ToJSON(void);
    };
public:
    explicit PXLManager(AppleFileConduitSession& afcSession);

    std::optional<std::vector<PXLApplication>> GetInstalledApplications(void) const;
    std::optional<PXLApplication> ApplicationWithBundleIdentifier(std::string_view bundleIdentifier) const;

    bool InstallDaemon(void) const;
    bool IsDaemonInstalled(void) const;
    
    bool RemoveApplication(const PXLApplication& application) const;
    bool InstallApplication(const std::vector<uint8_t>& pxlData) const;

    void SetVerboseLoggingEnabled(bool enabled) { m_verboseLoggingEnabled = enabled; };
private:
    AppleFileConduitSession &m_afcSession;
    bool m_verboseLoggingEnabled;
};

#endif // PXLMANAGER_H