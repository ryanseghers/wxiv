// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include "WxWidgetsUtil.h"
#include <wx/listctrl.h>
#include <wx/splitter.h>

#include <opencv2/opencv.hpp>
#include "ImageScrollPanel.h"
#include "WxivImage.h"
#include "HistChartPanel.h"

namespace Wxiv
{
    /**
     * @brief This has a combo box to select subject of the stats (whole image, current view, roi, etc), a table with stats,
     * and a HistChartPanel for pixel hist.
     */
    class PixelStatsPanel : public wxPanel
    {
        const int defaultChoice = 1;
        const wxString choices[4]{"None", "Whole WxivImage", "View ROI", "Drawn ROI"};

        // stats
        wxChoice* subjectComboBox = nullptr;
        wxSplitterWindow* mainSplitter = nullptr; // vert splitter between table and histogram
        wxListView* statsTableListView = nullptr;
        HistChartPanel* histChartPanel = nullptr;

        // this class interacts directly with the image scroll panel that renders the image (not the hist chart)
        // in order to compute hist on pixels etc
        ImageScrollPanel* imageScrollPanel = nullptr;

        std::shared_ptr<WxivImage> currentImage = nullptr;

        void build();
        void onSubjectChange(wxCommandEvent& event);
        void computeHistogram(int binCount, float min, float max, FloatHist& hist);
        void updateHistogram();
        void histComputeCallback(wxString selection, int binCount, float min, float max, FloatHist& hist);
        std::string getSubjectString();

      public:
        PixelStatsPanel(wxWindow* parent, ImageScrollPanel* imagePanel);

        void setImage(std::shared_ptr<WxivImage> p);
        void saveConfig();
        void restoreConfig();
        void refreshStats();
    };
}