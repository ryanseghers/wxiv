// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include "IntensityRangeParams.h"

namespace Wxiv
{
    void IntensityRangeParams::loadConfig(wxConfigBase* cfg)
    {
        this->mode = (IntensityRangeMode)cfg->ReadLong("mode", (long)IntensityRangeMode::ViewPercentile);
        this->explicitHighValue = (float)cfg->ReadDouble("explicitHighValue", 0.0);
        this->explicitLowValue = (float)cfg->ReadDouble("explicitLowValue", 0.0);
        this->viewRoiHighPercentile = (float)cfg->ReadDouble("viewRoiHighPercentile", 99.9);
        this->viewRoiLowPercentile = (float)cfg->ReadDouble("viewRoiLowPercentile", 0.1);
        this->wholeImageHighPercentile = (float)cfg->ReadDouble("wholeImageHighPercentile", 99.9);
        this->wholeImageLowPercentile = (float)cfg->ReadDouble("wholeImageLowPercentile", 0.1);
    }

    void IntensityRangeParams::writeConfig(wxConfigBase* cfg)
    {
        cfg->Write("mode", (long)this->mode);
        cfg->Write("explicitHighValue", this->explicitHighValue);
        cfg->Write("explicitLowValue", this->explicitLowValue);
        cfg->Write("viewRoiHighPercentile", this->viewRoiHighPercentile);
        cfg->Write("viewRoiLowPercentile", this->viewRoiLowPercentile);
        cfg->Write("wholeImageHighPercentile", this->wholeImageHighPercentile);
        cfg->Write("wholeImageLowPercentile", this->wholeImageLowPercentile);
    }
}
