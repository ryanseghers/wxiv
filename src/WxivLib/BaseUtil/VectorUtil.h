// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once

#include <vector>
#include <cfloat>
#include <climits>
#include <functional>

namespace Wxiv
{
    std::vector<float> vectorRandomFloat(int seed, int n, float min = 0.0f, float max = FLT_MAX);
    std::vector<int> vectorRandomInt(int seed, int n, int min = 0, int max = INT_MAX);

    /**
     * @brief Append all of v2 at the end of v1.
     */
    template <typename T> void vectorAppend(std::vector<T>& v1, const std::vector<T>& v2)
    {
        v1.insert(v1.end(), v2.begin(), v2.end());
    }

    /**
     * @brief Concatenate v1 and v2 and return the new vector.
     */
    template <typename T> std::vector<T> vectorConcat(const std::vector<T>& v1, const std::vector<T>& v2)
    {
        std::vector<T> both(v1);
        vectorConcat(both, v2);
        return both;
    }

    /**
     * @brief Generate a new vector by running a function that takes the index into the vector as argument.
     * @param n The count of elements to generate.
     * @return A new vector.
     */
    template <typename T> std::vector<T> vectorGen(int n, std::function<T(int n)> fn)
    {
        std::vector<T> v;
        v.reserve(n);

        for (int i = 0; i < n; i++)
        {
            v.push_back(fn(i));
        }

        return v;
    }
}
