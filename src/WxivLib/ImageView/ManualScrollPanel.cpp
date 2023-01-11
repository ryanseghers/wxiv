// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include "WxWidgetsUtil.h"
#include <wx/splitter.h>

#include <fmt/core.h>
#include "ManualScrollPanel.h"

namespace Wxiv
{
    ManualScrollPanel::ManualScrollPanel(wxFrame* parent) : wxWindow(parent, -1, wxDefaultPosition, wxDefaultSize, wxALWAYS_SHOW_SB)
    {
        setupControls();
    }

    ManualScrollPanel::ManualScrollPanel(wxSplitterWindow* splitter) : wxWindow(splitter, -1, wxDefaultPosition, wxDefaultSize, wxALWAYS_SHOW_SB)
    {
        setupControls();
    }

    void ManualScrollPanel::setupControls()
    {
        // sizer hierarchy with one for the main panel and horizontal scrollbar below it,
        // then another for the first sizer with a vert scrollbar to the right
        this->panel = new wxPanel(this);

        this->hScrollBar = new wxScrollBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_HORIZONTAL);
        this->hScrollBar->SetRange(100);
        this->hScrollBar->SetThumbPosition(0);
        this->hScrollBar->SetThumbSize(100);

        // sizer with main panel and horizontal scrollbar under it
        auto vertSizer = new wxBoxSizer(wxVERTICAL);
        vertSizer->Add(this->panel, 1, wxEXPAND, 0);
        vertSizer->Add(this->hScrollBar, 0, wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 0);

        // now a sizer for the subpanel with vert scrollbar to the right
        this->vScrollBar = new wxScrollBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);
        this->vScrollBar->SetRange(100);
        this->vScrollBar->SetThumbPosition(0);
        this->vScrollBar->SetThumbSize(100);

        // sizer with the above sizer with vert scrollbar to the right
        auto hSizer = new wxBoxSizer(wxHORIZONTAL);
        hSizer->Add(vertSizer, 1, wxEXPAND, 0);
        hSizer->Add(this->vScrollBar, 0, wxEXPAND | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 0);
        this->SetSizerAndFit(hSizer);

        // bind events
        Bind(wxEVT_SCROLL_CHANGED, &ManualScrollPanel::onScrollbarChanged, this, wxID_ANY);
        Bind(wxEVT_SCROLL_THUMBTRACK, &ManualScrollPanel::onScrollbarThumbTrack, this, wxID_ANY);
    }

    void ManualScrollPanel::onScrollbarChanged(wxScrollEvent& event)
    {
        if (event.GetOrientation() == wxHORIZONTAL)
        {
            wxMessageBox("Got a horz scroll event");
        }
        else
        {
            wxMessageBox("Got a vert scroll event");
        }
    }

    void ManualScrollPanel::onScrollbarThumbTrack(wxScrollEvent& event)
    {
        if (event.GetOrientation() == wxHORIZONTAL)
        {
            wxMessageBox("horz tracking");
        }
        else
        {
            wxMessageBox("vert tracking");
        }
    }
}
