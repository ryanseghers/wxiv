#include <gtest/gtest.h>
#include <string>

#include <opencv2/opencv.hpp>

#include "ImageUtil.h"

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
}
