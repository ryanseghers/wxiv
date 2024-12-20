// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <filesystem>
#include <fmt/core.h>

#include <opencv2/opencv.hpp>

#include <wx/clipbrd.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/confbase.h>
#include <wx/dir.h>

#include "WxivUtil.h"
#include "MiscUtil.h"
#include "WxWidgetsUtil.h"
#include "StringUtil.h"
#include "ImageUtil.h"

using namespace std;
namespace fs = std::filesystem;

namespace Wxiv
{
    void copyImageNameOrPathToClipboard(std::shared_ptr<WxivImage> image, bool doName, bool doLinux)
    {
        wxFileName path = image->getPath();

        if (image && path.IsOk())
        {
            wxString s = doName ? path.GetFullName() : path.GetFullPath();

            if (doLinux)
            {
                s.Replace("\\", "/");
            }

            if (!s.empty())
            {
                if (wxTheClipboard->Open())
                {
                    wxTheClipboard->SetData(new wxTextDataObject(s));
                    wxTheClipboard->Flush();
                    wxTheClipboard->Close();
                }
                else
                {
                    showAlertDialog("Unable to open the clipboard.");
                }
            }
            else
            {
                showAlertDialog("Skip copying empty string to clipboard.");
            }
        }
        else
        {
            showAlertDialog("No image provided to copy name or path to clipboard.");
        }
    }

    /**
     * Search for a data file used by this app (e.g. help text file, icon file), try to cover both
     * development and installed scenarios on all platforms, for convenience.
     * This returns empty string for not found.
     */
    wxFileName findInstalledFile(wxString basename)
    {
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();

        if (exePath.empty())
        {
            return wxFileName();
        }

        wxFileName exeDir = wxFileName(exePath).GetPath();

        // Want this to work when running during development and also as installed on target machine,
        // so search various paths.
        // (regarding /usr/share, I am not sure how to know if someone installs to a different location,
        // and it is not easy to find the path to the executable to use rel path from there, so just
        // hardcode for now)
        vector<wxString> relDirs = {
#if defined(_DEBUG) || defined(DEBUG)
            ".",
            "../doc",            // mac dev, vscode
            "../icons",          // mac dev, vscode
            "../../doc",         // linux dev, vscode
            "../../icons",       // linux dev, vscode
            "../../../../doc",   // windows dev, from VS build out location
            "../../../../icons", // windows dev, from VS build out location
#endif
#if defined(__APPLE__)
            "../Resources", // mac installed location
#endif
#if defined(_WIN32) || defined(__linux__)
            "../share", // windows and linux installed location
#endif
        };

        wxString exeDirStr = exeDir.GetFullPath();
        wxString delim(L"/");

        for (const wxString& relDir : relDirs)
        {
            // wxFileName cannot append relative path
            wxString pathStr = exeDir.GetFullPath() + delim + relDir + delim + basename;
            wxFileName path(pathStr);

            if (path.IsOk() && path.FileExists())
            {
                return path;
            }
        }

        return wxFileName();
    }

    bool checkIsOnlyAscii(const wxString& s)
    {
        for (size_t i = 0; i < s.size(); i++)
        {
            if ((int)s[i] > 127)
                return false;
        }

        return true;
    }

    /**
     * @brief If it's already ascii just toStdString, else convert to utf-8 string.
     * @param s
     * @return
     */
    std::string toNativeString(const wxString& s)
    {
        if (checkIsOnlyAscii(s))
        {
            return s.ToStdString();
        }
        else
        {
            return std::string(s.mb_str(wxConvUTF8));
        }
    }

