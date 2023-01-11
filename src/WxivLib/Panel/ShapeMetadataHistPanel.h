// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include "WxWidgetsUtil.h"
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/grid.h>

#include <opencv2/opencv.hpp>
#include "WxivImage.h"
#include "FloatHist.h"
#include "HistChartPanel.h"

namespace Wxiv
{
    /**
     * @brief A panel with controls to specify how to render histogram and a ImageViewPanel to render the hist plot.
     * This has a combo box to select which field to show.
     */
    class ShapeMetadataHistPanel : public wxPanel
    {
        ShapeSet* shapes = nullptr;
        HistChartPanel* histChartPanel = nullptr;

        void build();
        void refreshFieldComboBox(wxString selection);
        void onHistParamChange(wxCommandEvent& event);
        void refreshHistogram();
        void histComputeCallback(wxString selection, int binCount, float min, float max, FloatHist& hist);

      public:
        ShapeMetadataHistPanel(wxWindow* parent);
        void setShapes(ShapeSet* shapes);
        void clearShapes();
        void refreshControls();

        void saveConfig();
        void restoreConfig();
    };
}