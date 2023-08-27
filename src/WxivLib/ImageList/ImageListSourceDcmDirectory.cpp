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
#include "WxivUtil.h"
#include "VectorUtil.h"

using namespace std;
namespace fs = std::filesystem;

namespace Wxiv
{
    bool ImageListSourceDcmDirectory::checkSupportedFile(const wxString& name)
    {
        wxFileName wxn(name);
        wxString ext = wxn.GetExt();
        return ext == "dcm";
    }

    /**
    * @brief Need our own load to leave out structure dcm file(s).
    */
    void ImageListSourceDcmDirectory::load(wxString dirPath)
    {
        vector<wxString> paths = listFilesInDir(dirPath);
        auto predicate = [=](const wxString& s) -> bool { return checkSupportedFile(s); };
        vector<wxString> selectedPaths = vectorSelect<wxString>(paths, predicate);

        vector<wxString> imageDcmPaths, structureDcmPaths;
        
        for (const wxString& path : selectedPaths)
        {
            // Use Contour load to decide if structure file, because if so we want to load the contours anyway
            vector<Contour> thisContours = loadContours(path);

            if (!thisContours.empty())
            {
                structureDcmPaths.push_back(path);
                this->contours.insert(this->contours.end(), thisContours.begin(), thisContours.end());
            }
            else
            {
                imageDcmPaths.push_back(path);
            }
        }

        for (const auto& path : selectedPaths)
        {
            WxivImage* p = new WxivImage(path);
            this->images.push_back(std::shared_ptr<WxivImage>(p));
        }
    }

    bool ImageListSourceDcmDirectory::loadImage(std::shared_ptr<WxivImage> image)
    {
        if (!image->getIsLoaded())
        {
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

                // I'm not sure if this is a thing, so defer until have a case to develop against.
                //// pages
                //for (int i = 1; i < mats.size(); i++)
                //{
                //    WxivImage* pimg = new WxivImage(image->getPath());
                //    pimg->setImage(mats[i]);
                //    pimg->setPage(i);
                //    image->addPage(std::shared_ptr<WxivImage>(pimg));
                //}
            }
            else
            {
                throw runtime_error("Failed to read image file.");
            }

            // Contours
            if (!this->contours.empty())
            {
                for (const Contour& contour : this->contours)
                {
                    // Contour has all slices/images.
                    
                    //auto sliceIndex = std::find(contour.referencedSopInstanceUids.begin(), contour.referencedSopInstanceUids.end(), 
                    //    image->getSopInstanceUid()) - contour.referencedSopInstanceUids.begin();

                    //if (sliceIndex != contour.referencedSopInstanceUids.end())
                    //{
                    //    image->addContour(contour);
                    //}
                }
            }
        }
    }
}