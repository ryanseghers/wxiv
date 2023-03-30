// Copyright(c) 2023 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "WxWidgetsUtil.h"
#include <wx/splitter.h>

#include <fmt/core.h>
#include <opencv2/opencv.hpp>

#include "ProfilesPanel.h"
#include "ImageViewPanelSettings.h"
#include "ImageUtil.h"
#include "WxivImage.h"
#include "VectorUtil.h"
#include <CvPlot/cvplot.h>

using namespace std;

namespace Wxiv
{
    /**
    * @brief Constructor.
    * @param parent
    * @param imagePanel We keep this in order to re/compute stats.
    */
    ProfilesPanel::ProfilesPanel(wxWindow* parent, ImageScrollPanel* imagePanel) : wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize)
    {
        build();
        this->imageScrollPanel = imagePanel;
    }

    /**
    * @brief We keep this to get the pixels. This is the whole image, not just a roi (e.g. for the view roi case).
    */
    void ProfilesPanel::setImage(std::shared_ptr<WxivImage> p)
    {
        this->currentImage = p;
        this->refreshStats();
    }

    void ProfilesPanel::saveConfig()
    {
        wxConfigBase::Get()->Write("ProfilesPanelImageSubjectChoiceInt", this->subjectComboBox->GetSelection());
    }

    void ProfilesPanel::restoreConfig()
    {
        wxString choiceStr = "None";
        int restoreChoiceIdx = wxConfigBase::Get()->ReadLong("ProfilesPanelImageSubjectChoiceInt", defaultChoice);
        this->subjectComboBox->SetSelection(restoreChoiceIdx);
    }

    std::string ProfilesPanel::getSubjectString()
    {
        int idx = this->subjectComboBox->GetSelection();
        return choices[idx].ToStdString();
    }

    /**
    * wxBoxSizer vert sizer
    *      subject combo box
    *      ImageViewPanel colProfilePanel
    *      ImageViewPanel rowProfilePanel
    */
    void ProfilesPanel::build()
    {
        const int defaultBorder = 4;
        auto sizer = new wxBoxSizer(wxVERTICAL);

        // subject combo box
        this->subjectComboBox = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, sizeof(choices) / sizeof(wxString), choices);
        this->subjectComboBox->SetSelection(defaultChoice);

        sizer->Add(subjectComboBox, 0, wxEXPAND | wxALL, defaultBorder);
        this->Bind(wxEVT_CHOICE, &ProfilesPanel::onSubjectChange, this, wxID_ANY);

        this->colProfilePanel = new ImageViewPanel(this);
        this->colProfilePanel->setBackground(255);
        sizer->Add(this->colProfilePanel, 1, wxEXPAND | wxALL, defaultBorder);

        this->rowProfilePanel = new ImageViewPanel(this);
        this->rowProfilePanel->setBackground(255);
        sizer->Add(this->rowProfilePanel, 1, wxEXPAND | wxALL, defaultBorder);

        this->SetSizerAndFit(sizer);
    }

    void ProfilesPanel::onSubjectChange(wxCommandEvent& event)
    {
        this->refreshStats();
    }

    void renderToPanel(std::string chartTitle, ImageViewPanel* imageViewPanel, vector<int>& xs, vector<float>& ys)
    {
        // fit to panel aspect ratio
        auto drawSize = imageViewPanel->GetClientSize();

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

        if (!xs.empty() && !ys.empty())
        {
            auto axes = CvPlot::makePlotAxes();
            axes.title(chartTitle);

            axes.setYTight(false, true);
            axes.setXTight(true);
            axes.yLabel("Sums");

            // need some x margin to show whole bins, margin is in value-space apparently
            axes.setXMargin(0.5f);

            // plot
            CvPlot::Series& series = axes.create<CvPlot::Series>(xs, ys, "-r");
            series.setMarkerType(CvPlot::MarkerType::Bar);
            series.setMarkerSize(0);                    // means make bars wide enough to touch each other
            series.setLineType(CvPlot::LineType::None); // okay to have both line and bar

            img = axes.render(drawHeight, drawWidth);
        }
        else
        {
            img = cv::Scalar(255, 255, 255);
        }

        imageViewPanel->setImage(img);
        imageViewPanel->setViewToFitImage();    
    }

    void ProfilesPanel::refreshStats()
    {
        if (this->currentImage)
        {
            std::string subjectStr = this->getSubjectString();
            vector<int> colIndices, rowIndices;
            vector<float> colProfile, rowProfile;

            if (!subjectStr.empty() && (subjectStr != "None"))
            {
                cv::Mat wholeImage = this->currentImage->getImage();
                cv::Mat viewImage;

                if (subjectStr == "Whole Image")
                {
                    viewImage = wholeImage;
                    rowIndices = vectorRange(0, viewImage.rows);
                    colIndices = vectorRange(0, viewImage.cols);
                }
                else if (subjectStr == "View ROI")
                {
                    viewImage = this->imageScrollPanel->getOrigViewImage();
                    wxRect roi = this->imageScrollPanel->getViewRoi(); // can be off-image
                    wxRect imageOnlyViewRoi = roi.Intersect(wxRect(0, 0, wholeImage.cols, wholeImage.rows));
                    rowIndices = vectorRange(imageOnlyViewRoi.y, imageOnlyViewRoi.height);
                    colIndices = vectorRange(imageOnlyViewRoi.x, imageOnlyViewRoi.width);
                }
                else if (subjectStr == "Drawn ROI")
                {
                    cv::Rect2f roi = this->imageScrollPanel->getDrawnRoi();
                    cv::Rect2i iroi((int)roi.x, (int)roi.y, (int)roi.width, (int)roi.height);

                    if (!roi.empty())
                    {
                        viewImage = this->currentImage->getImage()(iroi);
                        rowIndices = vectorRange<int>((int)iroi.y, (int)iroi.height);
                        colIndices = vectorRange<int>((int)iroi.x, (int)iroi.width);
                    }
                }

                if (!viewImage.empty())
                {
                    ImageUtil::profile(viewImage, false, rowProfile);
                    ImageUtil::profile(viewImage, true, colProfile);
                }
            }

            // these clear the panel if either vector is empty
            renderToPanel("Column/Vertical Profile", colProfilePanel, colIndices, colProfile);
            renderToPanel("Row/Horizontal Profile", rowProfilePanel, rowIndices, rowProfile);
        }
    }
}
