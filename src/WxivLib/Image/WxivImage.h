// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <opencv2/opencv.hpp>

#include "WxWidgetsUtil.h"
#include "ImageUtil.h"
#include "ShapeSet.h"
#include "FloatHist.h"

namespace Wxiv
{
    /**
     * @brief Wrap cv::Mat to collect other information we might want for image viewer.
     * Defining and loading are two separate steps, in order to enable listing before loading all images in a dir.
     *
     * The memory management design is to have single copies of each image in the process, with shared_ptr to pass them
     * around. For that to be really clean, each object should be immutable. However in this case there are several
     * members that are mutable (shapes, imageStats, and hist). Right now with single-thread design that should be
     * fine but will have to change if go multi-threaded.
     *
     * TIF images (at least) can have multiple pages, and thus this keeps a vector of pointers to other pages.
     */
    class WxivImage
    {
      private:
        wxFileName path;

        // file type, probably just file name extension
        // This is not wide string on purpose, this is used as a sort of loose enum of image types, so not
        // necessarily the file name extension (though I hope foreign languages don't use wide chars for known image
        // types).
        std::string type;
        int page = 0; // for multi-page tif's, we split on load
        cv::Mat image;

        /**
         * For secondary pages loaded from this file.
         * This isn't supposed to provide a deep tree structure; this should only be non-empty for the top object.
         */
        std::vector<std::shared_ptr<WxivImage>> pages;

        // the following are mutable, despite the mem mgmt strategy
        ShapeSet shapes;
        ImageUtil::ImageStats imageStats;
        FloatHist hist;

        bool isLoaded = false;

        WxivImage();

      public:
        WxivImage(const std::filesystem::path& path);
        WxivImage(const wxFileName& path);
        WxivImage(const wxString& path);
        WxivImage(const cv::Mat& img);

        bool getIsLoaded();
        bool load();
        void save(const wxString& path, bool doParquet);
        bool empty();

        wxFileName getPath();
        wxString getDisplayName();
        std::string getTypeStr();
        int getPage();

        std::vector<std::shared_ptr<WxivImage>>& getPages();
        cv::Mat& getImage();
        ShapeSet& getShapes();
        ImageUtil::ImageStats& getImageStats();
        FloatHist& getFloatHist();
        void setFloatHist(FloatHist& hist);

        static bool checkSupportedExtension(const wxFileName& wfn);
    };
}
