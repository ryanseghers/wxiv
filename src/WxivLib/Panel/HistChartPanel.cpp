// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <fmt/core.h>
#include <opencv2/opencv.hpp>
#include <CvPlot/cvplot.h>

#include "WxWidgetsUtil.h"
#include "HistChartPanel.h"
#include "MiscUtil.h"
#include "StringUtil.h"

using namespace std;

namespace Wxiv
{
    /**
     * @brief Constructor.
     * @param parent
     * @param imagePanel We keep this in order to re/compute stats.
     */
    HistChartPanel::HistChartPanel(wxWindow* parent, bool doIncludeSelections,
        std::function<void(wxString selection, int binCount, float min, float max, FloatHist& hist)> computeCallback)
        : wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize), computeCallback(computeCallback)
    {
        build(doIncludeSelections);
        Bind(wxEVT_SIZE, &HistChartPanel::onSize, this, wxID_ANY);
    }

    /**
     * @brief To re-render the hist on resize.
     * @param event
     */
    void HistChartPanel::onSize(wxSizeEvent& event)
    {
        this->renderHistogram();
        event.Skip();
    }

    /**
     * @brief Clear the data and render empty hist.
     */
    void HistChartPanel::clear()
    {
        this->imageHist.clear();
        this->refreshControls();
    }

    void HistChartPanel::saveConfig(const char* context)
    {
        wxConfigBase::Get()->Write(wxString(context) + "_HistChartPanelBinCount", this->histBinCountTextBox->GetValue());
        wxConfigBase::Get()->Write(wxString(context) + "_HistChartPanelMin", this->histMinTextBox->GetValue());
        wxConfigBase::Get()->Write(wxString(context) + "_HistChartPanelMax", this->histMaxTextBox->GetValue());
        wxConfigBase::Get()->Write(wxString(context) + "_HistChartPanelIsLogScale", this->histLogCheckBox->GetValue());
    }

    void HistChartPanel::restoreConfig(const char* context)
    {
        // DO restore empty string since it disables hist which might be what user wants
        wxString histBinCountStr = wxConfigBase::Get()->Read(wxString(context) + "_HistChartPanelBinCount", "");
        this->histBinCountTextBox->SetValue(histBinCountStr);

        this->histMinTextBox->SetValue(wxConfigBase::Get()->Read(wxString(context) + "_HistChartPanelMin", ""));
        this->histMaxTextBox->SetValue(wxConfigBase::Get()->Read(wxString(context) + "_HistChartPanelMax", ""));

        this->histLogCheckBox->SetValue(wxConfigBase::Get()->ReadBool(wxString(context) + "_HistChartPanelIsLogScale", false));
    }

    /**
     * @brief
     *  wxBoxSizer vertical
     *      wxFlexGridSizer (2 cols)
     *          label, hist bin count
     *          label, log checkbox
     *          ...
     *      ImageViewPanel for hist plot
     */
    void HistChartPanel::build(bool doIncludeSelections)
    {
        auto sizer = new wxBoxSizer(wxVERTICAL);

        const int defaultBorder = 4;
        const int labelBorderFlags = wxALL;
        const int labelBorder = 4;
        const int textBoxBorderFlags = wxALL;
        const int textBoxBorder = 4;
        const int labelFlags = wxFIXED | labelBorderFlags | wxALIGN_CENTER_VERTICAL;
        const int gridSizerGap = 1;

        // hist and controls
        auto histControlsGridSizer = new wxFlexGridSizer(2, gridSizerGap, gridSizerGap);
        sizer->Add(histControlsGridSizer, 0, wxEXPAND | wxALL, defaultBorder);

        // selections
        if (doIncludeSelections)
        {
            histControlsGridSizer->Add(new wxStaticText(this, wxID_ANY, wxString("Selection")), 0, labelFlags, labelBorder);
            this->selectionsComboBox = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr);
            this->selectionsComboBox->Bind(wxEVT_CHOICE, &HistChartPanel::onHistParamChange, this, wxID_ANY);
            histControlsGridSizer->Add(selectionsComboBox, 1, wxFIXED | textBoxBorderFlags, textBoxBorder);
        }

        // hist bin count
        histControlsGridSizer->Add(new wxStaticText(this, wxID_ANY, wxString("Bin Count")), 0, labelFlags, labelBorder);
        this->histBinCountTextBox = new wxTextCtrl(this, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize);
        this->histBinCountTextBox->Bind(wxEVT_TEXT, &HistChartPanel::onHistParamChange, this, wxID_ANY); // happens on every keypress
        histControlsGridSizer->Add(histBinCountTextBox, 1, wxEXPAND | textBoxBorderFlags, textBoxBorder);
        histControlsGridSizer->SetItemMinSize(histBinCountTextBox, 100, -1); // force minimum width for this column

        // min
        histControlsGridSizer->Add(new wxStaticText(this, wxID_ANY, wxString("Min")), 0, labelFlags, labelBorder);
        this->histMinTextBox = new wxTextCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);
        this->histMinTextBox->Bind(wxEVT_TEXT, &HistChartPanel::onHistParamChange, this, wxID_ANY); // happens on every keypress
        histControlsGridSizer->Add(histMinTextBox, 1, wxEXPAND | textBoxBorderFlags, textBoxBorder);

        // max
        histControlsGridSizer->Add(new wxStaticText(this, wxID_ANY, wxString("Max")), 0, labelFlags, labelBorder);
        this->histMaxTextBox = new wxTextCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize);
        this->histMaxTextBox->Bind(wxEVT_TEXT, &HistChartPanel::onHistParamChange, this, wxID_ANY); // happens on every keypress
        histControlsGridSizer->Add(histMaxTextBox, 1, wxEXPAND | textBoxBorderFlags, textBoxBorder);

        // log-scale checkbox
        histControlsGridSizer->Add(new wxStaticText(this, wxID_ANY, wxString("Log 10 Scale")), 0, labelFlags, labelBorder);
        this->histLogCheckBox = new wxCheckBox(this, wxID_ANY, "");
        this->histLogCheckBox->Bind(wxEVT_CHECKBOX, &HistChartPanel::onHistParamChange, this, wxID_ANY);
        histControlsGridSizer->Add(histLogCheckBox, 1, wxEXPAND | textBoxBorderFlags, textBoxBorder);

        // hist image view panel
        this->histImageViewPanel = new ImageViewPanel(this);
        this->histImageViewPanel->setBackground(255);
        sizer->Add(this->histImageViewPanel, 1, wxEXPAND | wxALL, defaultBorder);

        this->SetSizerAndFit(sizer);
    }

    wxString HistChartPanel::getSelection()
    {
        if (!this->selectionsComboBox->GetStringSelection().empty())
        {
            int idx = this->selectionsComboBox->GetSelection();
            return selections[idx];
        }
        else
        {
            return wxString();
        }
    }

    void HistChartPanel::setSelections(const vector<wxString>& newSelections, wxString newSelection)
    {
        const char* DefaultSelection = "None";

        // preserve selection
        wxString selection = newSelection;

        if (!this->selectionsComboBox->GetStringSelection().empty())
        {
            selection = this->getSelection();
        }

        // fallback
        if (selection.empty())
        {
            selection = DefaultSelection;
        }

        this->selectionsComboBox->Set(newSelections);
        this->selections = newSelections;

        // restore selection
        if (!selection.empty())
        {
            this->selectionsComboBox->SetStringSelection(selection);
        }
    }

    void HistChartPanel::onHistParamChange(wxCommandEvent& event)
    {
        this->refreshControls();
    }

    void HistChartPanel::refreshControls()
    {
        this->computeHistogram();
        this->renderHistogram();
    }

    int HistChartPanel::parseBinCount()
    {
        int binCount = 100;
        string binCountStr = this->histBinCountTextBox->GetValue().ToStdString();

        if (!binCountStr.empty())
        {
            tryParseInt(binCountStr, binCount);
        }

        return binCount;
    }

    float HistChartPanel::parseMin()
    {
        float min = NAN;
        string minStr = this->histMinTextBox->GetValue().ToStdString();

        if (!minStr.empty())
        {
            tryParseFloat(minStr, min);
        }

        return min;
    }

    float HistChartPanel::parseMax()
    {
        float max = NAN;
        string maxStr = this->histMaxTextBox->GetValue().ToStdString();

        if (!maxStr.empty())
        {
            tryParseFloat(maxStr, max);
        }

        return max;
    }

    void HistChartPanel::computeHistogram()
    {
        if (computeCallback)
        {
            int binCount = this->parseBinCount();
            float min = this->parseMin();
            float max = this->parseMax();
            wxString selectionString;

            if (this->selectionsComboBox)
            {
                selectionString = this->getSelection();
            }

            computeCallback(selectionString, binCount, min, max, this->imageHist);
        }
        else
        {
            this->imageHist.clear();
        }
    }

    void HistChartPanel::renderHistogram()
    {
        // fit to panel aspect ratio
        auto drawSize = this->histImageViewPanel->GetClientSize();

        if ((drawSize.GetWidth() == 0) || (drawSize.GetHeight() == 0))
        {
            return;
        }

        float ar = (float)drawSize.x / drawSize.y;
        ar = std::clamp(ar, 1.0f, 1.5f);                  // limited range aspect ratio
        int drawWidth = std::max(drawSize.x, drawSize.y); // approx res, because we will scale anyway
        int drawHeight = (int)(drawWidth / ar);

        // the hist image to render to and then display
        cv::Mat img;
        img.create(drawHeight, drawWidth, CV_8UC3);

        bool isLogScale = this->histLogCheckBox->GetValue();

        if (!imageHist.empty())
        {
            auto axes = CvPlot::makePlotAxes();
            axes.title(this->chartTitle);

            axes.setYTight(false, true);
            axes.setXTight(true);
            axes.yLabel("Counts");
            axes.xLabel("Values");

            // need some x margin to show whole bins, margin is in value-space apparently
            axes.setXMargin(imageHist.getBinSize() / 2);

            // convert bars to float
            vector<float> ys;
            ys.reserve(imageHist.getBinCount());

            for (int i = 0; i < imageHist.getBinCount(); i++)
            {
                float val = (float)imageHist.counts[i];

                if (isLogScale && (val > 0.0f))
                {
                    val = log10f(val);
                    ys.push_back(val);
                }
                else
                {
                    ys.push_back(val);
                }
            }

            // values
            CvPlot::Series& series = axes.create<CvPlot::Series>(imageHist.bins, ys, "-r");
            series.setMarkerType(CvPlot::MarkerType::Bar);
            series.setMarkerSize(0);                    // means make bars wide enough to touch each other
            series.setLineType(CvPlot::LineType::None); // okay to have both line and bar

            img = axes.render(drawHeight, drawWidth);
        }
        else
        {
            img = cv::Scalar(255, 255, 255);
        }

        this->histImageViewPanel->setImage(img);
        this->histImageViewPanel->setViewToFitImage();
    }
}
