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
        ext.LowerCase();
        return ext == "dcm";
    }

    /**
     * @brief Need our own load to leave out structure dcm file(s).
     */
    void ImageListSourceDcmDirectory::load(wxString dirPath)
    {
        vector<wxString> paths = listFilesInDir(dirPath);
        auto thisPredicate = [=, this](const wxString& s) -> bool { return this->checkSupportedFile(s); };
        vector<wxString> selectedPaths = vectorSelect<wxString>(paths, thisPredicate);

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

    cv::Point2f applyAffineTransform(const cv::Mat& transform, const cv::Point2f& point)
    {
        int rows = transform.rows;
        int cols = transform.cols;
        if (rows != 3 || cols != 3)
        {
            throw runtime_error("Transform matrix must be 3x3.");
        }

        cv::Mat src(3, 1, CV_64F);
        src.at<double>(0, 0) = point.x;
        src.at<double>(1, 0) = point.y;
        src.at<double>(2, 0) = 1.0;
        cv::Mat dst = transform * src;
        return cv::Point2f(dst.at<double>(0, 0) / dst.at<double>(2, 0), dst.at<double>(1, 0) / dst.at<double>(2, 0));
    }

    bool ImageListSourceDcmDirectory::loadImage(std::shared_ptr<WxivImage> image)
    {
        if (!image->getIsLoaded())
        {
            vector<cv::Mat> mats;
            string uuidStr;
            wxString fullPath = image->getPath().GetFullPath();
            cv::Mat affineXform; // from world coords to pixel coords

            if (!wxLoadDicomImage(fullPath, mats, uuidStr, affineXform))
            {
                throw runtime_error("Failed to load and decode image file.");
            }

            if (mats.size() > 0)
            {
                // main image
                image->setImage(mats[0]);

                // I'm not sure multiple images per dcm file is a thing, so defer until have a case to develop against.
                //// pages
                // for (int i = 1; i < mats.size(); i++)
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
                    // each Contour has all slices/images.
                    auto iter = std::find(contour.referencedSopInstanceUids.begin(), contour.referencedSopInstanceUids.end(), uuidStr);

                    if (iter != contour.referencedSopInstanceUids.end())
                    {
                        int sliceIndex = iter - contour.referencedSopInstanceUids.begin();
                        std::vector<ContourPoint> slicePoints = contour.slicePoints[sliceIndex];
                        Polygon poly;
                        poly.colorRgb = cv::Scalar(contour.rgbColor[0], contour.rgbColor[1], contour.rgbColor[2]);
                        poly.pointDim = 1;
                        poly.lineThickness = 1;

                        for (const ContourPoint& pt : slicePoints)
                        {
                            cv::Point2f worldPt(pt.x, pt.y);
                            cv::Point2f pixelPt = applyAffineTransform(affineXform, worldPt);
                            poly.points.push_back(pixelPt);
                        }

                        image->getShapes().polygons.push_back(poly);
                    }
                }
            }
        }

        return true;
    }
}