#include "MainFrame.hpp"

#include <fstream>

#include <wx/statline.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>

#include "Events.hpp"

MainFrame::MainFrame() : 
    wxFrame(
        nullptr, 
        wxID_ANY, 
        "PXLInstaller", 
        wxDefaultPosition, 
        wxSize(s_minimumWindowWidth, s_minimumWindowHeight), 
        wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX)
    )
{
    wxImage::AddHandler(new wxPNGHandler());
    
    auto *root = new wxBoxSizer(wxHORIZONTAL);

    m_controlPanel = new ControlPanel(this);
    auto *divider = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);
    m_devicePanel = new DevicePanel(this);

    m_controlPanel->SetButtonsEnabled(false);
    m_devicePanel->RenderTransientView();

    root->Add(m_controlPanel, 1, wxEXPAND);
    root->Add(divider, 0, wxEXPAND | wxTOP | wxBOTTOM, 10);
    root->Add(m_devicePanel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 20);

    Bind(wxEVT_BUTTON, &MainFrame::OnInstallApplicationClicked, this, ID_INSTALL_APPLICATION);
    Bind(wxEVT_BUTTON, &MainFrame::OnInstallDaemonClicked, this, ID_INSTALL_DAEMON);
    Bind(wxEVT_BUTTON, &MainFrame::OnLogsClicked, this, ID_VIEW_LOGS);
    Bind(wxEVT_BUTTON, &MainFrame::OnRemoveApplicationClicked, this, ID_REMOVE_APPLICATION);
    Bind(wxEVT_TIMER, &MainFrame::OnConnectionPollTimer, this, ID_CONNECTION_TIMER);

    m_connectionTimer.SetOwner(this, ID_CONNECTION_TIMER);
    m_connectionTimer.Start(1500);

    CheckConnectionState();

    SetSizer(root);
    SetMinSize(wxSize(s_minimumWindowWidth, s_minimumWindowHeight));
}

void MainFrame::OnInstallDaemonClicked(wxCommandEvent&) {
    int confirm = wxMessageBox(
        "PXLdaemon is not installed on your device. In order to be able to install, remove, manage applications, and view logs on your device, you will have to install PXLdaemon.",
        "Install PXLdaemon?",
        wxYES_NO | wxICON_QUESTION
    );

    if (confirm != wxYES) {
        return;
    }

    if (!m_afcSession->PathExists("/bin/chmod")) {
        wxMessageBox("Device is jailbroken, but does not have a chmod binary at /bin/chmod. Install a BSD subsystem and try again.", "Error", wxICON_ERROR);
        return;
    }
    
    bool success = m_pxlManager->InstallDaemon(GetFilesDirectory());
    if (success) {
        wxMessageBox("Successfully installed PXLdaemon! Please reboot your device.", "Success", wxICON_INFORMATION);
    } else {
        wxMessageBox("Failed to install PXLdaemon! Please try again later.", "Error", wxICON_ERROR);
    }
}

void MainFrame::OnInstallApplicationClicked(wxCommandEvent&) {
    if (!m_pxlManager) {
        wxMessageBox("No device connected!", "Error", wxICON_ERROR);
        return;
    }
    
    wxFileDialog dialog(
        this,
        "Select a PXL file",
        wxEmptyString,
        wxEmptyString,
        "PXL Files (*.pxl)|*.pxl",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST
    );
    
    if (dialog.ShowModal() == wxID_CANCEL) {
        return;
    }
    
    const std::string path = dialog.GetPath().ToStdString();
    
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        wxMessageBox("Failed to open PXL file.", "Error", wxICON_ERROR);
        return;
    }

    std::vector<uint8_t> pxlData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    if (!file && !file.eof()) {
        wxMessageBox("Failed to read PXL file.", "Error", wxICON_ERROR);
        return;
    }
    
    if (pxlData.empty()) {
        wxMessageBox("PXL file is empty.", "Error", wxICON_ERROR);
        return;
    }
    
    bool success = m_pxlManager->InstallApplication(pxlData);
    if (success) {
        m_isProcessingCommand = true;
        m_controlPanel->SetStatus("Waiting for device to respring...");
        m_controlPanel->SetButtonsEnabled(false);
        
        wxMessageBox("Successfully installed application! Your device will now respring.", "Success", wxICON_INFORMATION);

        RefreshDeviceUI();
    } else {
        wxMessageBox("Failed to install application.", "Error", wxICON_ERROR);
    }
}

