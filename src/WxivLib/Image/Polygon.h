// Copyright(c) 2023 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <vector>
#include <unordered_map>
#include <memory>

#include <opencv2/opencv.hpp>

namespace Wxiv
{
    struct Polygon
    {
        std::vector<cv::Point2f> points;
        cv::Scalar colorRgb;
        int pointDim;
        int lineThickness;
        std::unordered_map<std::string, std::string> metadata;
    };
}