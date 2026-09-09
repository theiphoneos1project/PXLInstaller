//
// Copyright (c) 2026 Nightwind
//

#ifndef DEVICEPANEL_H
#define DEVICEPANEL_H

#include <wx/wx.h>
#include <string>
#include <optional>
#include "pxl/PXLManager.hpp"

class DevicePanel : public wxPanel {
public:
    DevicePanel(wxWindow *parent);
    
    void RenderDevice(std::string_view productType, const std::vector<PXLManager::PXLApplication>& applications);
    void RenderTransientView(void);
    void ClearDevice(void);

    static std::optional<wxRect> GetRectForProduct(std::string_view productType, int panelHeight);
private:
    wxBoxSizer *m_sizer = nullptr;
};

#endif // DEVICEPANEL_H
