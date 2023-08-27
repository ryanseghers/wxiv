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
#include "WxivUtil.h"

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

    bool ImageListSourceDirectory::checkSupportedFile(wxString name)
    {
        wxFileName wxn(name);
        return WxivImage::checkSupportedExtension(wxn);
    }

    bool ImageListSourceDirectory::loadImage(std::shared_ptr<WxivImage> image)
    {
        static std::mutex imreadMutex;

        if (!image->getIsLoaded())
        {
            const std::lock_guard<std::mutex> lock(imreadMutex); // imread is not MT-safe
            vector<cv::Mat> mats;
            wxString fullPath = image->getPath().GetPath();

            if (!wxLoadImage(fullPath, mats))
            {
                throw runtime_error("Failed to load and decode image file.");
            }

            if (mats.size() > 0)
            {
                // main image
                image->setImage(mats[0]);

                // pages
                for (int i = 1; i < mats.size(); i++)
                {
                    WxivImage* pimg = new WxivImage(image->getPath());
                    pimg->setImage(mats[i]);
                    pimg->setPage(i);
                    image->addPage(std::shared_ptr<WxivImage>(pimg));
                }
            }
            else
            {
                throw runtime_error("Failed to read image file.");
            }

            try
            {
                image->getShapes().tryLoadNeighborShapesFile(image->getPath());
            }
            catch (std::runtime_error& ex)
            {
                image->setShapeSetLoadError(wxString(ex.what()));
            }
        }
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
            if (checkSupportedFile(name))
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
