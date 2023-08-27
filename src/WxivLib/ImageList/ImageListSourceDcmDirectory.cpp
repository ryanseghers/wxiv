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
#include "DicomUtil.h"

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

    bool ImageListSourceDcmDirectory::loadImage(std::shared_ptr<WxivImage> image)
    {
        static std::mutex imreadMutex;

        if (!image->getIsLoaded())
        {
            const std::lock_guard<std::mutex> lock(imreadMutex); // imread is not MT-safe
            vector<cv::Mat> mats;
            wxString fullPath = image->getPath().GetFullPath();

            if (!wxLoadDicomImage(fullPath, mats))
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

            // Contours
            //try
            //{
            //    image->getShapes().tryLoadNeighborShapesFile(image->getPath());
            //}
            //catch (std::runtime_error& ex)
            //{
            //    image->setShapeSetLoadError(wxString(ex.what()));
            //}
        }
    }
}