void MainFrame::OnRemoveApplicationClicked(wxCommandEvent& event) {
    if (!m_pxlManager) {
        wxMessageBox("No device connected!", "Error", wxICON_ERROR);
        return;
    }
    
    wxString bundleIdentifier = event.GetString();
    if (bundleIdentifier.IsEmpty()) {
        wxMessageBox("Could not get bundle identifier of app", "Error", wxICON_ERROR);
        return;
    }

    int confirm = wxMessageBox(
        wxString::Format("Are you sure you want to remove %s?", bundleIdentifier),
        "Confirm Removal",
        wxYES_NO | wxICON_QUESTION
    );

    if (confirm != wxYES) {
        return;
    }

    auto application = m_pxlManager->ApplicationWithBundleIdentifier(bundleIdentifier.ToStdString());
    if (application.has_value()) {
        bool success = m_pxlManager->RemoveApplication(*application);
        if (success) {
            m_isProcessingCommand = true;
            m_controlPanel->SetStatus("Waiting for device to respring...");
            m_controlPanel->SetButtonsEnabled(false);
            
            wxMessageBox("Application removed successfully! Your device will now respring.", "Success", wxICON_INFORMATION);
            
            RefreshDeviceUI();
        } else {
            wxMessageBox("Failed to remove application", "Error", wxICON_ERROR);
        }
    } else {
        wxMessageBox(wxString::Format("Could not get app for %s", bundleIdentifier), "Error", wxICON_ERROR);
    }
}

