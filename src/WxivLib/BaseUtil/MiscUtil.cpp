// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <string>
#include <algorithm>
#include <filesystem>
#include <cassert>
#include <cstring>
#include <random>

// https://github.com/scottt/debugbreak
#include "debugbreak.h"

#include "MiscUtil.h"

using namespace std;

namespace Wxiv
{
    void bail(const char* msg)
    {
        debug_break();
        throw std::runtime_error(msg);
    }

    void bail(const std::string msg)
    {
        debug_break();
        throw std::runtime_error(msg.c_str());
    }

    std::chrono::steady_clock::time_point getTimeNow()
    {
        return std::chrono::steady_clock::now();
    }

    float getDurationSeconds(std::chrono::steady_clock::time_point startTime)
    {
        std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
        float duration_ms = (float)std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        return duration_ms / 1000.0f;
    }

    /**
     * @brief Get a random engine.
     * @param seed RNG seed. A value less than zero means to use the clock.
     */
    std::mt19937 getPrng(int seed)
    {
        std::mt19937 engine;

        if (seed >= 0)
        {
            engine.seed((unsigned int)seed);
        }
        else
        {
            engine.seed((unsigned int)std::chrono::system_clock::now().time_since_epoch().count());
        }

        return engine;
    }
}
