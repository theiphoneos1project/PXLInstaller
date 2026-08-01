#include "DevicePanel.hpp"
#include "Events.hpp"

#include <wx/mstream.h>

#include "generated/iphone1_1.h"
#include "generated/ipod1_1.h"
#include "generated/connect.h"

DevicePanel::DevicePanel(wxWindow *parent) : wxPanel(parent) {
    m_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(m_sizer);
    SetMinSize(wxSize(200, -1));
}

void DevicePanel::RenderDevice(std::string_view productType, const std::vector<PXLManager::PXLApplication>& applications) {
    m_sizer->Clear(true);
    
    const unsigned char *imageData = productType == "iPod1,1" ? ipod1_1_image : iphone1_1_image;
    const size_t imageSize = productType == "iPod1,1" ? ipod1_1_image_len : iphone1_1_image_len;

    if (imageData && imageSize != 0) {
        wxMemoryInputStream stream(imageData, imageSize);
        wxImage image(stream, wxBITMAP_TYPE_PNG);

        if (image.IsOk()) {
            int scaledHeight = image.GetHeight() * 200.0 / image.GetWidth();
            image = image.Scale(
                200.0, scaledHeight, 
                wxIMAGE_QUALITY_HIGH
            );

            auto *imageContainer = new wxPanel(this, wxID_ANY);
            auto *bitmap = new wxStaticBitmap(imageContainer, wxID_ANY, wxBitmap(image));
            
            auto *containerSizer = new wxBoxSizer(wxVERTICAL);
            imageContainer->SetSizer(containerSizer);
            containerSizer->Add(bitmap, 1, wxALIGN_CENTER);

            auto *overlay = new wxPanel(imageContainer, wxID_ANY);
            auto *overlaySizer = new wxBoxSizer(wxVERTICAL);
            overlay->SetSizer(overlaySizer);

            auto *appList = new wxScrolledWindow(overlay, wxID_ANY);
            appList->SetScrollRate(0, 10);
            
            auto *appListSizer = new wxBoxSizer(wxVERTICAL);
            appList->SetSizer(appListSizer);

            auto appLabelFont = appList->GetFont();
            appLabelFont.SetPointSize(10);
            
            for (size_t i = 0; i < applications.size(); i++) {
                const auto& application = applications.at(i);
                
                auto *row = new wxBoxSizer(wxHORIZONTAL);

                auto *appLabel = new wxStaticText(appList, wxID_ANY, wxString(application.name), wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
                appLabel->SetForegroundColour(*wxWHITE);
                appLabel->SetMinSize(wxSize(1, -1));
                appLabel->SetFont(appLabelFont);

                appLabel->Bind(wxEVT_LEFT_UP, [application](wxMouseEvent&) {
                    wxString message = wxString::Format(
                        "Name: %s\nBundle ID: %s\nVersion: %s\nDescription: %s",
                        wxString(application.name),
                        wxString(application.bundleIdentifier),
                        wxString(application.version),
                        wxString(application.description)
                    );
                    wxMessageBox(message, wxString(application.name), wxICON_INFORMATION);
                });
                
                appLabel->SetCursor(wxCursor(wxCURSOR_HAND));

                row->Add(appLabel, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);

                row->AddSpacer(5);

                auto *removeButton = new wxButton(appList, ID_REMOVE_APPLICATION, "✕", wxDefaultPosition, wxSize(20, 15));
                removeButton->Bind(wxEVT_BUTTON, [bundleIdentifier = application.bundleIdentifier](wxCommandEvent& event) {
                    event.SetString(bundleIdentifier);
                    event.Skip();
                });
                row->Add(removeButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

                appListSizer->Add(row, 0, wxEXPAND | wxTOP | wxBOTTOM, 6);

                if (i < applications.size() - 1) {
                    auto *separator = new wxPanel(appList, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
                    separator->SetBackgroundColour(wxColour(85, 85, 85));
                    appListSizer->Add(separator, 0, wxEXPAND);
                }
            }
            
            overlaySizer->Add(appList, 1, wxEXPAND);

            auto initialRect = GetRectForProduct(productType, scaledHeight);
            if (initialRect.has_value()) {
                overlay->SetSize(*initialRect);
            }

            m_sizer->AddStretchSpacer(1);
            m_sizer->Add(imageContainer, 0, wxALIGN_CENTER);
            m_sizer->AddStretchSpacer(1);
            
            Layout();
        }
    }
}

void DevicePanel::RenderTransientView() {
    m_sizer->Clear(true);
    
    wxMemoryInputStream stream(connect_image, connect_image_len);
    wxImage image(stream, wxBITMAP_TYPE_PNG);

    if (image.IsOk()) {
        int scaledHeight = image.GetHeight() * 200.0 / image.GetWidth();
        image = image.Scale(
            200.0, scaledHeight, 
            wxIMAGE_QUALITY_HIGH
        );

        auto *imageContainer = new wxPanel(this, wxID_ANY);
        auto *bitmap = new wxStaticBitmap(imageContainer, wxID_ANY, wxBitmap(image));
        
        auto *containerSizer = new wxBoxSizer(wxVERTICAL);
        imageContainer->SetSizer(containerSizer);
        containerSizer->Add(bitmap, 1, wxALIGN_CENTER);

        m_sizer->AddStretchSpacer(1);
        m_sizer->Add(imageContainer, 0, wxALIGN_CENTER);
        m_sizer->AddStretchSpacer(1);

        Layout();
    }
}

void DevicePanel::ClearDevice(void) {
    m_sizer->Clear(true);
    Layout();
}

std::optional<wxRect> DevicePanel::GetRectForProduct(std::string_view productType, int panelHeight) {
    if (productType == "iPod1,1") {
        int height = 244;
        int yPosition = (panelHeight / 2) - (height / 2);
        return wxRect(wxPoint(18, yPosition), wxSize(163, height));
    } else if (productType == "iPhone1,1") {
        int height = 249;
        int yPosition = (panelHeight / 2) - (height / 2) + 2;
        return wxRect(wxPoint(18, yPosition), wxSize(166, height));
    }
    
    return std::nullopt;
}
