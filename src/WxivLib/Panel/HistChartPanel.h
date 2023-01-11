// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <opencv2/opencv.hpp>

#include "WxWidgetsUtil.h"
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/grid.h>
#include "WxivImage.h"
#include "FloatHist.h"
#include "ImageViewPanel.h"
#include "ImageScrollPanel.h"

namespace Wxiv
{
    /**
     * @brief A panel that renders histogram and displays in a ImageViewPanel and has controls like bin count, log scale checkbox, etc.
     * You provide a callback to recompute the histogram from whatever the source data is.
     */
    class HistChartPanel : public wxPanel
    {
        std::string chartTitle;

        wxChoice* selectionsComboBox = nullptr;
        std::vector<wxString> selections;

        wxTextCtrl* histBinCountTextBox = nullptr;
        wxTextCtrl* histMinTextBox = nullptr;
        wxTextCtrl* histMaxTextBox = nullptr;
        wxCheckBox* histLogCheckBox = nullptr;
        ImageViewPanel* histImageViewPanel = nullptr;

        std::function<void(wxString selection, int binCount, float min, float max, FloatHist& hist)> computeCallback;

        FloatHist imageHist;

        void computeHistogram();
        void renderHistogram();

        void build(bool doIncludeSelections);

        void onSize(wxSizeEvent& event);
        void onHistParamChange(wxCommandEvent& event);

        int parseBinCount();
        float parseMin();
        float parseMax();

      public:
        HistChartPanel(wxWindow* parent, bool doIncludeSelections,
            std::function<void(wxString selection, int binCount, float min, float max, FloatHist& hist)> computeCallback);

        wxString getSelection();
        void setSelections(const std::vector<wxString>& newSelections, wxString newSelection);
        void clear();
        void refreshControls();

        void saveConfig(const char* context);
        void restoreConfig(const char* context);
    };
}