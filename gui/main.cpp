#include "MainFrame.hpp"
#include <wx/snglinst.h>
#include <memory>

class App : public wxApp {
public:
    bool OnInit() override {
#if _WIN32
        wxTheApp->SetAppearance(wxApp::Appearance::Dark);
#endif
        wxString name = wxString::Format("PXLInstaller-%s", wxGetUserId());
        m_checker = std::make_unique<wxSingleInstanceChecker>(name);
        if (m_checker->IsAnotherRunning()) {
            wxLogError("Another instance is already running. Exiting.");
            return false;
        }
        
        auto *frame = new MainFrame();
        frame->Show();
        return true;
    }
private:
    std::unique_ptr<wxSingleInstanceChecker> m_checker;
};

wxIMPLEMENT_APP(App);
