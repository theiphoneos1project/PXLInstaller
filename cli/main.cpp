#include <iostream>
#include <fstream>
#include <sstream>

#include "lockdownd/LockdownDaemonClient.hpp"
#include "afc/AppleFileConduitSession.hpp"
#include "pxl/PXLManager.hpp"

static bool g_isVerboseLoggingEnabled = false;

static std::string EscapeJSON(std::string_view input) {
    std::ostringstream stream;
    
    for (char character : input) {
        switch (character) {
            case '"': stream << "\\\""; break;
            case '\\': stream << "\\\\"; break;
            case '\b': stream << "\\b"; break;
            case '\f': stream << "\\f"; break;
            case '\n': stream << "\\n"; break;
            case '\r': stream << "\\r"; break;
            case '\t': stream << "\\t"; break;
            default: stream << character; break;
        }
    }

    return stream.str();
}

static std::string GetFilesDirectory(void) {
    char directory[PATH_MAX];
    GetExecutableDirectory(directory, sizeof(directory));
    return std::string(directory) + "/files/";
}

static void PrintUsage(void) {
    std::cout << "\n"
              << "  PXLInstaller CLI  \n"
              << "  - By Nightwind -  \n\n"
              << "Usage:\n"
              << "  ./pxlinstaller --install-daemon\n"
              << "  ./pxlinstaller --list-applications\n"
              << "  ./pxlinstaller --list-applications-json\n"
              << "  ./pxlinstaller --remove-application <Bundle ID>\n"
              << "  ./pxlinstaller --install-application <path/to/pxl>\n"
              << "  ./pxlinstaller --dump-logs\n\n";
}

#ifdef _WIN32
struct WSAProcess {
public:
    WSAProcess() {
        WSAStartup(MAKEWORD(2, 2), &m_wsa);
    }

    ~WSAProcess() {
        WSACleanup();
    }
private:
    WSADATA m_wsa;
};
#endif

