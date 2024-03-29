// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <string>

#include <opencv2/opencv.hpp>

#include "WxivImage.h"
#include "ArrowUtil.h"
#include "MiscUtil.h"
#include "VectorUtil.h"

using namespace std;

namespace Wxiv
{
    /**
     * @brief Create a test table with some random shapes.
     */
    std::shared_ptr<arrow::Table> buildTestShapesTable(int rows, int cols, int shapeCount, bool doStringColors, bool doOnlyRects = false)
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
        int nPoints = shapeCount / 3;

        if (!doOnlyRects)
        {
            vectorAppend(types, vector<int>(nPoints, (int)ShapeType::Point));
            vectorAppend(xs, vectorRandomFloat(seed++, nPoints, border, cols - border));
            vectorAppend(ys, vectorRandomFloat(seed++, nPoints, border, rows - border));
            vectorAppend(dim1, vector<float>(nPoints, 1.0f));
            vectorAppend(dim2, vector<float>(nPoints, 2.0f));
            vectorAppend(colorStrings, vector<string>(nPoints, "0xFF00FF"));
            vectorAppend(colorInts, vectorRandomInt(seed++, nPoints, 0, 0xFFFFFF));
            vectorAppend(thicknesses, vector<int>(nPoints, 2));
            vectorAppend(score1, vectorGen<float>(nPoints, [&](int idx) { return scoresDistrib(prng); }));
            vectorAppend(score2, vectorGen<float>(nPoints, [&](int idx) { return scoresDistrib(prng); }));
        }

        // rects
        int nRects = doOnlyRects ? shapeCount : nPoints;
        vectorAppend(types, vector<int>(nRects, (int)ShapeType::Rect));
        vectorAppend(xs, vectorRandomFloat(seed++, nRects, border, cols - border));
        vectorAppend(ys, vectorRandomFloat(seed++, nRects, border, rows - border));

        // for doOnlyRects do fixed-size rects
        if (doOnlyRects)
        {
            float rectDim = 5.0f;
            vectorAppend(dim1, vector<float>(nRects, rectDim));
            vectorAppend(dim2, vector<float>(nRects, rectDim));
            vectorAppend(colorInts, vector<int>(nRects, 0xFFFFFF));
        }
        else
        {
            vectorAppend(dim1, vectorRandomFloat(seed++, nRects, 5, 20));
            vectorAppend(dim2, vectorRandomFloat(seed++, nRects, 5, 20));
            vectorAppend(colorInts, vectorRandomInt(seed++, nRects, 0, 0xFFFFFF));
        }

        vectorAppend(colorStrings, vector<string>(nRects, "0x00FFFF"));
        vectorAppend(thicknesses, vector<int>(nRects, 2));
        vectorAppend(score1, vectorGen<float>(nRects, [&](int idx) { return scoresDistrib(prng); }));
        vectorAppend(score2, vectorGen<float>(nRects, [&](int idx) { return scoresDistrib(prng); }));

        // circles
        if (!doOnlyRects)
        {
            int nCircles = shapeCount - nPoints - nRects;
            vectorAppend(types, vector<int>(nCircles, (int)ShapeType::Circle));
            vectorAppend(xs, vectorRandomFloat(seed++, nCircles, border, cols - border));
            vectorAppend(ys, vectorRandomFloat(seed++, nCircles, border, rows - border));
            vectorAppend(dim1, vectorRandomFloat(seed++, nCircles, 5, 20));
            vectorAppend(dim2, vectorRandomFloat(seed++, nCircles, 5, 20));
            vectorAppend(colorStrings, vector<string>(nCircles, "0x0000FF"));
            vectorAppend(colorInts, vectorRandomInt(seed++, nCircles, 0, 0xFFFFFF));
            vectorAppend(thicknesses, vector<int>(nCircles, 2));
            vectorAppend(score1, vectorGen<float>(nCircles, [&](int idx) { return scoresDistrib(prng); }));
            vectorAppend(score2, vectorGen<float>(nCircles, [&](int idx) { return scoresDistrib(prng); }));
        }

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
     * @brief Create a test image (with shapes) with random bright spots.
     */
    std::shared_ptr<WxivImage> buildTestImageBrightSpots(int rows, int cols, int spotCount, int prngSeed)
    {
        // image with noise
        cv::Mat img = cv::Mat::zeros(rows, cols, CV_8UC1);
        img = 10;
        cv::randn(img, 50, 10);

        // WxivImage
        std::shared_ptr<WxivImage> image = std::make_shared<WxivImage>(img);

        // shapes
        ShapeSet& shapes = image->getShapes();
        shapes.ptable = buildTestShapesTable(rows, cols, spotCount, false, true);

        // render spots by adding gaussian kernel at each spot location
        std::vector<float> xs, ys;
        ArrowUtil::getFloatValues(shapes.ptable->GetColumnByName("x"), xs);
        ArrowUtil::getFloatValues(shapes.ptable->GetColumnByName("y"), ys);

        int kernelSize = 5;
        float sigma = 1.0;
        float magnitude = 1000;
        cv::Mat kernel = ImageUtil::generateGaussianKernel(kernelSize, sigma);
        kernel *= magnitude;

        for (int i = 0; i < xs.size(); i++)
        {
            ImageUtil::addKernelToImage(img, kernel, lroundf(xs[i]), lroundf(ys[i]));
        }

        return image;
    }
}