void MainFrame::OnLogsClicked(wxCommandEvent&) {
    if (!m_afcSession) {
        wxMessageBox("Not connected to Apple File Conduit \"2\"!", "Error", wxICON_ERROR);
        return;
    }

    wxDialog dialog(this, wxID_ANY, "Device Logs", wxDefaultPosition, wxSize(600, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    auto *sizer = new wxBoxSizer(wxVERTICAL);

    auto *text = new wxTextCtrl(&dialog, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);

    auto contents = m_afcSession->ReadFile("/var/root/Media/PXL/pxl.log");
    if (contents) {
        text->SetValue(reinterpret_cast<const char *>(contents->data()));
    } else {
        text->SetValue("Failed to read logs!");
        text->SetForegroundColour(wxColour(255, 0, 0));
    }

    text->ShowPosition(text->GetLastPosition());

    sizer->Add(text, 1, wxEXPAND | wxALL, 5);
    dialog.SetSizer(sizer);
    dialog.ShowModal();
}

void MainFrame::OnConnectionPollTimer(wxTimerEvent&) {
    CheckConnectionState();
}

void MainFrame::CheckConnectionState(void) {
    if (!m_lockdowndClient) {
        TryConnect();
        return;
    }
    
    if (!m_lockdowndClient->IsOpen()) {
        DisconnectDevice();
        return;
    }

    if (m_isProcessingCommand && !m_pxlManager->IsProcessingCommand()) {
        m_isProcessingCommand = false;
        m_controlPanel->SetButtonsEnabled(true);
        RefreshDeviceInfo();
        RefreshDeviceUI();
    }
}

void MainFrame::TryConnect(void) {
    auto client = std::make_unique<LockdownDaemonClient>();

    bool successfullyOpenedClient = client->Open();
    if (!successfullyOpenedClient) {
        m_controlPanel->SetStatus("Easily install PXL files onto your iPhone OS 1 device. Plug your iPhone OS 1 device in to get started.", false);
        return;
    }

    m_lockdowndClient = std::move(client);

    std::string sessionError;
    auto sessionID = m_lockdowndClient->StartPairedSession(sessionError);
    if (!sessionID.has_value()) {
        m_controlPanel->SetStatus("Please make sure this is an iPhone OS 1 device. Other devices are NOT supported. Failed to start paired session: " + sessionError + ".", true);
        m_controlPanel->SetButtonsEnabled(false);
        
        if (sessionError == "InvalidHostID") {
            wxMessageBox("Unplug and replug the device for repair.", "InvalidHostID", wxICON_ERROR);
        }

        return;
    }
    
    std::string serviceError;
    auto afcPort = m_lockdowndClient->StartService("com.apple.afc2", serviceError);
    if (!afcPort.has_value()) {
        m_controlPanel->SetStatus("Failed to start Apple File Conduit \"2\" service. Make sure you have jailbroken with an afc2 patch.", true);
        m_controlPanel->SetButtonsEnabled(false);
        return;
    }
    
    auto afcSession = std::make_unique<AppleFileConduitSession>(m_lockdowndClient->GetPipe(), *afcPort);
    bool successfullyConnected = afcSession->Connect();
    if (!successfullyConnected) {
        m_controlPanel->SetStatus("Could not connect to Apple File Conduit \"2\" service.", true);
        m_controlPanel->SetButtonsEnabled(false);
        return;
    }

    m_afcSession = std::move(afcSession);
    m_pxlManager = std::make_unique<PXLManager>(*m_afcSession);

    RefreshDeviceInfo();
    RefreshDeviceUI();

    if (!m_pxlManager->IsDaemonInstalled()) {
        m_controlPanel->SetDaemonInstalled(false);
        m_controlPanel->SetButtonsEnabled(false);
        return;
    }

    m_controlPanel->SetDaemonInstalled(true);
    m_controlPanel->SetButtonsEnabled(true);
}

void MainFrame::DisconnectDevice(void) {
    m_pxlManager.reset();
    m_afcSession.reset();
    m_lockdowndClient.reset();
    
    m_controlPanel->SetStatus("Easily install PXL files onto your iPhone OS 1 device. Plug your iPhone OS 1 device in to get started.", false);
    m_controlPanel->SetDaemonInstalled(true);
    m_controlPanel->SetButtonsEnabled(false);

    m_devicePanel->RenderTransientView();

    Layout();
}

void MainFrame::RefreshDeviceInfo(void) {
    auto productType = m_lockdowndClient->GetValueString("ProductType");
    auto productVersion = m_lockdowndClient->GetValueString("ProductVersion");

    if (productType.has_value() && productVersion.has_value()) {
        auto marketingName = GetMarketingProductName(*productType);
        m_controlPanel->SetStatus(wxString(marketingName.value_or(*productType) + " on iPhone OS " + *productVersion + " connected!"));
    } else {
        m_controlPanel->SetStatus("Failed to fetch device type or version", true);
    }
}

void MainFrame::RefreshDeviceUI(void) {
    if (!m_pxlManager) {
        return;
    }
    
    auto productType = m_lockdowndClient->GetValueString("ProductType");
    auto applications = m_pxlManager->GetInstalledApplications();
    
    if (productType.has_value()) {
        if (!applications.has_value()) {
            m_controlPanel->SetStatus("Failed to read installed applications from device.", true);
            applications = std::vector<PXLManager::PXLApplication>();
        }

        if (!m_pxlManager->IsDaemonInstalled()) {
            m_controlPanel->SetStatus("The PXL daemon is not installed. Install it to be able to install applications.", true);
            applications = std::vector<PXLManager::PXLApplication>();
        }

        m_devicePanel->RenderDevice(*productType, *applications);
    } else {
        m_devicePanel->RenderTransientView();
    }

    Layout();
}

std::optional<std::string> MainFrame::GetMarketingProductName(std::string_view productType) {
    if (productType == "iPod1,1") {
        return "iPod touch (1st generation)";
    } else if (productType == "iPhone1,1") {
        return "iPhone (1st generation)";
    }
    
    return std::nullopt;
}

std::string MainFrame::GetFilesDirectory(void) {
#if defined(__APPLE__)
    return wxStandardPaths::Get().GetResourcesDir().ToStdString() + "/files/";
#else
    const wxString executablePath = wxStandardPaths::Get().GetExecutablePath();
    return wxFileName(executablePath).GetPath().ToStdString() + "/files/";
#endif
}
