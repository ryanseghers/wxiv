#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <fmt/core.h>

#include <opencv2/opencv.hpp>

#include "WxivUtil.h"
#include <wx/stdpaths.h>
#include "TempFile.h"

using namespace std;
using namespace Wxiv;

namespace WxivTests
{
    /**
     * @brief Find test images dir relative to cwd of test process.
     * This throws for not found.
     */
    wxString findTestDataDir()
    {
        // this is going to be different based on various things that are out of my control
        vector<wxString> dirPaths = {
            "../../../../tests/data/images" // VS on Windows
        };
        wxString dirPath;

        for (auto& thisDirPath : dirPaths)
        {
            wxFileName fn(thisDirPath);

            if (fn.DirExists())
            {
                dirPath = thisDirPath;
                break;
            }
        }

        if (dirPath.empty())
        {
            auto cwd = wxFileName::GetCwd();
            throw std::runtime_error("Failed to find test data images dir relative to: " + cwd);
        }

        return dirPath;
    }

    void loadTestDataImage(wxString fileName, cv::Mat& img)
    {
        wxString inputDir = findTestDataDir();
        wxString inputPath = inputDir + "/" + fileName;
        vector<cv::Mat> images;
        EXPECT_TRUE(wxLoadImage(inputPath, images));
        EXPECT_EQ(images.size(), 1);
        img = images[0];
    }

    bool trySaveFile(wxString fileName, string ext, cv::Mat& img)
    {
        bool result = false;
        wxString tmpDir;

        if (wxGetEnv("WXIV_TMP_DIR", &tmpDir))
        {
            wxString fileBaseName = wxFileName(fileName).GetName();
            result = wxSaveImage(tmpDir + "/" + fileBaseName + "." + ext, img, false);
        }
        else
        {
            TempFile tempFile("WxWidgetsUtilTests", ext);
            result = wxSaveImage(tempFile.GetFullPath(), img, false);
        }

        return result;
    }

    /**
     * @brief Load several input color images and then save to png.
     * Unless the env var is set, this is just checking no unexpected exceptions.
     * If the env var is set, then you can manually inspect the output images (viewing them with something other than wxiv!)
     */
    TEST(WxWidgetsUtilTests, testReadImageFileFormats)
    {
        vector<wxString> inputFileNames = {"colors-png.png", "colors-png-argb.png", "colors-tif.tif", "colors-jpg.jpg"};

        for (auto& fileName : inputFileNames)
        {
            cv::Mat img;
            loadTestDataImage(fileName, img);

            // write
            string outputExt = "png";
            wxString fileBaseName = wxFileName(fileName).GetName();
            string inputExt = wxFileName(fileName).GetExt().ToStdString();
            string outputFileName = fmt::format("testRead_{}_{}_to_{}", fileBaseName.ToStdString(), inputExt, outputExt);
            EXPECT_TRUE(trySaveFile(outputFileName, outputExt, img));
        }
    }

    /**
     * @brief Load a color png and then save to all kinds of output files.
     *   Unless the env var is set, this is just checking no unexpected exceptions because this doesn't do any
     *   verification of the output file.
     *   If the env var is set, then you can manually inspect the output images (viewing them with something other than wxiv!)
     */
    TEST(WxWidgetsUtilTests, testWriteImageFileFormats)
    {
        vector<wxString> inputFileNames = {"colors-png.png"};
        vector<string> outputExts = ImageUtil::getAllExtensions();
        int successCount = 0;

        for (auto& fileName : inputFileNames)
        {
            string inputExt = wxFileName(fileName).GetExt().ToStdString();

            cv::Mat img;
            loadTestDataImage(fileName, img);

            // write
            for (auto& outputExt : outputExts)
            {
                string outputFileName = fmt::format("testWrite_{}_to_{}", inputExt, outputExt);

                if (trySaveFile(outputFileName, outputExt, img))
                {
                    fmt::print("Saving to {} worked.", outputExt);
                    successCount++;
                }
                else
                {
                    fmt::print("Saving to {} failed", outputExt);
                }
            }
        }

        // should be some minimum number that succeed across all platforms and available libraries
        EXPECT_GT(successCount, 5);
    }
}
