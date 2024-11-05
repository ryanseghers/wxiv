// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "WxWidgetsUtil.h"
#include <wx/splitter.h>

#include <fmt/core.h>
#include <opencv2/opencv.hpp>

#include "PixelStatsPanel.h"
#include "ImageViewPanelSettings.h"
#include "ImageUtil.h"
#include "WxivImage.h"
#include <CvPlot/cvplot.h>

using namespace std;

namespace Wxiv
{
    // because mac listview has no padding for cell values
    const int StatsTableLabelIndex = 1;
    const int StatsTableValueIndex = 2;

    /**
     * @brief Constructor.
     * @param parent
     * @param imagePanel We keep this in order to re/compute stats.
     */
    PixelStatsPanel::PixelStatsPanel(wxWindow* parent, ImageScrollPanel* imagePanel) : wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize)
    {
        build();
        this->imageScrollPanel = imagePanel;
    }

    /**
     * @brief We keep this to get the pixels. This is the whole image, not just a roi (e.g. for the view roi case).
     */
    void PixelStatsPanel::setImage(std::shared_ptr<WxivImage> p)
    {
        this->currentImage = p;
        this->refreshStats();
    }

    void PixelStatsPanel::saveConfig()
    {
        wxConfigBase::Get()->Write("PixelStatsPanelSashPosition", mainSplitter->GetSashPosition());
        wxConfigBase::Get()->Write("PixelStatsPanelImageSubjectChoiceInt", this->subjectComboBox->GetSelection());
        this->histChartPanel->saveConfig("PixelStatsPanel");
    }

    void PixelStatsPanel::restoreConfig()
    {
        int sashPosition = wxConfigBase::Get()->Read("PixelStatsPanelSashPosition", 300);
        this->mainSplitter->SetSashPosition(sashPosition);

        wxString choiceStr = "Whole Image";
        int restoreChoiceIdx = wxConfigBase::Get()->ReadLong("PixelStatsPanelImageSubjectChoiceInt", defaultChoice);
        this->subjectComboBox->SetSelection(restoreChoiceIdx);

        this->histChartPanel->restoreConfig("PixelStatsPanel");
    }

    std::string PixelStatsPanel::getSubjectString()
    {
        int idx = this->subjectComboBox->GetSelection();
        return choices[idx].ToStdString();
    }

    /**
     * wxBoxSizer vert sizer
     *      subject combo box
     *      wxSplitterWindow vert splitter
     *          wxListView table of stats
     *          HistChartPanel
     */
    void PixelStatsPanel::build()
    {
        const int defaultBorder = 4;
        auto sizer = new wxBoxSizer(wxVERTICAL);

        // subject combo box
        this->subjectComboBox = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, sizeof(choices) / sizeof(wxString), choices);
        this->subjectComboBox->SetSelection(defaultChoice);

        sizer->Add(subjectComboBox, 0, wxEXPAND | wxALL, defaultBorder);
        this->Bind(wxEVT_CHOICE, &PixelStatsPanel::onSubjectChange, this, wxID_ANY);

        // vert splitter for table and hist plot
        mainSplitter = new wxSplitterWindow(this, wxID_ANY, wxPoint(0, 400), wxDefaultSize, wxSP_BORDER | wxSP_LIVE_UPDATE);
        sizer->Add(mainSplitter, 1, wxEXPAND | wxALL, defaultBorder);

        // stats table
        this->statsTableListView = new wxListView(mainSplitter);
        this->statsTableListView->AppendColumn("");
        this->statsTableListView->AppendColumn("Stat");
        this->statsTableListView->AppendColumn("Value");

        // mac listview has 0 spacing on left of labels, so add another column just for spacing
        int spacerColWidth = 0;

#ifdef __APPLE__
        spacerColWidth = 8;
