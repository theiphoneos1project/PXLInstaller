//
// Copyright (c) 2026 Nightwind
//

#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include <wx/wx.h>
#include <wx/hyperlink.h>

class ControlPanel : public wxPanel {
public:
    ControlPanel(wxWindow *parent);
    void SetStatus(const wxString& message, bool isError = false);
    void SetButtonsEnabled(bool enabled);
    void SetDaemonInstalled(bool installed);
private:
    wxHyperlinkCtrl *MakeCustomHyperlink(const wxString& name, const wxString& link);
private:
    wxStaticText *m_statusText = nullptr;
    wxStaticText *m_creditsText = nullptr;

    wxButton *m_daemonButton = nullptr;
    wxButton *m_installButton = nullptr;
    wxButton *m_logsButton = nullptr;

    wxBoxSizer *m_sizer = nullptr;
};

#endif // CONTROLPANEL_H
