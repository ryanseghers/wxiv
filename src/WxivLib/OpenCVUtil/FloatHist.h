// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <vector>
#include <opencv2/opencv.hpp>

namespace Wxiv
{
    /**
     * @brief Contains a uniform histogram on floats.
     * Bin size is computed from minVal, maxVal, and count of bins.
     */
    struct FloatHist
    {
        /**
         * @brief Bottom of the first bin.
         */
        float minVal = 0.0f;

        /**
         * @brief Either a specified max value to include in the hist or found max plus epsilon.
         */
        float maxVal = 0.0f;

        /**
         * @brief Bottoms of each bin. Top is open-ended.
         * This floating point regardless of original image type.
         */
        std::vector<float> bins;

        /**
         * @brief Counts per bin.
         */
        std::vector<int> counts;

        bool empty();
        void clear();
        int getBinCount();
        float getBinSize();
        float getMin();
        float getMax();
        void copy(FloatHist& other);

        void compute(const std::vector<float>& values, int binCount, float minVal = NAN, float maxVal = NAN);
    };
}
