// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once

#include "WxWidgetsUtil.h"
#include <wx/config.h>

namespace Wxiv
{
    enum class IntensityRangeMode
    {
        NoOp,
        Explicit,
        WholeImagePercentile,
        ViewPercentile,
    };

    /**
     * @brief Parameters that ImageViewPanel uses to do intensity ranging.
     */
    struct IntensityRangeParams
    {
        IntensityRangeMode mode = IntensityRangeMode::ViewPercentile;

        /**
         * @brief For explicit ranging, the value in the image to map to intensity 0.
         * This is float in case the input image is float, but will of course be cast to the image type.
         */
        float explicitLowValue = 0.0f;

        /**
         * @brief For explicit ranging, the value in the image to map to intensity 255.
         * This is float in case the input image is float, but will of course be cast to the image type.
         */
        float explicitHighValue = 0.0f;

        /**
         * @brief Percentile of image to map to intensity 0.
         * This is range 0 to 100.
         */
        float wholeImageLowPercentile = 0.1f;

        /**
         * @brief Percentile of image to map to intensity 255.
         * This is range 0 to 100.
         */
        float wholeImageHighPercentile = 99.9f;

        /**
         * @brief Percentile of image to map to intensity 0.
         * This is range 0 to 100.
         */
        float viewRoiLowPercentile = 0.1f;

        /**
         * @brief Percentile of image to map to intensity 255.
         * This is range 0 to 100.
         */
        float viewRoiHighPercentile = 99.9f;

        auto operator<=>(const IntensityRangeParams&) const = default;
        void loadConfig(wxConfigBase* cfg);
        void writeConfig(wxConfigBase* cfg);
    };
}