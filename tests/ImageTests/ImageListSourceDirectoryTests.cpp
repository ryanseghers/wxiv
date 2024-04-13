#include <gtest/gtest.h>
#include <string>

#include <opencv2/opencv.hpp>

#include "WxWidgetsUtil.h"
#include "WxivUtil.h"
#include "WxivImageUtil.h"
#include "ArrowUtil.h"
#include "VectorUtil.h"
#include "TempFile.h"

using namespace std;
using namespace Wxiv;

namespace WxivTests
{
    // void createTempImage(const wxString& path, int imageDim)
    //{
    //    cv::Mat img(imageDim, imageDim, CV_8U);
    //    img = 10;
    //    cv::randn(img, 50, 10);
    //    wxSaveImage(path, img);
    //}

    ///**
    //* @brief Save a test image/shape file pair.
    //* @return The generated neighbor shapes file path.
    //*/
    // wxFileName saveTestImageShapesFiles(const wxString& path, int shapeCount, bool doStringColors, bool doParquet)
    //{
    //    int imageDim = 512;
    //    createTempImage(path, imageDim);

    //    // some shapes
    //    wxFileName shapesPath(path);
    //    shapesPath.SetExt(doParquet ? "parquet" : "geo.csv");

    //    auto ptable = buildTestShapesTable(imageDim, imageDim, shapeCount, doStringColors, false);
    //    ArrowUtil::saveFile(toNativeString(shapesPath.GetFullPath()), ptable);

    //    return shapesPath;
    //}

    // TEST(ImageListDirectorySourceTests, testLoad)
    //{
    //    int shapeCount = 20;

    //    for (bool doUnicodeName : {false, true})
    //    {
    //        for (bool doStringColors : {false, true})
    //        {
    //            for (bool doParquet : {false, true})
    //            {
    //                // save image and shapes
    //                TempFile tempFile(doUnicodeName ? L"WxivImageTests-unicΩode" : L"WxivImageTests", "tif");
    //                wxFileName neighborShapesPath = saveTestImageShapesFiles(tempFile.GetFullPath(), shapeCount, doStringColors, doParquet);

    //                // load
    //                WxivImage image(tempFile.GetFullPath());
    //                EXPECT_TRUE(image.load());

    //                // checks
    //                EXPECT_FALSE(image.empty());

    //                ShapeSet& shapes = image.getShapes();
    //                EXPECT_EQ(shapeCount, shapes.size());

    //                if (neighborShapesPath.FileExists())
    //                {
    //                    wxRemoveFile(neighborShapesPath.GetFullPath());
    //                }
    //            }
    //        }
    //    }
    //}

    // TEST(WxivImageTests, testBadNeighborFile)
    //{
    //    TempFile tempImage(L"WxivImageTests", "tif");
    //    createTempImage(tempImage.GetFullPath(), 128);

    //    for (bool doParquet : {false, true})
    //    {
    //        wxFileName shapesPath(tempImage.GetFullPath());
    //        shapesPath.SetExt(doParquet ? "parquet" : "geo.csv");

    //        // write garbage to file
    //        vector<uchar> buffer = {1, 2, 3, 4, 5};
    //        writeFile(shapesPath.GetFullPath(), buffer);

    //        // load
    //        WxivImage image(tempImage.GetFullPath());
    //        EXPECT_TRUE(image.load());
    //        EXPECT_TRUE(image.checkIsShapeSetLoadError());

    //        // cleanup
    //        if (shapesPath.FileExists())
    //        {
    //            wxRemoveFile(shapesPath.GetFullPath());
    //        }
    //    }
    //}
}
