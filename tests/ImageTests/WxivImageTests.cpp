#include <gtest/gtest.h>
#include <string>

#include <opencv2/opencv.hpp>

#include "WxWidgetsUtil.h"
#include "WxivUtil.h"
#include "ArrowUtil.h"
#include "VectorUtil.h"
#include "TempFile.h"

using namespace std;
using namespace Wxiv;

namespace WxivTests
{
    /**
     * @brief Create a test table with some random shapes.
     */
    std::shared_ptr<arrow::Table> buildShapesTable(int imageDim, int nRows, bool doStringColors)
    {
        int seed = 1;    // repeatable prng
        int border = 32; // avoid shapes off edge
        auto prng = getPrng(seed);
        std::normal_distribution<float> scoresDistrib(10.0f, 2.0f);

        vector<int> types, thicknesses;
        vector<float> xs, ys, dim1, dim2;
        vector<string> colorStrings;
        vector<int> colorInts;
        vector<float> score1, score2;

        // points
        int nPoints = nRows / 3;
        vectorAppend(types, vector<int>(nPoints, (int)ShapeType::Point));
        vectorAppend(xs, vectorRandomFloat(seed++, nPoints, border, imageDim - border));
        vectorAppend(ys, vectorRandomFloat(seed++, nPoints, border, imageDim - border));
        vectorAppend(dim1, vector<float>(nPoints, 1.0f));
        vectorAppend(dim2, vector<float>(nPoints, 2.0f));
        vectorAppend(colorStrings, vector<string>(nPoints, "0xFF00FF"));
        vectorAppend(colorInts, vectorRandomInt(seed++, nPoints, 0, 0xFFFFFF));
        vectorAppend(thicknesses, vector<int>(nPoints, 2));
        vectorAppend(score1, vectorGen<float>(nPoints, [&](int idx) { return scoresDistrib(prng); }));
        vectorAppend(score2, vectorGen<float>(nPoints, [&](int idx) { return scoresDistrib(prng); }));

        // rects
        int nRects = nPoints;
        vectorAppend(types, vector<int>(nRects, (int)ShapeType::Rect));
        vectorAppend(xs, vectorRandomFloat(seed++, nRects, border, imageDim - border));
        vectorAppend(ys, vectorRandomFloat(seed++, nRects, border, imageDim - border));
        vectorAppend(dim1, vectorRandomFloat(seed++, nRects, 5, 20));
        vectorAppend(dim2, vectorRandomFloat(seed++, nRects, 5, 20));
        vectorAppend(colorStrings, vector<string>(nRects, "0x00FFFF"));
        vectorAppend(colorInts, vectorRandomInt(seed++, nRects, 0, 0xFFFFFF));
        vectorAppend(thicknesses, vector<int>(nRects, 2));
        vectorAppend(score1, vectorGen<float>(nRects, [&](int idx) { return scoresDistrib(prng); }));
        vectorAppend(score2, vectorGen<float>(nRects, [&](int idx) { return scoresDistrib(prng); }));

        // circles
        int nCircles = nRows - nPoints - nRects;
        vectorAppend(types, vector<int>(nCircles, (int)ShapeType::Circle));
        vectorAppend(xs, vectorRandomFloat(seed++, nCircles, border, imageDim - border));
        vectorAppend(ys, vectorRandomFloat(seed++, nCircles, border, imageDim - border));
        vectorAppend(dim1, vectorRandomFloat(seed++, nCircles, 5, 20));
        vectorAppend(dim2, vectorRandomFloat(seed++, nCircles, 5, 20));
        vectorAppend(colorStrings, vector<string>(nCircles, "0x0000FF"));
        vectorAppend(colorInts, vectorRandomInt(seed++, nCircles, 0, 0xFFFFFF));
        vectorAppend(thicknesses, vector<int>(nCircles, 2));
        vectorAppend(score1, vectorGen<float>(nCircles, [&](int idx) { return scoresDistrib(prng); }));
        vectorAppend(score2, vectorGen<float>(nCircles, [&](int idx) { return scoresDistrib(prng); }));

        // save
        auto typeArray = ArrowUtil::buildInt32Array(types);
        auto xArray = ArrowUtil::buildFloatArray(xs);
        auto yArray = ArrowUtil::buildFloatArray(ys);
        auto dim1Array = ArrowUtil::buildFloatArray(dim1);
        auto dim2Array = ArrowUtil::buildFloatArray(dim2);
        auto colorArray = doStringColors ? ArrowUtil::buildStringArray(colorStrings) : ArrowUtil::buildInt32Array(colorInts);
        auto thicknessArray = ArrowUtil::buildInt32Array(thicknesses);
        auto score1Array = ArrowUtil::buildFloatArray(score1);
        auto score2Array = ArrowUtil::buildFloatArray(score2);

        // schema
        auto schema = arrow::schema({arrow::field("type", arrow::int32()), arrow::field("x", arrow::float32()), arrow::field("y", arrow::float32()),
            arrow::field("dim1", arrow::float32()), arrow::field("dim2", arrow::float32()),
            arrow::field("color", doStringColors ? arrow::utf8() : arrow::int32()), arrow::field("thickness", arrow::int32()),
            arrow::field("score1", arrow::float32()), arrow::field("score2", arrow::float32())});

        arrow::ArrayVector arrayVector;
        arrayVector.push_back(typeArray);
        arrayVector.push_back(xArray);
        arrayVector.push_back(yArray);
        arrayVector.push_back(dim1Array);
        arrayVector.push_back(dim2Array);
        arrayVector.push_back(colorArray);
        arrayVector.push_back(thicknessArray);
        arrayVector.push_back(score1Array);
        arrayVector.push_back(score2Array);

        std::shared_ptr<arrow::Table> table = arrow::Table::Make(schema, arrayVector);

        return table;
    }

