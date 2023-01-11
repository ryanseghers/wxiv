// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include "WxWidgetsUtil.h"

#include <fmt/core.h>
#include <opencv2/opencv.hpp>

#include "ShapeMetadataHistPanel.h"
#include <CvPlot/cvplot.h>
#include "MiscUtil.h"

using namespace std;

namespace Wxiv
{
    const char* DefaultSelection = "None";

    /**
     * @brief Constructor.
     * @param parent
     * @param imagePanel We keep this in order to re/compute stats.
     */
    ShapeMetadataHistPanel::ShapeMetadataHistPanel(wxWindow* parent) : wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize)
    {
        build();
    }

    void ShapeMetadataHistPanel::setShapes(ShapeSet* newShapeSet)
    {
        this->shapes = newShapeSet;
        this->refreshControls();
    }

    /**
     * @brief Called when we switch to an image that has no shapes.
     */
    void ShapeMetadataHistPanel::clearShapes()
    {
        this->shapes = nullptr;
        this->refreshControls();
    }

    void ShapeMetadataHistPanel::saveConfig()
    {
        this->histChartPanel->saveConfig("ShapeMetadataHistPanel");
    }

    void ShapeMetadataHistPanel::restoreConfig()
    {
        wxString selection = wxConfigBase::Get()->Read("ShapeMetadataHistPanelSelection", "");

        if (!selection.empty())
        {
            this->refreshFieldComboBox(selection);
        }

        this->histChartPanel->restoreConfig("ShapeMetadataHistPanel");
    }

    /**
     * @brief
     *  wxBoxSizer vertical
     *      label, field combo box
     *      HistChartPanel
     */
    void ShapeMetadataHistPanel::build()
    {
        auto sizer = new wxBoxSizer(wxVERTICAL);

        const int defaultBorder = 4;
        const int gridSizerGap = 1;

        // hist and controls
        auto histControlsGridSizer = new wxFlexGridSizer(3, 2, gridSizerGap, gridSizerGap);
        sizer->Add(histControlsGridSizer, 0, wxEXPAND | wxALL, defaultBorder);

        // hist chart panel
        this->histChartPanel = new HistChartPanel(this, true, [&](wxString selection, int binCount, float min, float max, FloatHist& hist) {
            this->histComputeCallback(selection, binCount, min, max, hist);
        });
        sizer->Add(histChartPanel, 1, wxEXPAND | wxALL, defaultBorder);

        this->SetSizerAndFit(sizer);
    }

    void ShapeMetadataHistPanel::onHistParamChange(wxCommandEvent& event)
    {
        this->refreshHistogram();
    }

    /**
     * @brief Ensure the field name combo box has entries for the table columns.
     * @param selection To specify a current combo box value, else we try to preserve existing selection,
     * else fallback to a default selection.
     */
    void ShapeMetadataHistPanel::refreshFieldComboBox(wxString selection)
    {
        vector<wxString> fieldNames;

        // always have None in the list
        wxString defaultSelection(DefaultSelection);
        fieldNames.push_back(defaultSelection);

        // preserve selection
        if (!this->histChartPanel->getSelection().empty())
        {
            selection = this->histChartPanel->getSelection();
        }

        // fallback
        if (selection.empty())
        {
            selection = defaultSelection;
        }

        // get field names from shapes
        if ((this->shapes != nullptr) && !this->shapes->empty() && this->shapes->ptable)
        {
            vector<string> cols = this->shapes->ptable->ColumnNames();
            std::sort(cols.begin(), cols.end());
            fieldNames.insert(fieldNames.end(), cols.begin(), cols.end());
        }

        // we allow selection even if it is not in this set for continuity when stepping through images with different metadata
        if (!selection.empty())
        {
            if (std::find(fieldNames.begin(), fieldNames.end(), selection) == fieldNames.end())
            {
                fieldNames.push_back(selection);
            }
        }

        this->histChartPanel->setSelections(fieldNames, selection);
    }

    void ShapeMetadataHistPanel::refreshControls()
    {
        this->refreshFieldComboBox(wxString());
        this->refreshHistogram();
    }

    void ShapeMetadataHistPanel::histComputeCallback(wxString selection, int binCount, float min, float max, FloatHist& hist)
    {
        if (this->shapes && !selection.empty() && (selection != DefaultSelection))
        {
            vector<float> values = this->shapes->getFloatValues(selection.ToStdString());
            hist.compute(values, binCount, min, max);
        }
    }

    void ShapeMetadataHistPanel::refreshHistogram()
    {
        this->histChartPanel->refreshControls();
    }
}
