// Copyright(c) 2023 Ryan Seghers
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
    * @brief This has a combo box to select subject of the stats (whole image, current view, roi, etc)
    * and scatter plots for row and column profiles.
    */
    class ProfilesPanel : public wxPanel
    {
        const int defaultChoice = 1;
        const wxString choices[4]{"None", "Whole Image", "View ROI", "Drawn ROI"};

        // stats
        wxChoice* subjectComboBox = nullptr;

        ImageViewPanel* colProfilePanel = nullptr;
        ImageViewPanel* rowProfilePanel = nullptr;

        // this class interacts directly with the image scroll panel that renders the image (not the hist chart)
        // in order to compute the profiles
        ImageScrollPanel* imageScrollPanel = nullptr;

        std::shared_ptr<WxivImage> currentImage = nullptr;

        void build();
        void onSubjectChange(wxCommandEvent& event);
        std::string getSubjectString();

    public:
        ProfilesPanel(wxWindow* parent, ImageScrollPanel* imagePanel);

        void setImage(std::shared_ptr<WxivImage> p);
        void saveConfig();
        void restoreConfig();
        void refreshStats();
    };
}