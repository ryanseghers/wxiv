// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <string>
#include <vector>
#include <cmath>
#include <fmt/core.h>
#include <wx/wxprec.h>
#include <wx/splitter.h>
#include <wx/config.h>
#include <wx/grid.h>
#include <wx/confbase.h>
#include <wx/headerctrl.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <opencv2/opencv.hpp>
#include <CvPlot/cvplot.h>

#include "WxivMainSplitWindow.h"
#include "ImageUtil.h"
#include "MiscUtil.h"

using namespace std;

namespace Wxiv
{
    WxivMainSplitWindow::WxivMainSplitWindow(wxWindow* parent) : wxWindow(parent, -1, wxDefaultPosition, wxDefaultSize, wxALWAYS_SHOW_SB)
    {
        auto topSizer = new wxBoxSizer(wxHORIZONTAL);

        // left-right splitter
#ifdef __APPLE__
        mainSplitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
#else
        mainSplitter = new wxSplitterWindow(this, wxID_ANY, wxPoint(0, 0), wxSize(500, 900), wxSP_BORDER | wxSP_LIVE_UPDATE);
#endif
        topSizer->Add(mainSplitter, 1, wxEXPAND | wxALL, 5);

        // right image panel
        this->imageScrollPanel = new ImageScrollPanel(mainSplitter);
        this->imageScrollPanel->setOnViewChangeCallback([&](void) { this->onImageViewChange(); });
        this->imageScrollPanel->setOnDrawnRoiChangeCallback([&](void) { this->onDrawnRoiChange(); });

        // left notebook
        this->notebook = new wxNotebook(mainSplitter, wxID_ANY);

        this->statsPanel = new PixelStatsPanel(this->notebook, this->imageScrollPanel);
        this->notebook->AddPage(statsPanel, "Statistics", true);

        this->shapesPanel = new ShapeMetadataPanel(this->notebook);
        this->shapesPanel->setOnShapeFilterChangeCallback([&](ShapeSet& newShapes) { this->onShapeFilterChange(newShapes); });
        this->notebook->AddPage(shapesPanel, "Shapes", false);
        this->imageScrollPanel->setOnMouseOverShapeChangeCallback(
            [&](ShapeType type, int idx) { this->shapesPanel->onMouseOverShapeChange(type, idx); });

        mainSplitter->SetMinimumPaneSize(50);
        mainSplitter->SplitVertically(notebook, imageScrollPanel);
        this->SetSizerAndFit(topSizer);
    }

    void WxivMainSplitWindow::onShapeFilterChange(ShapeSet& newShapes)
    {
        this->imageScrollPanel->setShapes(newShapes);
    }

    void WxivMainSplitWindow::saveConfig()
    {
        wxConfigBase::Get()->Write("WxivMainSplitWindowSashPosition", mainSplitter->GetSashPosition());
        wxConfigBase::Get()->Write("WxivMainSplitWindowNotebookPage", this->notebook->GetSelection());

        this->statsPanel->saveConfig();
        this->shapesPanel->saveConfig();

        ImageViewPanelSettings settings = this->imageScrollPanel->getSettings();
        settings.writeConfig(wxConfigBase::Get());
    }

    void WxivMainSplitWindow::restoreConfig()
    {
        int sashPosition = wxConfigBase::Get()->Read("WxivMainSplitWindowSashPosition", 300L);
        this->mainSplitter->SetSashPosition(sashPosition);

        int notebookPage = wxConfigBase::Get()->Read("WxivMainSplitWindowNotebookPage", 0L);
        this->notebook->SetSelection(notebookPage);

        this->statsPanel->restoreConfig();
        this->shapesPanel->restoreConfig();

        ImageViewPanelSettings imageViewPanelSettings;
        imageViewPanelSettings.loadConfig(wxConfigBase::Get());
        this->imageScrollPanel->setSettings(imageViewPanelSettings);
    }

    int WxivMainSplitWindow::getSashPosition()
    {
        return this->mainSplitter->GetSashPosition();
    }

    void WxivMainSplitWindow::onImageViewChange()
    {
        this->statsPanel->refreshStats();
    }

    void WxivMainSplitWindow::onDrawnRoiChange()
    {
        this->statsPanel->refreshStats();
    }

    void WxivMainSplitWindow::setViewToFitImage()
    {
        this->imageScrollPanel->setViewToFitImage();
    }

    void WxivMainSplitWindow::setImage(std::shared_ptr<WxivImage> newImage)
    {
        this->currentImage = newImage;
        this->statsPanel->setImage(newImage);

        // temporarily turn off callback because we will handle the update ourself and we don't know
        // if imageScrollPanel is going to event a view change (because setting same image size doesn't change the "view")
        this->imageScrollPanel->setOnViewChangeCallback(nullptr);
        this->imageScrollPanel->setImage(newImage->getImage());
        this->imageScrollPanel->setOnViewChangeCallback([&](void) { this->onImageViewChange(); });

        this->onImageViewChange();
        this->onCurrentImageShapesChange();
    }

    /**
     * @brief For example when filter is applied.
     */
    void WxivMainSplitWindow::onCurrentImageShapesChange()
    {
        if (this->currentImage != nullptr)
        {
            this->imageScrollPanel->setShapes(this->currentImage->getShapes());
            this->shapesPanel->setShapes(this->currentImage->getShapes());
        }
        else
        {
            ShapeSet noShapes;
            this->imageScrollPanel->setShapes(noShapes);
            this->shapesPanel->setShapes(noShapes);
        }
    }

    cv::Mat WxivMainSplitWindow::getCurrentImage()
    {
        return this->imageScrollPanel->getImage();
    }

    cv::Mat WxivMainSplitWindow::getCurrentViewImageClone()
    {
        return this->imageScrollPanel->getViewImageClone();
    }

    wxImage WxivMainSplitWindow::renderToWxImage(std::shared_ptr<WxivImage> image)
    {
        return this->imageScrollPanel->renderToWxImage(image);
    }

    /**
     * @brief Stop showing any image, e.g. user has de-selected or multi-selected in the image list.
     */
    void WxivMainSplitWindow::clearImage()
    {
        this->imageScrollPanel->clearImage();
        this->currentImage = nullptr;
        this->statsPanel->setImage(this->currentImage);
        this->shapesPanel->clearShapes();
    }

    /**
     * @brief Ptr to orig so short lifetime.
     * @return
     */
    wxBitmap WxivMainSplitWindow::getCurrentViewImageBitmap()
    {
        return this->imageScrollPanel->getViewBitmap();
    }

    wxImage WxivMainSplitWindow::getViewWxImageClone()
    {
        return this->imageScrollPanel->getViewWxImageClone();
    }

    void WxivMainSplitWindow::setRenderShapes(bool doRender)
    {
        if (this->imageScrollPanel)
        {
            this->imageScrollPanel->setRenderShapes(doRender);
        }
    }

    bool WxivMainSplitWindow::getRenderShapes()
    {
        if (this->imageScrollPanel)
        {
            return this->imageScrollPanel->getRenderShapes();
        }
        else
        {
            return true;
        }
    }
}
