// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include "FloatHist.h"

namespace Wxiv
{
    namespace ImageUtil
    {
        /**
         * @brief Stats over an image or ROI of an image.
         */
        struct ImageStats
        {
            int type = 0;
            int width = 0;
            int height = 0;
            int nonzeroCount = 0;
            float sum = 0.0f;
            float minVal = 0.0f;
            float maxVal = 0.0f;

            bool empty()
            {
                return width <= 0;
            }
        };

        bool convertAfterLoad(cv::Mat& img, const std::string& ext, cv::Mat& dst);
        bool convertForSave(cv::Mat& img, const std::string& ext, cv::Mat& dst);

        std::vector<int> histInt(cv::Mat& img);
        std::vector<int> histInt(cv::Mat& img, int binShift);
        FloatHist histFloat(cv::Mat& img, int binCount, float minVal, float maxVal);
        void histFloat(cv::Mat& img, int binCount, float minVal, float maxVal, FloatHist& hist);
        void histFloat(cv::Mat& img, int binCount, float& minVal, float& maxVal, std::vector<float>& bins, std::vector<int>& hist);

        std::pair<int, int> histPercentilesInt(cv::Mat& img, float lowPct, float highPct);
        std::pair<float, float> histPercentiles32f(cv::Mat& img, float lowPct, float highPct);
        std::pair<float, float> histPercentiles(cv::Mat& img, float lowPct, float highPct);

        std::string getImageTypeString(int type);
        std::string getImageTypeString(cv::Mat& img);
        std::string getImageDescString(cv::Mat& img);
        std::string getPixelValueString(cv::Mat& img, cv::Point2i pt);

        void imgTo8u(cv::Mat& img, cv::Mat& dst, float lowVal = 0.0f, float highVal = 0.0f);
        void imgToRgb(cv::Mat& img8u, uint8_t* dst);
        ImageStats computeStats(cv::Mat& img);

        bool checkSupportedExtension(const std::string& ext);
    }
}