#endif

        this->statsTableListView->SetColumnWidth(0, spacerColWidth);
        this->statsTableListView->SetColumnWidth(1, 90);
        this->statsTableListView->SetColumnWidth(StatsTableValueIndex, 100);

        // initial rows
        int rowIdx = 0;
        statsTableListView->InsertItem(rowIdx++, "");
        statsTableListView->InsertItem(rowIdx++, "");
        statsTableListView->InsertItem(rowIdx++, "");
        statsTableListView->InsertItem(rowIdx++, "");
        statsTableListView->InsertItem(rowIdx++, "");
        statsTableListView->InsertItem(rowIdx++, "");
        statsTableListView->InsertItem(rowIdx++, "");

        rowIdx = 0;
        statsTableListView->SetItem(rowIdx++, StatsTableLabelIndex, "Type");
        statsTableListView->SetItem(rowIdx++, StatsTableLabelIndex, "Width");
        statsTableListView->SetItem(rowIdx++, StatsTableLabelIndex, "Height");
        statsTableListView->SetItem(rowIdx++, StatsTableLabelIndex, "Non-zero");
        statsTableListView->SetItem(rowIdx++, StatsTableLabelIndex, "Sum");
        statsTableListView->SetItem(rowIdx++, StatsTableLabelIndex, "Min");
        statsTableListView->SetItem(rowIdx++, StatsTableLabelIndex, "Max");

        // hist chart panel
        this->histChartPanel = new HistChartPanel(mainSplitter, false, [&](wxString selection, int binCount, float min, float max, FloatHist& hist) {
            this->histComputeCallback(selection, binCount, min, max, hist);
        });

        mainSplitter->SetMinimumPaneSize(100);
        mainSplitter->SplitHorizontally(this->statsTableListView, histChartPanel);

        this->SetSizerAndFit(sizer);
    }

    void PixelStatsPanel::onSubjectChange(wxCommandEvent& event)
    {
        this->refreshStats();
    }

    void PixelStatsPanel::histComputeCallback(wxString selection, int binCount, float min, float max, FloatHist& hist)
    {
        this->computeHistogram(binCount, min, max, hist);
    }

    /**
     * @brief Compute whole, view, or roi histogram.
     * @param hist Output.
     * @param bins
     */
    void PixelStatsPanel::computeHistogram(int binCount, float min, float max, FloatHist& hist)
    {
        if (this->currentImage)
        {
            std::string subjectStr = this->getSubjectString();

            // get the image (portion) to compute hist on
            cv::Mat img;

            if (subjectStr == "Whole Image")
            {
                img = this->currentImage->getImage();

                // image may already have a hist, see if same
                FloatHist& imgHist = this->currentImage->getFloatHist();

                if (!imgHist.empty() && (std::isnan(min) || (imgHist.getMin() == min)) && (std::isnan(max) || (imgHist.getMax() == max)) &&
                    (imgHist.getBinCount() == binCount))
                {
                    // already have this hist
                    hist.copy(imgHist);
                    return;
                }
            }
            else if (subjectStr == "View ROI")
            {
                img = this->imageScrollPanel->getOrigViewImage();
            }
            else if (subjectStr == "Drawn ROI")
            {
                cv::Rect2f roi = this->imageScrollPanel->getDrawnRoi();

                if (!roi.empty())
                {
                    img = this->imageScrollPanel->getImage()(roi);
                }
            }

            if (!img.empty() && (img.channels() == 1))
            {
                ImageUtil::histFloat(img, binCount, min, max, hist);

                if (subjectStr == "Whole Image")
                {
                    // we computed on whole image, save for next time
                    this->currentImage->setFloatHist(hist);
                }
            }
        }
    }

    void PixelStatsPanel::updateHistogram()
    {
        // not doing color hists
        bool isImageTypeHandled = true;

        if (this->currentImage && !this->currentImage->empty() && (this->currentImage->getImage().channels() != 1))
        {
            isImageTypeHandled = false;
        }

        std::string subjectStr = this->getSubjectString();

        if (isImageTypeHandled && !subjectStr.empty() && (subjectStr != "None"))
        {
            this->histChartPanel->refreshControls();
        }
        else
        {
            this->histChartPanel->clear();
        }
    }

    void PixelStatsPanel::refreshStats()
    {
        if (this->currentImage)
        {
            std::string subjectStr = this->getSubjectString();
            ImageUtil::ImageStats stats;

            if (subjectStr.empty() || (subjectStr == "None"))
            {
                for (int rowIdx = 0; rowIdx < 7; rowIdx++)
                {
                    this->statsTableListView->SetItem(rowIdx, StatsTableValueIndex, std::string());
                }
            }
            else
            {
                if (subjectStr == "Whole Image")
                {
                    // compute and cache whole-image stats
                    ImageUtil::ImageStats& currentImageStats = this->currentImage->getImageStats();

                    if (currentImageStats.empty())
                    {
                        currentImageStats = ImageUtil::computeStats(this->currentImage->getImage());
                    }

                    stats = currentImageStats;
                }
                else if (subjectStr == "View ROI")
                {
                    cv::Mat viewImage = this->imageScrollPanel->getOrigViewImage();
                    stats = ImageUtil::computeStats(viewImage);
                }
                else if (subjectStr == "Drawn ROI")
                {
                    cv::Rect2f roi = this->imageScrollPanel->getDrawnRoi();

                    if (!roi.empty())
                    {
                        cv::Mat viewImage = this->imageScrollPanel->getImage()(roi);
                        stats = ImageUtil::computeStats(viewImage);
                    }
                }

                // put in list view
                int rowIdx = 0;
                this->statsTableListView->SetItem(rowIdx++, StatsTableValueIndex, ImageUtil::getImageTypeString(stats.type));
                this->statsTableListView->SetItem(rowIdx++, StatsTableValueIndex, fmt::format("{}", stats.width));
                this->statsTableListView->SetItem(rowIdx++, StatsTableValueIndex, fmt::format("{}", stats.height));
                this->statsTableListView->SetItem(rowIdx++, StatsTableValueIndex, fmt::format("{}", stats.nonzeroCount));

                std::string fmtStr = "{:.0f}";

                if (stats.type == CV_32F)
                {
                    // Wouldn't normally want this many digits, but not sure how to know how many to show
                    fmtStr = "{:.9f}";
                }

                this->statsTableListView->SetItem(rowIdx++, StatsTableValueIndex, fmt::format(fmt::runtime(fmtStr), stats.sum));
                this->statsTableListView->SetItem(rowIdx++, StatsTableValueIndex, fmt::format(fmt::runtime(fmtStr), stats.minVal));
                this->statsTableListView->SetItem(rowIdx++, StatsTableValueIndex, fmt::format(fmt::runtime(fmtStr), stats.maxVal));
            }

            this->updateHistogram();
        }
    }
}
