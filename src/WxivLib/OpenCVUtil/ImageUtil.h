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

        /**
         * @brief Parameters to specify how to render a list of images in rows and columns into a single larger image,
         * with a caption for each image.
        */
        struct CollageSpec
        {
            /**
             * @brief Number of columns of images.
             * The number of rows is computed from the total number of images.
            */
            int colCount = 4;

            /**
             * @brief Output image width (of the large single image into which all of the input images have been rendered).
             * The height is computed from the number of columns, scaling, and this width.
            */
            int imageWidthPx = 2048;

            /**
             * @brief The margin between images and also between the edge images and the edge of the output image.
            */
            int marginPx = 16;

            /**
             * @brief OpenCV font face.
            */
            int fontFace = cv::FONT_HERSHEY_SIMPLEX;

            /**
             * @brief OpenCV font scale. This is multipled by the base font size to produce the final font size.
             * I arbitrarily chose this default from a single scenario.
            */
            double fontScale = 0.6f;

            /**
             * @brief Background can be white or black. Default is white.
            */
            bool doBlackBackground = false;
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

        cv::Mat generateGaussianKernel(int ksize, float sigma);
        void addKernelToImage(cv::Mat& image, const cv::Mat& kernel, int x, int y);

        bool checkSupportedExtension(const std::string& ext);
        void renderCollage(const std::vector<cv::Mat>& images, const std::vector<std::string>& captions, const CollageSpec& spec, cv::Mat& dst);
    }
}
