// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <wx/wxprec.h>
#include <wx/splitter.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <opencv2/opencv.hpp>

namespace Wxiv
{
    /**
     * @brief Just a panel with scrollbars, for sub-classing for image viewer.
     */
    class ManualScrollPanel : public wxWindow
    {
      protected:
        wxPanel* panel;
        wxScrollBar* hScrollBar;
        wxScrollBar* vScrollBar;

        virtual void onScrollbarChanged(wxScrollEvent& event);
        virtual void onScrollbarThumbTrack(wxScrollEvent& event);

      public:
        ManualScrollPanel(wxFrame* parent);
        ManualScrollPanel(wxSplitterWindow* splitter);
        void setupControls();
    };
}