    /**
     * @brief wxWidgets to load image in cross-platform way. For paths with non-ASCII characters this can only load a single page
     * from multi-page tif's.
     * @param path
     * @param mats
     * @return
     */
    bool wxLoadImage(const wxString& path, vector<cv::Mat>& mats)
    {
        bool result = false;

        // extension has to be ascii
        wxFileName fn(path);

        if (!fn.HasExt())
        {
            return false;
        }

        wxString wxExt = fn.GetExt();

        if (!checkIsOnlyAscii(wxExt))
        {
            throw runtime_error("Image files must have known extensions and those must be ASCII.");
        }

        if (checkIsOnlyAscii(path))
        {
            // ASCII path, can use opencv
            result = imreadmulti(path.ToStdString(), mats, cv::IMREAD_UNCHANGED);
        }
        else
        {
            wxFile file(path, wxFile::read);

            if (file.IsOpened())
            {
                // get size
                file.SeekEnd();
                size_t n = file.Tell();
                file.Seek(0); // back to start

                vector<uchar> buffer(n);
                size_t nRead = file.Read(buffer.data(), n);

                if ((nRead == n) && !file.Error() && file.Eof())
                {
                    file.Close();

                    cv::Mat img;
                    cv::imdecode(buffer, cv::IMREAD_UNCHANGED, &img);

                    if (!img.empty())
                    {
                        mats.push_back(img);
                        result = true;
                    }
                }
            }
        }

        if (result && !mats.empty())
        {
            string ext = wxExt.ToStdString();

            for (cv::Mat& img : mats)
            {
                ImageUtil::convertAfterLoad(img, ext, img);
            }
        }

        return result;
    }

    /**
     * @brief Save image to specified path.
     * @param doShowErrorDialog If true (default) then show an error dialog on failure.
     * @return Whether the image was successfully saved or not.
     */
    bool wxSaveImage(const wxString& path, cv::Mat& img, bool doShowErrorDialog)
    {
        bool result = false;

        // encode
        vector<uchar> buffer;
        wxFileName wxName(path);
        string ext = toNativeString(wxName.GetExt());

        // opencv wants leading period
        ext = "." + ext;

        // maybe have to convert for save
        cv::Mat converted;
        ImageUtil::convertForSave(img, ext, converted);

        try
        {
            std::vector<int> params;

            if (ext == ".tif" || ext == ".tiff")
            {
                params.push_back(cv::IMWRITE_TIFF_COMPRESSION);
                params.push_back(5); // 5 = LZW (lossless, and appears to be the default), 1 = no compression
            }

            result = cv::imencode(ext, converted, buffer, params);
        }
        catch (cv::Exception& ex)
        {
            if (doShowErrorDialog)
            {
                showAlertDialog(fmt::format("Error encoding image to target type:\n\n{}", ex.what()));
            }

            result = false;
        }
        catch (...)
        {
            if (doShowErrorDialog)
            {
                showAlertDialog("Unknown exception encoding image to target type.");
            }

            result = false;
        }

        if (result)
        {
            try
            {
                result = writeFile(path, buffer);
            }
            catch (std::exception& ex)
            {
                if (doShowErrorDialog)
                {
                    showAlertDialog(fmt::format("Error writing file:\n\n{}", ex.what()));
                }

                result = false;
            }
            catch (...)
            {
                if (doShowErrorDialog)
                {
                    showAlertDialog("Unknown exception writing file.");
                }

                result = false;
            }
        }

        return result;
    }

    void saveCollageSpecToConfig(const ImageUtil::CollageSpec& spec)
    {
        auto config = wxConfigBase::Get();
        config->Write("colCount", spec.colCount);
        config->Write("imageWidthPx", spec.imageWidthPx);
        config->Write("marginPx", spec.marginPx);
        config->Write("fontFace", spec.fontFace);
        config->Write("fontScale", spec.fontScale);
        config->Write("doBlackBackground", spec.doBlackBackground);
        config->Write("doCaptions", spec.doCaptions);
    }

    void loadCollageSpecFromConfig(ImageUtil::CollageSpec& spec)
    {
        auto config = wxConfigBase::Get();
        config->Read("colCount", &spec.colCount, spec.colCount);
        config->Read("imageWidthPx", &spec.imageWidthPx, spec.imageWidthPx);
        config->Read("marginPx", &spec.marginPx, spec.marginPx);
        config->Read("fontFace", &spec.fontFace, spec.fontFace);
        config->Read("fontScale", &spec.fontScale, spec.fontScale);
        config->Read("doBlackBackground", &spec.doBlackBackground, spec.doBlackBackground);
        config->Read("doCaptions", &spec.doCaptions, spec.doCaptions);
    }

    vector<wxString> listFilesInDir(const wxString& dirPath)
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
            paths.push_back(dirPath + "/" + name);
            cont = dir.GetNext(&name);
        }

        std::sort(paths.begin(), paths.end(), [](const wxString& a, const wxString& b) -> bool { return a.compare(b) < 0; });
        return paths;
    }
}
