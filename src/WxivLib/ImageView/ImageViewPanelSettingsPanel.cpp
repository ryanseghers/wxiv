// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "WxWidgetsUtil.h"
#include <wx/splitter.h>

#include <fmt/core.h>
#include "ImageViewPanelSettingsPanel.h"

namespace Wxiv
{
    wxBoxSizer* buildHorizontalLabeledTextBoxes(
        wxWindow* parent, wxTextCtrl** ppTextCtrlLow, wxTextCtrl** ppTextCtrlHigh, float lowPct, float highPct, std::string precisionStr)
    {
        std::string fmtStr = "{:" + precisionStr + "}";

        auto sizer = new wxBoxSizer(wxHORIZONTAL);
        *ppTextCtrlLow = new wxTextCtrl(parent, wxID_ANY, fmt::format(fmt::runtime(fmtStr), lowPct));
        *ppTextCtrlHigh = new wxTextCtrl(parent, wxID_ANY, fmt::format(fmt::runtime(fmtStr), highPct));
        sizer->Add(new wxStaticText(parent, wxID_ANY, "Low"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
        sizer->Add(*ppTextCtrlLow, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
        sizer->Add(new wxStaticText(parent, wxID_ANY, "High"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
        sizer->Add(*ppTextCtrlHigh, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);

        return sizer;
    }

    /**
     * @param hideRenderShapesOption By default this shows a checkbox for whether or not to render shapes. Set this to true to not show it.
     */
    ImageViewPanelSettingsPanel::ImageViewPanelSettingsPanel(wxWindow* parent, ImageViewPanelSettings initialSettings, bool hideRenderShapesOption)
        : wxPanel(parent)
    {
        this->settings = initialSettings;

        const int borderFlags = wxALL;
        const int borderWidth = 12;

        auto vertSizer = new wxBoxSizer(wxVERTICAL);

        // doRenderShapes
        if (!hideRenderShapesOption)
        {
            this->doRenderShapesCheckBox = new wxCheckBox(this, wxID_ANY, "Render Shapes");
            vertSizer->Add(doRenderShapesCheckBox, 0, wxEXPAND | borderFlags, borderWidth);
            this->doRenderShapesCheckBox->SetValue(this->settings.doRenderShapes);
        }

        // intensity ranging controls
        auto intensitySizer = new wxStaticBoxSizer(wxVERTICAL, this, "Intensity Ranging");
        this->radioButtonModeNone = new wxRadioButton(intensitySizer->GetStaticBox(), wxID_ANY, "None", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
        this->radioButtonModeWholeImagePercentile = new wxRadioButton(intensitySizer->GetStaticBox(), wxID_ANY, "Whole-image Percentiles");
        this->radioButtonModeViewRoiPercentile = new wxRadioButton(intensitySizer->GetStaticBox(), wxID_ANY, "View ROI Percentiles");
        this->radioButtonModeExplicit = new wxRadioButton(intensitySizer->GetStaticBox(), wxID_ANY, "Explicit");
        intensitySizer->Add(this->radioButtonModeNone, 0, wxALL, 6);

        // whole-image percentiles
        intensitySizer->Add(this->radioButtonModeWholeImagePercentile, 0, wxALL, 6);
        float lowPct = this->settings.intensityRangeParams.wholeImageLowPercentile;
        float highPct = this->settings.intensityRangeParams.wholeImageHighPercentile;
        auto percentilesSizer1 = buildHorizontalLabeledTextBoxes(
            intensitySizer->GetStaticBox(), &wholeImagePercentileLow, &wholeImagePercentileHigh, lowPct, highPct, ".2f");
        intensitySizer->Add(percentilesSizer1, 0, wxLEFT, 24);

        // view-roi percentiles
        intensitySizer->Add(this->radioButtonModeViewRoiPercentile, 0, wxALL, 6);
        lowPct = this->settings.intensityRangeParams.viewRoiLowPercentile;
        highPct = this->settings.intensityRangeParams.viewRoiHighPercentile;
        auto percentilesSizer2 =
            buildHorizontalLabeledTextBoxes(intensitySizer->GetStaticBox(), &viewRoiPercentileLow, &viewRoiPercentileHigh, lowPct, highPct, ".2f");
        intensitySizer->Add(percentilesSizer2, 0, wxLEFT, 24);

        // explicit values
        intensitySizer->Add(this->radioButtonModeExplicit, 0, wxALL, 6);
        lowPct = this->settings.intensityRangeParams.explicitLowValue;
        highPct = this->settings.intensityRangeParams.explicitHighValue;
        auto percentilesSizer3 =
            buildHorizontalLabeledTextBoxes(intensitySizer->GetStaticBox(), &explicitValuesLow, &explicitValuesHigh, lowPct, highPct, ".0f");
        intensitySizer->Add(percentilesSizer3, 0, wxLEFT, 24);

        vertSizer->Add(intensitySizer, 0, wxALL, 12);

        this->radioButtonModeNone->SetValue(this->settings.intensityRangeParams.mode == IntensityRangeMode::NoOp);
        this->radioButtonModeWholeImagePercentile->SetValue(this->settings.intensityRangeParams.mode == IntensityRangeMode::WholeImagePercentile);
        this->radioButtonModeViewRoiPercentile->SetValue(this->settings.intensityRangeParams.mode == IntensityRangeMode::ViewPercentile);
        this->radioButtonModeExplicit->SetValue(this->settings.intensityRangeParams.mode == IntensityRangeMode::Explicit);

        this->SetSizerAndFit(vertSizer);
    }

    ImageViewPanelSettings ImageViewPanelSettingsPanel::getSettings()
    {
        // controls to settings
        if (this->doRenderShapesCheckBox)
        {
            this->settings.doRenderShapes = this->doRenderShapesCheckBox->IsChecked();
        }

        if (this->radioButtonModeNone->GetValue())
        {
            this->settings.intensityRangeParams.mode = IntensityRangeMode::NoOp;
        }
        else if (this->radioButtonModeWholeImagePercentile->GetValue())
        {
            this->settings.intensityRangeParams.mode = IntensityRangeMode::WholeImagePercentile;
        }
        else if (this->radioButtonModeViewRoiPercentile->GetValue())
        {
            this->settings.intensityRangeParams.mode = IntensityRangeMode::ViewPercentile;
        }
        else if (this->radioButtonModeExplicit->GetValue())
        {
            this->settings.intensityRangeParams.mode = IntensityRangeMode::Explicit;
        }

        this->settings.intensityRangeParams.wholeImageLowPercentile = std::stof(this->wholeImagePercentileLow->GetValue().ToStdString());
        this->settings.intensityRangeParams.wholeImageHighPercentile = std::stof(this->wholeImagePercentileHigh->GetValue().ToStdString());

        this->settings.intensityRangeParams.viewRoiLowPercentile = std::stof(this->viewRoiPercentileLow->GetValue().ToStdString());
        this->settings.intensityRangeParams.viewRoiHighPercentile = std::stof(this->viewRoiPercentileHigh->GetValue().ToStdString());

        this->settings.intensityRangeParams.explicitLowValue = std::stof(this->explicitValuesLow->GetValue().ToStdString());
        this->settings.intensityRangeParams.explicitHighValue = std::stof(this->explicitValuesHigh->GetValue().ToStdString());

        return this->settings;
    }
}