int main(int argc, char *argv[]) {
    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    if (std::string(argv[argc - 1]) == "--verbose") {
        g_isVerboseLoggingEnabled = true;
    }

#ifdef _WIN32
    WSAProcess wsaProcess;
#endif
    
    LockdownDaemonClient lockdowndClient;
    bool successfullyOpened = lockdowndClient.Open();
    if (!successfullyOpened) {
        std::cerr << "[-] Failed to open USB connection to device...\n";
        return EXIT_FAILURE;
    }

    if (g_isVerboseLoggingEnabled) {
        std::cout << "[+] We're in!\n";
        std::cout << "[+] USB connected, starting lockdown session...\n\n";
    }

    std::string sessionError;
    auto sessionID = lockdowndClient.StartPairedSession(sessionError);
    if (!sessionID.has_value()) {
        std::cerr << "[-] Failed to start paired session: " << sessionError << "\n";
        return EXIT_FAILURE;
    }

    if (g_isVerboseLoggingEnabled) {
        std::cout << "Device: " << lockdowndClient.GetValueString("ProductType").value_or("Unknown Device")
                << " (" << lockdowndClient.GetValueString("DeviceName").value_or("Unknown Device Name") << ")\n";
        std::cout << "Software version: " << lockdowndClient.GetValueString("ProductVersion").value_or("Unknown Version") 
                << " (" << lockdowndClient.GetValueString("BuildVersion").value_or("Unknown Build Version") << ")\n\n";
    }

    std::string serviceError;
    auto afcPort = lockdowndClient.StartService("com.apple.afc2", serviceError);
    if (!afcPort.has_value()) {
        std::cerr << "[-] StartService(\"com.apple.afc2\") failed - " << serviceError << "\n";
        std::cerr << "Make sure your device is jailbroken with a Apple File Conduit \"2\" patch enabled!\n";
        return EXIT_FAILURE;
    }

    AppleFileConduitSession afcSession(lockdowndClient.GetPipe(), *afcPort);
    bool successfullyConnected = afcSession.Connect();
    if (!successfullyConnected) {
        std::cerr << "[-] Failed to connect to afc2\n";
        return EXIT_FAILURE;
    }

    PXLManager manager(afcSession);
    manager.SetVerboseLoggingEnabled(g_isVerboseLoggingEnabled);
    
    std::string_view command = argv[1];
    
    if (command == "--install-daemon") {
        bool successfullyInstalled = manager.InstallDaemon(GetFilesDirectory());
        if (successfullyInstalled) {
            std::cout << "[+] Successfully installed PXLdaemon!\n";
            std::cout << "Please reboot your device to finish the installation!\n";
        } else {
            std::cerr << "[-] Failed to install PXLdaemon...\n";
            return EXIT_FAILURE;
        }
    } else if (command == "--list-applications") {
        auto applications = manager.GetInstalledApplications();
        if (!applications.has_value()) {
            std::cerr << "[-] Could not list installed applications!\n";
            return EXIT_FAILURE;
        }

        for (const PXLManager::PXLApplication& application : *applications) {
            std::cout << application.name << " (" << application.bundleIdentifier << ") @ " << application.version << std::endl;
        }
    } else if (command == "--list-applications-json") {
        auto applications = manager.GetInstalledApplications();
        if (!applications.has_value()) {
            std::cerr << "[-] Could not list installed applications!\n";
            return EXIT_FAILURE;
        }

        std::cout << "[\n";
        bool isFirst = true;

        for (const PXLManager::PXLApplication& application : *applications) {
            if (!isFirst) {
                std::cout << ",\n";
            }
            isFirst = false;

            std::cout << "\t{\n"
                      << "\t\t\"RDPxlPackageName\": \"" << EscapeJSON(application.name) << "\",\n"
                      << "\t\t\"RDPxlPackageVersion\": \"" << EscapeJSON(application.version) << "\",\n"
                      << "\t\t\"RDPxlPackageDesc\": \"" << EscapeJSON(application.description) << "\",\n"
                      << "\t\t\"CFBundleIdentifier\": \"" << EscapeJSON(application.bundleIdentifier) << "\"\n"
                      << "\t}";
        }

        std::cout << "\n]\n";
    } else if (command == "--remove-application") {
        if (argc < 3 || std::string(argv[2]) == "--verbose") {
            std::cerr << "[-] Error: --remove-application requires the bundle identifier of the app you want to remove\n";
            return EXIT_FAILURE;
        }
        
        auto application = manager.ApplicationWithBundleIdentifier(argv[2]);
        if (!application.has_value()) {
            std::cerr << "[-] Could not get application!\n";
            return EXIT_FAILURE;
        }
        
        bool sucessfullyRemoved = manager.RemoveApplication(*application);
        if (sucessfullyRemoved) {
            std::cout << "[+] Successfully removed " << (*application).name << " (" << (*application).bundleIdentifier << ") @ " << (*application).version << " from the device! Your device will now respring.\n";
        } else {
            std::cout << "[-] Failed to remove application. Please check logs at " << PXLManager::PXLFolderPath << "/pxl.log" << std::endl;
            return EXIT_FAILURE;
        }
    } else if (command == "--install-application") {
        if (argc < 3 || std::string(argv[2]) == "--verbose") {
            std::cerr << "[-] Error: --install-application requires the path of a .pxl on the host\n";
            return EXIT_FAILURE;
        }
        
        std::ifstream file(argv[2], std::ios::binary);
        std::vector<uint8_t> pxlData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        if (pxlData.empty()) {
            std::cerr << "[-] Failed to read .pxl file\n";
            return EXIT_FAILURE;
        }

        bool successfullyInstalled = manager.InstallApplication(pxlData);
        if (successfullyInstalled) {
            std::cout << "[+] Successfully installed " << argv[2] << " on device! Your device will now respring.\n";
        } else {
            std::cout << "[-] Failed to install PXL. Please check logs at " << PXLManager::PXLFolderPath << "/pxl.log" << std::endl;
            return EXIT_FAILURE;
        }
    } else if (command == "--dump-logs") {
        auto contents = afcSession.ReadFile("/var/root/Media/PXL/pxl.log");
        if (contents) {
            std::cout.write(reinterpret_cast<const char *>(contents->data()), contents->size());
        } else {
            std::cerr << "[-] Failed to read logs!\n";
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
