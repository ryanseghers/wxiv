// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include "WxWidgetsUtil.h"
#include <wx/config.h>

#include "ShapeSet.h"
#include "IntensityRangeParams.h"

namespace Wxiv
{
    /**
     * @brief Settings for ImageViewPanel. Settings are information that we'd often want to persist.
     */
    struct ImageViewPanelSettings
    {
        /**
         * @brief Default is to not scale the image such that aspect ratio is changed.
         * Set this to true to instead always scale the view to fit the draw surface, regardless of zoom.
         */
        bool doScaleToFit = false;

        /**
         * @brief For doScaleToFit, can maintain aspect ratio or not.
         */
        bool doScaleMaintainAspectRatio = false;

        bool doRenderShapes = true;
        IntensityRangeParams intensityRangeParams;

        auto operator<=>(const ImageViewPanelSettings&) const = default;
        void loadConfig(wxConfigBase* cfg);
        void writeConfig(wxConfigBase* cfg);
    };
}
