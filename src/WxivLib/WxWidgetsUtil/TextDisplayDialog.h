// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once

#include "WxWidgetsUtil.h"
#include <wx/html/htmlwin.h>
#include <wx/dialog.h>

namespace Wxiv
{
    class TextDisplayDialog : public wxDialog
    {
        wxTextCtrl* textCtrl;

      public:
        TextDisplayDialog();
        TextDisplayDialog(wxWindow* parent, wxString title, wxSize size);

        void loadFile(wxFileName path);
    };
}
