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
    std::shared_ptr<arrow::Table> buildTestShapesTable(int rows, int cols, int shapeCount, bool doStringColors, bool doOnlyRects);
    std::shared_ptr<WxivImage> buildTestImageBrightSpots(int rows, int cols, int spotCount, int prngSeed);
}
