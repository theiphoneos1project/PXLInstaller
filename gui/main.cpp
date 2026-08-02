#include "MainFrame.hpp"

class App : public wxApp {
public:
    bool OnInit() override {
#if _WIN32
        wxTheApp->SetAppearance(wxApp::Appearance::Dark);
#endif
        auto *frame = new MainFrame();
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(App);
