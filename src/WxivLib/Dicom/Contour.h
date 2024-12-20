#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <fmt/core.h>

namespace Wxiv
{
    struct ContourPoint
    {
        float x;
        float y;
        float z;
    };

    /**
     * This is a volume contour, meaning one that spans all Z.
     * This includes polygons for all slices.
     */
    struct Contour
    {
        std::string name;
        std::uint8_t rgbColor[3];
        std::int32_t referencedRoiNumber;

        // the following two vectors correspond
        std::vector<std::vector<ContourPoint>> slicePoints; // e.g. 51 slices each with a contour
        std::vector<std::string> referencedSopInstanceUids; // the image that each slice is for

        std::string toString()
        {
            std::string s = fmt::format("number: {}, color: ({},{},{}), sliceCount: {}, name: {}", referencedRoiNumber, (int)rgbColor[0],
                (int)rgbColor[1], (int)rgbColor[2], slicePoints.size(), name);
            return s;
        }
    };
}
