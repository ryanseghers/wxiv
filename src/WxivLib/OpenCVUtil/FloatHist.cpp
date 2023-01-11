// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <string>

#include <opencv2/opencv.hpp>
#include <fmt/core.h>

#include "FloatHist.h"
#include "MiscUtil.h"
#include "MathUtil.h"

using namespace std;

namespace Wxiv
{
    void FloatHist::copy(FloatHist& other)
    {
        this->minVal = other.minVal;
        this->maxVal = other.maxVal;
        this->counts = other.counts;
        this->bins = other.bins;
    }

    void FloatHist::clear()
    {
        this->counts.clear();
        this->bins.clear();
        this->minVal = 0.0f;
        this->maxVal = 0.0f;
    }

    bool FloatHist::empty()
    {
        return this->counts.empty();
    }

    int FloatHist::getBinCount()
    {
        return (int)this->bins.size();
    }

    float FloatHist::getBinSize()
    {
        if (this->bins.size() > 1)
        {
            return this->bins[1] - this->bins[0];
        }
        else
        {
            return NAN;
        }
    }

    float FloatHist::getMin()
    {
        return this->minVal;
    }

    float FloatHist::getMax()
    {
        return this->maxVal;
    }

    /**
     * @brief
     * @param values
     * @param binCount
     * @param inMinVal If NAN then is set to minimum good value in the input vector, or 0 if that value is gt 0.
     * @param inMaxVal If NAN then is set to maximum good value in the input vector.
     */
    void FloatHist::compute(const std::vector<float>& values, int binCount, float inMinVal, float inMaxVal)
    {
        this->minVal = inMinVal;
        this->maxVal = inMaxVal;

        if (std::isnan(minVal) || std::isnan(maxVal))
        {
            float foundMin, foundMax;

            if (vectorMinMax(values, foundMin, foundMax))
            {
                if (std::isnan(minVal))
                {
                    minVal = foundMin;

                    // almost always want min to be 0, unless actual values go lt 0
                    if (minVal > 0)
                    {
                        minVal = 0;
                    }
                }

                if (std::isnan(maxVal))
                {
                    maxVal = foundMax + std::numeric_limits<float>::epsilon();
                }
            }
            else
            {
                // empty hist
                bins.resize(0);
                counts.resize(0);
                return;
            }
        }

        if (maxVal <= minVal)
        {
            // all same value
            bins.resize(1);
            bins[0] = minVal;
            counts.resize(1);
            counts[0] = 0;
        }
        else
        {
            // bins
            bins.resize(binCount);
            counts.resize(binCount);
            float binSize = (maxVal - minVal) / binCount;

            for (int i = 0; i < binCount; i++)
            {
                bins[i] = minVal + i * binSize;
                counts[i] = 0;
            }

            // hist
            for (int i = 0; i < values.size(); i++)
            {
                float v = values[i];

                if (!std::isfinite(v))
                {
                    counts[binCount - 1]++;
                }
                else if ((v >= minVal) && (v <= maxVal))
                {
                    int idx = (int)((values[i] - minVal) / binSize);
                    idx = std::min(binCount - 1, idx); // if maxVal is == max in array then at least one idx would go over
                    counts[idx]++;
                }
            }
        }
    }
}
