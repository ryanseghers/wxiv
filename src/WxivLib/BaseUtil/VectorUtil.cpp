// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <vector>
#include <random>
#include <functional>

#include "VectorUtil.h"
#include "MiscUtil.h"

using namespace std;

namespace Wxiv
{
    /**
     * @brief Get a vector of random floats from min to max.
     * @param seed PRNG seed. A value lt 0 means use the clock.
     * @param n Number of values to get.
     * @param min Minimum value for the random floats.
     * @param max Maximum value for the random floats.
     * @return A new vector.
     */
    std::vector<float> vectorRandomFloat(int seed, int n, float min, float max)
    {
        auto prng = getPrng(seed);
        std::uniform_real_distribution<float> distrib(min, max);
        std::vector<float> v;
        v.reserve(n);

        for (int i = 0; i < n; i++)
        {
            v.push_back(distrib(prng));
        }

        return v;
    }

    /**
     * @brief Get a vector of random ints from min to less than max.
     * @param seed PRNG seed. A value lt 0 means use the clock.
     * @param n Number of values to get.
     * @param min Minimum value for the random ints.
     * @param max One over the maximum value for the random ints.
     * @return A new vector.
     */
    std::vector<int> vectorRandomInt(int seed, int n, int min, int max)
    {
        auto prng = getPrng(seed);
        std::uniform_int_distribution<int> distrib(min, max);
        std::vector<int> v;
        v.reserve(n);

        for (int i = 0; i < n; i++)
        {
            v.push_back(distrib(prng));
        }

        return v;
    }
}
