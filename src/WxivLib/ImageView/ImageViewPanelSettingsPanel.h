// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include "WxWidgetsUtil.h"
#include "ImageViewPanelSettings.h"

namespace Wxiv
{
    /**
     * @brief An editor (panel) for ImageViewPanelSettings.
     */
    class ImageViewPanelSettingsPanel : public wxPanel
    {
        ImageViewPanelSettings settings;

        wxCheckBox* doRenderShapesCheckBox = nullptr;

        wxRadioButton* radioButtonModeNone = nullptr;
        wxRadioButton* radioButtonModeWholeImagePercentile = nullptr;
        wxRadioButton* radioButtonModeViewRoiPercentile = nullptr;
        wxRadioButton* radioButtonModeExplicit = nullptr;

        wxTextCtrl* wholeImagePercentileLow = nullptr;
        wxTextCtrl* wholeImagePercentileHigh = nullptr;
        wxTextCtrl* viewRoiPercentileLow = nullptr;
        wxTextCtrl* viewRoiPercentileHigh = nullptr;
        wxTextCtrl* explicitValuesLow = nullptr;
        wxTextCtrl* explicitValuesHigh = nullptr;

      public:
        ImageViewPanelSettingsPanel(wxWindow* parent, ImageViewPanelSettings initialSettings, bool hideRenderShapesOption);
        ImageViewPanelSettings getSettings();
    };
}
