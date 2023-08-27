// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once

#include <string>
#include <vector>

#include "WxWidgetsUtil.h"
#include "WxivImage.h"

namespace Wxiv
{
    void copyImageNameOrPathToClipboard(std::shared_ptr<WxivImage> image, bool doName);
    wxFileName findInstalledFile(wxString basename);
    bool checkIsOnlyAscii(const wxString& s);
    std::string toNativeString(const wxString& s);
    bool wxLoadImage(const wxString& path, std::vector<cv::Mat>& mats);
    bool wxSaveImage(const wxString& path, cv::Mat& img, bool doShowErrorDialog = true);
    void saveCollageSpecToConfig(const ImageUtil::CollageSpec& spec);
    void loadCollageSpecFromConfig(ImageUtil::CollageSpec& spec);
    std::vector<wxString> listFilesInDir(const wxString& dirPath);
}
