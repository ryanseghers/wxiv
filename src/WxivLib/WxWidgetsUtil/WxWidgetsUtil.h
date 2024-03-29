// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <vector>

#include <wx/image.h>
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <string>

namespace Wxiv
{
    void showAlertDialog(const char* msg);
    void showAlertDialog(std::string msg);

    void showMessageDialog(const char* msg);
    void showMessageDialog(std::string msg);

    void showHtmlDialog(std::string msg);

    wxString showSaveImageDialog(wxWindow* parent, const std::string& defaultExt, const wxString& configDirSaveKey = wxEmptyString,
        const wxString& defaultFileName = wxEmptyString);

    bool writeFile(const wxString& path, const std::vector<unsigned char>& buffer);
    bool saveToGif(std::vector<wxImage>& images, const wxString& path, int delayMs);
}
