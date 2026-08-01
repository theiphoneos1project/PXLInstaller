#include "ControlPanel.hpp"
#include "Events.hpp"

#include <wx/statline.h>

#include <fstream>

ControlPanel::ControlPanel(wxWindow *parent) : wxPanel(parent) {
    m_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(m_sizer);

    auto titleFont = GetFont();
    titleFont.SetPointSize(20);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);

    auto subtitleFont = GetFont();
    subtitleFont.SetPointSize(15);

    auto *title = new wxStaticText(this, wxID_ANY, "PXLInstaller");
    title->SetFont(titleFont);
    m_sizer->Add(title, 0, wxLEFT | wxTOP, 20);

    auto *subtitle = new wxStaticText(this, wxID_ANY, "By Nightwind");
    subtitle->SetFont(subtitleFont);
    m_sizer->Add(subtitle, 0, wxLEFT | wxRIGHT, 20);
    m_sizer->AddStretchSpacer();
    
    m_statusText = new wxStaticText(this, wxID_ANY, wxEmptyString);
    m_sizer->Add(m_statusText, 0, wxLEFT | wxRIGHT, 20);
    
    m_sizer->AddStretchSpacer();
    m_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL), 0, wxEXPAND | wxALL, 10);
    m_sizer->AddStretchSpacer();
    
    m_creditsText = new wxStaticText(this, wxID_ANY, "Special thanks to: EthanArbuckle, Nate True (iBrickr), PXL archive", wxDefaultPosition, wxDefaultSize);
    m_sizer->Add(m_creditsText, 0, wxLEFT, 20);
    
    m_sizer->AddSpacer(12);
    
    m_sizer->Add(new wxStaticText(this, wxID_ANY, "References:", wxDefaultPosition, wxDefaultSize), 0, wxLEFT, 20);
    
    m_sizer->Add(MakeCustomHyperlink("theiphoneos1project/PXLInstaller", "https://github.com/theiphoneos1project/PXLInstaller"), 0, wxLEFT, 20);
    m_sizer->Add(MakeCustomHyperlink("EthanArbuckle/iOS1.0-Jailbreak", "https://github.com/EthanArbuckle/iOS1.0-Jailbreak"), 0, wxLEFT, 20);
    m_sizer->Add(MakeCustomHyperlink("archive/pxl", "https://code.google.com/archive/p/pxl/"), 0, wxLEFT, 20);
    
    m_sizer->AddSpacer(12);
    
    m_sizer->Add(new wxStaticText(this, wxID_ANY, "Find us at:", wxDefaultPosition, wxDefaultSize), 0, wxLEFT, 20);
    m_sizer->Add(MakeCustomHyperlink("NightwindDev GitHub", "https://github.com/NightwindDev"), 0, wxLEFT, 20);
    m_sizer->Add(MakeCustomHyperlink("The iPhone OS 1 Project GitHub", "https://github.com/theiphoneos1project"), 0, wxLEFT, 20);
    m_sizer->Add(MakeCustomHyperlink("@NightwindDev Twitter", "https://twitter.com/NightwindDev"), 0, wxLEFT, 20);
    
    m_sizer->AddSpacer(12);
    
    m_sizer->Add(new wxStaticText(this, wxID_ANY, "Device mockup credits:", wxDefaultPosition, wxDefaultSize), 0, wxLEFT, 20);
    m_sizer->Add(MakeCustomHyperlink("Rafael Fernandez (TheGoldenBox)", "https://commons.wikimedia.org/wiki/User:TheGoldenBox"), 0, wxLEFT, 20);
    m_sizer->Add(MakeCustomHyperlink("CC BY-SA 4.0 License", "https://creativecommons.org/licenses/by-sa/4.0/"), 0, wxLEFT, 20);
    
    m_sizer->AddStretchSpacer();

    m_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    auto *buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    m_daemonButton = new wxButton(this, ID_INSTALL_DAEMON, "Install Daemon");
    m_daemonButton->Hide();
    buttonSizer->Add(m_daemonButton, 0, wxRIGHT, 8);

    m_installButton = new wxButton(this, ID_INSTALL_APPLICATION, "Install Application");
    buttonSizer->Add(m_installButton, 0, wxRIGHT, 8);

    m_logsButton = new wxButton(this, ID_VIEW_LOGS, "View Device Logs");
    buttonSizer->Add(m_logsButton, 0);

    m_sizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 20);
}

void ControlPanel::SetStatus(const wxString& message, bool isError) {
    m_statusText->SetLabel(message);

    const int availableWidth = GetClientSize().GetWidth() - 40;
    m_statusText->Wrap(availableWidth);

    m_statusText->SetForegroundColour(isError ? *wxRED : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    
    m_sizer->Layout();
    Refresh();
}

void ControlPanel::SetButtonsEnabled(bool enabled) {
    m_logsButton->Enable(enabled);
    m_installButton->Enable(enabled);
}

void ControlPanel::SetDaemonInstalled(bool installed) {
    if (installed) {
        m_daemonButton->Hide();
    } else {
        m_daemonButton->Show();
    }

    m_sizer->Layout();
}

wxHyperlinkCtrl *ControlPanel::MakeCustomHyperlink(const wxString& name, const wxString& link) {
    auto *hyperlink = new wxHyperlinkCtrl(
        this,
        wxID_ANY,
        name,
        link
    );
    hyperlink->SetNormalColour(wxColour(138, 180, 248));
    hyperlink->SetHoverColour(wxColour(174, 203, 250));
    hyperlink->SetVisitedColour(wxColour(200, 140, 255));
    return hyperlink;
}
