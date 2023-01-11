// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <string>
#include <algorithm>
#include <cassert>
#include <cstring>

#include "MathUtil.h"
#include "MiscUtil.h"

using namespace std;

namespace Wxiv
{
    /**
     * @brief Find bin index of the specified percentile in the input hist.
     * @param pct Percentile to find, 0 to 100
     * @return Bin index in the input hist.
     */
    int findPercentileInHist(const std::vector<int>& hist, float pct)
    {
        assert(pct >= 0.0f);
        assert(pct <= 100.0f);

        int totalSum = 0;

        for (int i = 0; i < hist.size(); i++)
        {
            totalSum += hist[i];
        }

        int sum = 0;
        int targetSum = (int)lroundf(pct / 100.0f * totalSum);

        for (int i = 0; i < hist.size(); i++)
        {
            sum += hist[i];

            if (sum >= targetSum)
            {
                return i;
            }
        }

        bail("findPercentileInHist: bug");
        return -1; // for compiler warning
    }

    /**
     * @brief Check if two float numbers are close to each other, with relative
     * and absolute tolerance.
     * The absolute delta between the number is compared to: atol + rtol * min(abs(a), abs(b))
     * @param atol Absolute tolerance.
     * @param rtol Relative tolerance.
     */
    bool checkClose(float a, float b, float rtol, float atol)
    {
        float delta = fabsf(a - b);
        float minVal = std::min(fabsf(a), fabsf(b));
        float threshold = atol + rtol * minVal;

        return (delta <= std::numeric_limits<float>::epsilon()) || (delta < threshold);
    }

    /**
     * @brief Check if two float numbers are close to each other, with relative
     * and absolute tolerance.
     * The absolute delta between the number is compared to: atol + rtol * min(abs(a), abs(b))
     * @param atol Absolute tolerance.
     * @param rtol Relative tolerance.
     */
    bool checkClose(double a, double b, double rtol, double atol)
    {
        double delta = std::abs(a - b);
        double minVal = std::min(std::abs(a), std::abs(b));
        double threshold = atol + rtol * minVal;

        return (delta <= std::numeric_limits<double>::epsilon()) || (delta < threshold);
    }

    /**
     * @brief Downsample a hist for render to fewer bins.
     * @param hist
     * @param maxBinIndex Highest used bin index in the input hist, e.g. for 16U with 12-bit range.
     * @param newBinCount How many bins we want output.
     * @param newBins
     * @param newHist
     */
    void downsampleHist(
        const std::vector<int>& hist, const std::vector<float>& bins, int newBinCount, std::vector<float>& newBins, std::vector<int>& newHist)
    {
        // often for 16U it doesn't make sense to include the whole range, so find max nonzero bin
        int maxBinIndex = -1;

        for (int i = 0; i < hist.size(); i++)
        {
            if (hist[i])
                maxBinIndex = i;
        }

        if (maxBinIndex > 0)
        {
            // bin width (bins are pixel values)
            int binWidth = (maxBinIndex + 1) / newBinCount;
            binWidth = std::max(1, binWidth);

            newBins.reserve(newBinCount);
            newHist.reserve(newBinCount);

            // for each new bin
            for (int i = 0; i < newBinCount; i++)
            {
                // bins are by the lowest val in bin
                newBins.push_back(bins[i / binWidth]);

                // combine hist bins into this bar
                int binSum = 0;

                for (int j = 0; j < binWidth; j++)
                {
                    int oldIdx = i * binWidth + j;

                    if (oldIdx < hist.size())
                    {
                        binSum += hist[oldIdx];
                    }
                }

                newHist.push_back((int)binSum);
            }
        }
    }
}
