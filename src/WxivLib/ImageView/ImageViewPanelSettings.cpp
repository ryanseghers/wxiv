// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include "WxWidgetsUtil.h"
#include <wx/config.h>

#include "ImageViewPanelSettings.h"

namespace Wxiv
{
    void ImageViewPanelSettings::loadConfig(wxConfigBase* cfg)
    {
        this->doScaleToFit = cfg->ReadBool("doScaleToFit", true);
        this->doScaleMaintainAspectRatio = cfg->ReadBool("doScaleMaintainAspectRatio", true);
        this->doRenderShapes = cfg->ReadBool("doRenderShapes", true);
        this->doRenderPixelValues = cfg->ReadBool("doRenderPixelValues", true);
        this->maxZoom = (float)cfg->ReadDouble("maxZoom", DefaultMaxZoom);

        // ensure maxZoom is reasonable
        if (this->maxZoom <= 10.0f)
        {
            this->maxZoom = DefaultMaxZoom;
        }

        this->intensityRangeParams.loadConfig(cfg);
    }

    void ImageViewPanelSettings::writeConfig(wxConfigBase* cfg)
    {
        cfg->Write("doScaleToFit", this->doScaleToFit);
        cfg->Write("doScaleMaintainAspectRatio", this->doScaleMaintainAspectRatio);
        cfg->Write("doRenderShapes", this->doRenderShapes);
        cfg->Write("doRenderPixelValues", this->doRenderPixelValues);
        cfg->Write("maxZoom", this->maxZoom);
        this->intensityRangeParams.writeConfig(cfg);
    }
}
