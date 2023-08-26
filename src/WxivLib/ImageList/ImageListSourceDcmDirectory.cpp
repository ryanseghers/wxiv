// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <string>
#include <opencv2/opencv.hpp>
#include <fmt/core.h>
#include <filesystem>

#include "WxWidgetsUtil.h"
#include <wx/dir.h>

#include "MiscUtil.h"
#include "ImageListSourceDcmDirectory.h"
#include "StringUtil.h"

using namespace std;
namespace fs = std::filesystem;

namespace Wxiv
{
    bool ImageListSourceDcmDirectory::checkSupportedFile(wxString name)
    {
        wxFileName wxn(name);
        wxString ext = wxn.GetExt();
        return ext == "dcm";
    }
}