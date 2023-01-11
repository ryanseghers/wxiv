// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once

#include <string>
#include <cmath>
#include <vector>

namespace Wxiv
{
    int findPercentileInHist(const std::vector<int>& hist, float pct);
    void downsampleHist(
        const std::vector<int>& hist, const std::vector<float>& bins, int newBinCount, std::vector<float>& newBins, std::vector<int>& newHist);
    bool checkClose(float a, float b, float rtol = 1e-5f, float atol = 1e-8f);
    bool checkClose(double a, double b, double rtol = 1e-5, double atol = 1e-8);

    /**
     * @brief Find min and max values of input vector. Ignores nan and inf values.
     * If there are no good values this returns false;
     * @param maxVal Output.
     * @return true for found any good values and thus set min and max, else false.
     */
    template <typename T> bool vectorMinMax(const std::vector<T>& v, T& minVal, T& maxVal)
    {
        minVal = std::numeric_limits<T>::max();
        maxVal = std::numeric_limits<T>::min();
        bool isAnyGood = false;
        size_t n = v.size();

        for (int i = 0; i < n; i++)
        {
            T val = v[i];

            if (std::isfinite(val))
            {
                minVal = std::min(minVal, val);
                maxVal = std::max(maxVal, val);
                isAnyGood = true;
            }
        }

        return isAnyGood;
    }
}
