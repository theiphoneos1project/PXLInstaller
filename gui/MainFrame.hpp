//
// Copyright (c) 2026 Nightwind
//

#ifndef MAINFRAME_H
#define MAINFRAME_H

#include <string_view>
#include <optional>
#include <string>

#include <wx/wx.h>

#include "lockdownd/LockdownDaemonClient.hpp"
#include "afc/AppleFileConduitSession.hpp"
#include "pxl/PXLManager.hpp"

#include "ControlPanel.hpp"
#include "DevicePanel.hpp"

class MainFrame : public wxFrame {
public:
    MainFrame();
private:
    void OnInstallDaemonClicked(wxCommandEvent&);
    
    void OnInstallApplicationClicked(wxCommandEvent&);
    void OnRemoveApplicationClicked(wxCommandEvent& event);
    
    void OnLogsClicked(wxCommandEvent&);

    void OnConnectionPollTimer(wxTimerEvent&);
    void CheckConnectionState(void);
    void TryConnect(void);

    void DisconnectDevice(void);

    void RefreshDeviceInfo(void);
    void RefreshDeviceUI(void);

    static std::optional<std::string> GetMarketingProductName(std::string_view productType);
    static std::string GetFilesDirectory(void);
private:
    wxTimer m_connectionTimer;

    bool m_isProcessingCommand = false;

    std::unique_ptr<LockdownDaemonClient> m_lockdowndClient;
    std::unique_ptr<AppleFileConduitSession> m_afcSession;
    std::unique_ptr<PXLManager> m_pxlManager;

    ControlPanel *m_controlPanel = nullptr;
    DevicePanel *m_devicePanel = nullptr;
    
    static constexpr uint32_t s_minimumWindowWidth = 725; 
    static constexpr uint32_t s_minimumWindowHeight = 500; 
};

#endif // MAINFRAME_H
