// Copyright(c) 2023 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/combobox.h>

#include "CollageSpec.h"

namespace Wxiv
{
    class CollageSpecDialog : public wxDialog
    {
      public:
        CollageSpecDialog(wxWindow* parent, ImageUtil::CollageSpec& spec);

      private:
        void OnOk(wxCommandEvent& event);
        void OnCancel(wxCommandEvent& event);

        ImageUtil::CollageSpec& m_spec;
        wxSpinCtrl* m_colCountCtrl;
        wxSpinCtrl* m_imageWidthPxCtrl;
        wxSpinCtrl* m_marginPxCtrl;
        wxSpinCtrlDouble* m_fontScaleCtrl;
        wxComboBox* m_fontFaceCtrl;
        wxCheckBox* m_doBlackBackgroundCtrl;
        wxCheckBox* m_doCaptionsCtrl;
    };
}
