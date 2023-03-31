#include <gtest/gtest.h>
#include <string>
#include <fmt/core.h>

#include <opencv2/opencv.hpp>

#include "ImageUtil.h"
#include "WxivUtil.h"
#include "VectorUtil.h"

using namespace std;
using namespace Wxiv;

namespace WxivTests
{
    /**
     * @brief Just a placeholder, getting these tests started, do not think this is worth a unit test.
     */
    TEST(ImageUtilTests, testGetImageTypeString)
    {
        cv::Mat img(16, 16, CV_8U);
        img = 123;
        EXPECT_EQ(ImageUtil::getImageTypeString(img), "8U");
    }

    cv::Mat generateBrightSpotImage(int rows, int cols, int spotCount, int prngSeed)
    {
        cv::Mat img = cv::Mat::zeros(rows, cols, CV_8UC1);
        img = 10;
        cv::randn(img, 50, 10);

        // render spots by adding gaussian kernel at each spot location
        std::vector<float> xs = vectorRandomFloat(prngSeed, spotCount, 0.0f, (float)cols);
        std::vector<float> ys = vectorRandomFloat(prngSeed + 1, spotCount, 0.0f, (float)rows);

        int kernelSize = 5;
        float sigma = 1.0;
        float magnitude = 1000;
        cv::Mat kernel = ImageUtil::generateGaussianKernel(kernelSize, sigma);
        kernel *= magnitude;

        for (int i = 0; i < spotCount; i++)
        {
            ImageUtil::addKernelToImage(img, kernel, lroundf(xs[i]), lroundf(ys[i]));
        }

        return img;
    }

    /**
     * @brief This only inspects the output image size, so is a pretty shallow test.
     * But also saves it, depending on env var, for manual inspection.
     */
    TEST(ImageUtilTests, testRenderCollage)
    {
        const int imgRows = 256;
        const int imgCols = 288;

        std::vector<cv::Mat> images;
        std::vector<std::string> captions;
        ImageUtil::CollageSpec spec;

        const int imageCount = 7;

        for (int i = 0; i < imageCount; i++)
        {
            // one diff size image
            int thisImageWidth = imgCols;
            int thisImageHeight = imgRows;

            if (i == imageCount - 1)
            {
                thisImageWidth += 128;
            }

            cv::Mat img = generateBrightSpotImage(thisImageHeight, thisImageWidth, 1000, i);
            images.push_back(img);
            captions.push_back(fmt::format("This is image {} and this text is really long to check handling of that case", i));
        }

        cv::Mat collage;
        renderCollage(images, captions, spec, collage);

        EXPECT_EQ(spec.imageWidthPx, collage.cols);

        if (wxGetEnv("WXIV_TESTS_SAVE_TEMP_IMAGES", nullptr))
        {
            wxSaveImage("C:/Temp/ImageUtilTests-collage.png", collage);
        }
    }
}
