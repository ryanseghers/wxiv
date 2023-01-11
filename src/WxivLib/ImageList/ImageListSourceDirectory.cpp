// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <string>
#include <opencv2/opencv.hpp>
#include <fmt/core.h>
#include <filesystem>

#include "WxWidgetsUtil.h"
#include <wx/dir.h>

#include "MiscUtil.h"
#include "ImageListSourceDirectory.h"
#include "StringUtil.h"

using namespace std;
namespace fs = std::filesystem;

namespace Wxiv
{
    ImageListSourceDirectory::ImageListSourceDirectory()
    {
    }

    ImageListSourceDirectory::~ImageListSourceDirectory()
    {
    }

    /**
     * @brief Load images from dir. Only known image types. This sorts.
     * @param dirPath
     */
    void ImageListSourceDirectory::load(wxString dirPath)
    {
        // collect paths first to make sorting easy
        vector<wxString> paths;

        wxDir dir(dirPath);

        if (!dir.IsOpened())
        {
            throw std::runtime_error("Unable to open directory for read.");
        }

        wxString filespec; // doesn't handle multiple extensions, apparently
        wxString name;
        bool cont = dir.GetFirst(&name, filespec, wxDIR_FILES);

        while (cont)
        {
            wxFileName wxn(name);

            if (WxivImage::checkSupportedExtension(wxn))
            {
                paths.push_back(dirPath + "/" + name);
            }

            cont = dir.GetNext(&name);
        }

        std::sort(paths.begin(), paths.end(), [](const wxString& a, const wxString& b) -> bool { return a.compare(b) < 0; });

        for (const auto& path : paths)
        {
            WxivImage* p = new WxivImage(path);
            this->images.push_back(std::shared_ptr<WxivImage>(p));
        }
    }

    int ImageListSourceDirectory::getImageCount()
    {
        return (int)this->images.size();
    }

    std::shared_ptr<WxivImage> ImageListSourceDirectory::getImage(int idx)
    {
        if (idx < this->images.size())
        {
            return this->images[idx];
        }
        else
        {
            throw std::runtime_error("getImage index out of range");
        }
    }

    void ImageListSourceDirectory::addImagePages(int idx, std::vector<std::shared_ptr<WxivImage>>& pages)
    {
        images.insert(images.begin() + idx + 1, pages.begin(), pages.end());
    }
}
