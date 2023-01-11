// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <iostream>
#include "TextDisplayDialog.h"

using namespace std;

namespace Wxiv
{
    TextDisplayDialog::TextDisplayDialog(wxWindow* parent, wxString title, wxSize size) : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, size)
    {
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        // wxStaticText* msg = new wxStaticText(this, wxID_ANY, wxT("Please select a name or type a new one:"));

        this->textCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_MULTILINE);
        sizer->Add(textCtrl, 1, wxEXPAND | wxALL);

        // can set content from stream
        // ostream stream(tc);
        // stream << 123.456 << " some text\n";
        // stream.flush();

        // wxHtmlWindow* hw = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
        // sizer->Add(hw, wxEXPAND | wxALL);

        wxSizer* buttonSizer = CreateButtonSizer(wxOK);
        sizer->Add(buttonSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 8);

        SetSizer(sizer);
        CentreOnParent();
    }

    void TextDisplayDialog::loadFile(wxFileName path)
    {
        this->textCtrl->LoadFile(path.GetFullPath());
    }
}