    /**
     * @brief Save a test image/shape file pair.
     * @return The generated neighbor shapes file path.
     */
    wxFileName saveTestImageShapesFiles(const wxString& path, int nRows, bool doStringColors, bool doParquet)
    {
        int imageDim = 512;
        cv::Mat img(imageDim, imageDim, CV_8U);
        img = 10;
        cv::randn(img, 50, 10);
        wxSaveImage(path, img);

        // some shapes
        wxFileName shapesPath(path);
        shapesPath.SetExt(doParquet ? "parquet" : "geo.csv");

        auto ptable = buildShapesTable(imageDim, nRows, doStringColors);
        ArrowUtil::saveFile(toNativeString(shapesPath.GetFullPath()), ptable);

        return shapesPath;
    }

    /**
     * @brief Temporarily using this to leverage my test function to save images and shapes for manual testing.
     */
    TEST(WxImageTests, saveExampleTifAndShapesFiles)
    {
        // using this to create a test file for manual testing, too
        int nRows = 50;

#ifdef __linux__
        wxString path("/tmp/WxImageTests.tif");
        wxString jpegPath("/tmp/WxImageTests-csv.jpeg");
#elif _WIN32
        wxString path("C:/Temp/WxImageTests-parquet.jpeg");
        wxString jpegPath("C:/Temp/WxImageTests-csv.jpeg");
#elif __APPLE__
        wxString path("/tmp/WxImageTests.tif");
        wxString jpegPath("/tmp/WxImageTests-csv.jpeg");
#endif

        bool doStringColors = false;
        bool doParquet = true;
        saveTestImageShapesFiles(path, nRows, doStringColors, doParquet);

        // also save to jpeg and csv
        doParquet = false;
        saveTestImageShapesFiles(jpegPath, nRows, doStringColors, doParquet);
    }

    TEST(WxImageTests, testLoad)
    {
        int nRows = 20;

        for (bool doUnicodeName : {false, true})
        {
            for (bool doStringColors : {false, true})
            {
                for (bool doParquet : {false, true})
                {
                    // save image and shapes
                    TempFile tempFile(doUnicodeName ? L"WxImageTests-unicΩode" : L"WxImageTests", "tif");
                    wxFileName neighborShapesPath = saveTestImageShapesFiles(tempFile.GetFullPath(), nRows, doStringColors, doParquet);

                    // load
                    WxivImage image(tempFile.GetFullPath());
                    EXPECT_TRUE(image.load());

                    // checks
                    EXPECT_FALSE(image.empty());

                    ShapeSet& shapes = image.getShapes();
                    EXPECT_EQ(nRows, shapes.size());

                    if (neighborShapesPath.FileExists())
                    {
                        wxRemoveFile(neighborShapesPath.GetFullPath());
                    }
                }
            }
        }
    }
}
