// Copyright(c) 2023 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once

#include <string>
#include <vector>

#include "WxWidgetsUtil.h"
#include "WxivImage.h"
#include "Contour.h"

namespace Wxiv
{
    bool wxLoadDicomImage(const wxString& path, std::vector<cv::Mat>& mats);
    std::vector<Contour> loadContours(const wxString& path);
}
