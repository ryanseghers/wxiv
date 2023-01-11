#include <string>
#include <vector>
#include <gtest/gtest.h>

#include <opencv2/opencv.hpp>

#include "WxivUtil.h"
#include <wx/stdpaths.h>
#include "TempFile.h"

using namespace std;
using namespace Wxiv;

namespace WxivTests
{
    /**
     * @brief Test save image to temp dir and then load it again.
     */
    TEST(WxivUtilTests, testSaveAndLoadImage)
    {
        cv::Mat img(16, 16, CV_16U);
        img = 123;
        TempFile tempFile("WxivUtilTests", "tif");
        EXPECT_TRUE(wxSaveImage(tempFile.GetFullPath(), img));

        vector<cv::Mat> rtImages;
        EXPECT_TRUE(wxLoadImage(tempFile.GetFullPath(), rtImages));

        // basic checks: not trying to test opencv
        EXPECT_EQ(1, rtImages.size());
        EXPECT_EQ(CV_16U, rtImages[0].type());
        EXPECT_EQ(img.rows, rtImages[0].rows);
        EXPECT_EQ(img.cols, rtImages[0].cols);
    }

    /**
     * @brief Test save image to temp dir and then load it again, with non-ascii chars in the file name.
     */
    TEST(WxivUtilTests, testSaveAndLoadImageNonAscii)
    {
        cv::Mat img(16, 16, CV_16U);
        img = 123;

        TempFile tempFile(L"WxivUtilTests-unicΩode", "tif");
        EXPECT_TRUE(wxSaveImage(tempFile.GetFullPath(), img));

        vector<cv::Mat> rtImages;
        EXPECT_TRUE(wxLoadImage(tempFile.GetFullPath(), rtImages));

        // basic checks: not trying to test opencv
        EXPECT_EQ(1, rtImages.size());
        EXPECT_EQ(CV_16U, rtImages[0].type());
        EXPECT_EQ(img.rows, rtImages[0].rows);
        EXPECT_EQ(img.cols, rtImages[0].cols);
    }
}
