// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once

#include <string>
#include <chrono>
#include <random>

namespace Wxiv
{
    // misc
    void bail(const char* msg);
    void bail(const std::string msg);

    // time
    std::chrono::steady_clock::time_point getTimeNow();
    float getDurationSeconds(std::chrono::steady_clock::time_point startTime);

    std::mt19937 getPrng(int seed);
}
