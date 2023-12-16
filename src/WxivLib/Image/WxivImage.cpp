// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <string>
#include <filesystem>
#include <exception>

#include <opencv2/opencv.hpp>
#include <fmt/core.h>
#include <fmt/xchar.h>

#include <wx/ffile.h>

#include "WxivImage.h"
#include "MiscUtil.h"
#include "StringUtil.h"
#include "WxivUtil.h"
#include "ArrowUtil.h"

// This should not be loading images, the ImageListSource should be doing that.
#include "DicomUtil.h"

using namespace std;
namespace fs = std::filesystem;

namespace Wxiv
{
    static std::mutex imreadMutex;

    WxivImage::WxivImage()
    {
    }

    WxivImage::WxivImage(const wxString& inPath)
    {
        this->path = wxFileName(inPath);
        this->type = getNormalizedExt(this->path.GetExt().ToStdString());
    }

    WxivImage::WxivImage(const wxFileName& inPath)
    {
        this->path = inPath;
        this->type = this->path.GetExt().ToStdString();
    }

    WxivImage::WxivImage(const std::filesystem::path& inPath)
    {
        this->path = wxFileName(inPath.wstring());
        this->type = getNormalizedExt(this->path.GetExt().ToStdString());
    }

    WxivImage::WxivImage(const cv::Mat& img) : image(img)
    {
        this->path = wxFileName();
        this->type = "png";
        this->isLoaded = true;
    }

    void WxivImage::setImage(cv::Mat& img)
    {
        this->image = img;
        this->isLoaded = true;
    }

    void WxivImage::setPage(int newPage)
    {
        this->page = newPage;
    }

    void WxivImage::addPage(std::shared_ptr<WxivImage> pageImage)
    {
        this->pages.push_back(pageImage);
    }

    bool WxivImage::getIsLoaded()
    {
        return this->isLoaded;
    }

    bool WxivImage::empty()
    {
        return this->image.empty();
    }

    wxFileName WxivImage::getPath()
    {
        return this->path;
    }

    /**
     * @brief Not just path basename because can have "page N" appended for display.
     */
    wxString WxivImage::getDisplayName()
    {
        if (!this->pages.empty())
        {
            // parent image
            string pageStr = fmt::format(" (page {})", this->page + 1);
            wxString name = this->path.GetFullName() + pageStr;
            return name;
        }
        else if (this->page > 0)
        {
            // child image
            string pageStr = fmt::format(" (page {})", this->page + 1);
            wxString name = wxString("    ") + this->path.GetFullName() + pageStr;
            return name;
        }
        else
        {
            return this->path.GetFullName();
        }
    }

    string WxivImage::getTypeStr()
    {
        return this->type;
    }

    int WxivImage::getPage()
    {
        return this->page;
    }

    std::vector<std::shared_ptr<WxivImage>>& WxivImage::getPages()
    {
        return this->pages;
    }

    cv::Mat& WxivImage::getImage()
    {
        return this->image;
    }

    ShapeSet& WxivImage::getShapes()
    {
        return this->shapes;
    }

    bool WxivImage::checkIsShapeSetLoadError()
    {
        return !shapeSetLoadError.empty();
    }

    void WxivImage::setShapeSetLoadError(wxString msg)
    {
        this->shapeSetLoadError = msg;
    }

    wxString WxivImage::getShapeSetLoadError()
    {
        return shapeSetLoadError;
    }

    ImageUtil::ImageStats& WxivImage::getImageStats()
    {
        return this->imageStats;
    }

    FloatHist& WxivImage::getFloatHist()
    {
        return this->hist;
    }

    void WxivImage::setFloatHist(FloatHist& newHist)
    {
        this->hist = newHist;
    }

    bool WxivImage::checkSupportedExtension(const wxFileName& name)
    {
        if (name.HasExt())
        {
            wxString ext = name.GetExt();

            // check supported extension
            try
            {
                string stdExt = ext.ToStdString();
                return ImageUtil::checkSupportedExtension(stdExt);
            }
            catch (...)
            {
                // apparently unicode chars in ext, so not one of our supported formats then, I guess
            }
        }

        return false;
    }

    /**
     * @brief Save to another path. This does not set this->path.
     * @param path The path to save to.
     * @param doParquet True for save to parquet, false for .geo.csv.
     */
    void WxivImage::save(const wxString& savePath, bool doParquet)
    {
        if (wxSaveImage(savePath, this->image))
        {
            // some shapes
            wxFileName shapesPath(savePath);
            shapesPath.SetExt(doParquet ? "parquet" : "geo.csv");

            ArrowUtil::saveFile(toNativeString(shapesPath.GetFullPath()), shapes.ptable);
        }
    }
}
