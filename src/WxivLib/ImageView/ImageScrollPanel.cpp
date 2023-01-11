// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <string>

#include <opencv2/opencv.hpp>
#include <fmt/core.h>

#include "WxWidgetsUtil.h"
#include <wx/splitter.h>

#include "ImageScrollPanel.h"
#include "ImageUtil.h"
#include "ImageViewPanelSettingsPanel.h"

using namespace std;

namespace Wxiv
{
    ImageScrollPanel::ImageScrollPanel(wxWindow* parent) : wxWindow(parent, -1, wxDefaultPosition, wxDefaultSize, wxALWAYS_SHOW_SB)
    {
        build();
    }

    void ImageScrollPanel::buildToolbar()
    {
        auto toolbarSizer = new wxBoxSizer(wxHORIZONTAL);
        this->toolbarPanel = new wxPanel(this);

        const int labelBorderFlags = wxALL;
        const int labelBorder = 4;
        const int textBoxBorderFlags = wxALL;
        const int textBoxBorder = 4;

        // image type
#ifdef __linux__
        this->imageTypeTextBox = new wxStaticText(this->toolbarPanel, wxID_ANY, wxEmptyString);
        this->mousePosTextBox = new wxStaticText(this->toolbarPanel, wxID_ANY, wxEmptyString);
        this->pixelValueTextBox = new wxStaticText(this->toolbarPanel, wxID_ANY, wxEmptyString);
        this->intensityRangeTextBox = new wxStaticText(this->toolbarPanel, wxID_ANY, wxEmptyString);
#else
        this->imageTypeTextBox = new wxTextCtrl(this->toolbarPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
        this->mousePosTextBox = new wxTextCtrl(this->toolbarPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
        this->pixelValueTextBox = new wxTextCtrl(this->toolbarPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
        this->intensityRangeTextBox = new wxTextCtrl(this->toolbarPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
#endif

        toolbarSizer->Add(new wxStaticText(this->toolbarPanel, wxID_ANY, wxString("WxivImage type")), 0, wxFIXED | labelBorderFlags, labelBorder);
        toolbarSizer->Add(this->imageTypeTextBox, 1, wxFIXED | textBoxBorderFlags, textBoxBorder);

        // mouse position
        toolbarSizer->Add(new wxStaticText(this->toolbarPanel, wxID_ANY, wxString("Mouse Position")), 0, wxFIXED | labelBorderFlags, labelBorder);
        toolbarSizer->Add(this->mousePosTextBox, 1, wxFIXED | textBoxBorderFlags, textBoxBorder);

        // pixel value
        toolbarSizer->Add(new wxStaticText(this->toolbarPanel, wxID_ANY, wxString("Pixel Value")), 0, wxFIXED | labelBorderFlags, labelBorder);
        toolbarSizer->Add(this->pixelValueTextBox, 1, wxFIXED | textBoxBorderFlags, textBoxBorder);

        // intensity range
        toolbarSizer->Add(new wxStaticText(this->toolbarPanel, wxID_ANY, wxString("Intensity Range")), 0, wxFIXED | labelBorderFlags, labelBorder);
        toolbarSizer->Add(this->intensityRangeTextBox, 1, wxFIXED | textBoxBorderFlags, textBoxBorder);

        // settings button
        wxButton* settingsButton = new wxButton(this->toolbarPanel, wxID_ANY, "Settings");
        toolbarSizer->Add(settingsButton, 0, wxFIXED | textBoxBorderFlags, textBoxBorder);
        this->Bind(wxEVT_BUTTON, &ImageScrollPanel::doSettingsDialog, this);

        this->toolbarPanel->SetSizerAndFit(toolbarSizer);
    }

    void ImageScrollPanel::build()
    {
        this->buildToolbar();

        this->panel = new ImageViewPanel(this);
        this->panel->setOnRenderCallback([&](void) { this->onImageRender(); });

        // sizer hierarchy with one for the main panel and horizontal scrollbar below it,
        // then another for the first sizer with a vert scrollbar to the right
        this->hScrollBar = new wxScrollBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_HORIZONTAL);
        this->hScrollBar->SetRange(100);
        this->hScrollBar->SetThumbPosition(0);
        this->hScrollBar->SetThumbSize(100);

        // sizer with main panel and horizontal scrollbar under it
        auto vertSizer = new wxBoxSizer(wxVERTICAL);
        vertSizer->Add(this->toolbarPanel, 0, wxEXPAND, 0);
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
        Bind(wxEVT_SIZE, &ImageScrollPanel::onSize, this, wxID_ANY);
        Bind(wxEVT_MOUSEWHEEL, &ImageScrollPanel::onMouseWheelMoved, this, wxID_ANY);
        Bind(wxEVT_SCROLL_CHANGED, &ImageScrollPanel::onScrollbarChanged, this, wxID_ANY);
        Bind(wxEVT_SCROLL_THUMBTRACK, &ImageScrollPanel::onScrollbarThumbTrack, this, wxID_ANY);

        this->panel->Bind(wxEVT_MOTION, &ImageScrollPanel::onMouseMoved, this, wxID_ANY);
        this->panel->Bind(wxEVT_KEY_DOWN, &ImageScrollPanel::onKeyDown, this, wxID_ANY);
        this->panel->Bind(wxEVT_LEFT_UP, &ImageScrollPanel::onMouseLeftUp, this, wxID_ANY);
    }

    void ImageScrollPanel::doSettingsDialog(wxCommandEvent& event)
    {
        wxDialog dlg(this, wxID_ANY, "WxivImage View Panel Settings");

        auto vertSizer = new wxBoxSizer(wxVERTICAL);

        // settings panel on the dialog
        ImageViewPanelSettingsPanel* settingsPanel = new ImageViewPanelSettingsPanel(&dlg, this->panel->getSettings());
        wxSizer* buttonSizer = dlg.CreateButtonSizer(wxCANCEL | wxOK);
        vertSizer->Add(settingsPanel);

        const int borderFlags = wxALL;
        const int borderWidth = 12;
        vertSizer->Add(buttonSizer, 0, borderFlags, borderWidth);
        dlg.SetSizerAndFit(vertSizer);

        // position near the button
        wxPoint pos = this->toolbarPanel->GetScreenPosition();
        pos.x += this->toolbarPanel->GetSize().x - dlg.GetSize().x;
        pos.y += this->toolbarPanel->GetSize().y;
        dlg.SetPosition(pos);

        if (dlg.ShowModal() == wxID_OK)
        {
            ImageViewPanelSettings settings = settingsPanel->getSettings();

            // give to the ImageViewPanel
            this->panel->setSettings(settings);
        }
    }

    ImageViewPanelSettings ImageScrollPanel::getSettings()
    {
        return this->panel->getSettings();
    }

    void ImageScrollPanel::setSettings(ImageViewPanelSettings newSettings)
    {
        this->panel->setSettings(newSettings);
    }

    void ImageScrollPanel::setOnViewChangeCallback(const std::function<void(void)>& f)
    {
        this->onViewChangeCallback = f;
    }

    void ImageScrollPanel::setOnDrawnRoiChangeCallback(const std::function<void(void)>& f)
    {
        this->onDrawnRoiChangeCallback = f;
    }

    void ImageScrollPanel::setOnMouseOverShapeChangeCallback(const std::function<void(ShapeType, int)>& f)
    {
        this->onMouseOverShapeChangeCallback = f;
    }

    bool ImageScrollPanel::checkHasImage()
    {
        return this->panel->checkHasImage();
    }

    void ImageScrollPanel::setImage(cv::Mat& newImage)
    {
        cv::Size2i newSize(newImage.cols, newImage.rows);
        this->panel->setImage(newImage);

        if (newSize != this->currentImageSize)
        {
            this->setViewToFitImage();
        }

        this->currentImageSize = cv::Size2i(newImage.cols, newImage.rows);

        this->imageTypeTextBox->SetLabelText(wxString(ImageUtil::getImageDescString(newImage)));

        // ensure drawnRoi is on image
        cv::Rect2f drawnRoi = this->panel->getDrawnRoi();

        if (!drawnRoi.empty())
        {
            cv::Rect2f imageRoi(0, 0, newImage.cols, newImage.rows);
            drawnRoi = drawnRoi & imageRoi;
            this->panel->setDrawnRoi(drawnRoi);
        }
    }

    void ImageScrollPanel::clearImage()
    {
        this->panel->clearImage();
        this->panel->setDrawnRoi(cv::Rect2f());
    }

    cv::Mat ImageScrollPanel::getImage()
    {
        return this->panel->getImage();
    }

    void ImageScrollPanel::setShapes(ShapeSet& set)
    {
        this->panel->setShapes(set);
    }

    cv::Rect2f ImageScrollPanel::getDrawnRoi()
    {
        return this->panel->getDrawnRoi();
    }

    void ImageScrollPanel::setViewToFitImage()
    {
        this->viewPoint = wxPoint(0, 0);
        this->zoomFactor = this->panel->getZoomToFitImage();
        this->updateView();
    }

    wxRect ImageScrollPanel::getViewRoi()
    {
        return this->panel->getViewRoi();
    }

    cv::Mat ImageScrollPanel::getOrigViewImage()
    {
        // don't need a clone if we will select roi
        cv::Mat image = this->panel->getImage();

        if (image.empty())
        {
            return image;
        }

        // get the portion of image that is displayed
        // Note that view roi is often off-image.
        wxRect wxRoiView = this->getViewRoi();
        wxRect wxRoiImage = wxRect(0, 0, image.cols, image.rows);
        wxRect wxRoi = wxRoiView.Intersect(wxRoiImage);

        cv::Rect2i roi = cv::Rect2i(wxRoi.x, wxRoi.y, wxRoi.width, wxRoi.height);
        return image(roi);
    }

    cv::Mat ImageScrollPanel::getViewImageClone()
    {
        return this->panel->getRenderedImageClone();
    }

    wxBitmap ImageScrollPanel::getViewBitmap()
    {
        return this->panel->getViewBitmap();
    }

    wxImage ImageScrollPanel::renderToWxImage(std::shared_ptr<WxivImage> image)
    {
        return this->panel->renderToWxImage(image->getShapes(), image->getImage());
    }

    void ImageScrollPanel::updateView()
    {
        // clip viewpoint to image size
        wxSize fullImageSize = panel->getFullImageSize();

        if (fullImageSize.x > 0)
        {
            this->viewPoint.x = std::clamp(this->viewPoint.x, 0, fullImageSize.x - 1);
            this->viewPoint.y = std::clamp(this->viewPoint.y, 0, fullImageSize.y - 1);

            this->panel->setView(this->viewPoint, this->zoomFactor);
            this->updateScrollbars();

            if (this->onViewChangeCallback)
            {
                this->onViewChangeCallback();
            }
        }
    }

    void ImageScrollPanel::onSize(wxSizeEvent& event)
    {
        this->updateView();
        event.Skip();
    }

    void ImageScrollPanel::onMouseLeftUp(wxMouseEvent& event)
    {
        if (isDrawing)
        {
            // done draw
            wxRealPoint imagePoint;

            if (this->panel->pointToOrigImageCoords(wxPoint(event.GetX(), event.GetY()), imagePoint))
            {
                cv::Rect2f drawnRoi = this->panel->getDrawnRoi();
                drawnRoi.width = imagePoint.x - drawnRoi.x;
                drawnRoi.height = imagePoint.y - drawnRoi.y;
                this->panel->setDrawnRoi(drawnRoi);
                isDrawing = false;

                if (this->onDrawnRoiChangeCallback)
                {
                    this->onDrawnRoiChangeCallback();
                }
            }
        }
    }

    void ImageScrollPanel::onMouseMoved(wxMouseEvent& event)
    {
        wxPoint mousePoint(event.GetX(), event.GetY());
        wxRealPoint imagePoint;

        if (this->panel->pointToOrigImageCoords(mousePoint, imagePoint))
        {
            if (event.LeftIsDown())
            {
                cv::Rect2f drawnRoi = this->panel->getDrawnRoi();

                if (!isDrawing)
                {
                    // start draw
                    isDrawing = true;
                    drawnRoi.x = imagePoint.x;
                    drawnRoi.y = imagePoint.y;
                }
                else
                {
                    // already drawing
                    drawnRoi.width = imagePoint.x - drawnRoi.x;
                    drawnRoi.height = imagePoint.y - drawnRoi.y;
                }

                this->panel->setDrawnRoi(drawnRoi);
            }
            else
            {
                std::string s;

                if (this->zoomFactor > 4.0f)
                {
                    s = fmt::format("{:.1f}, {:.1f}", imagePoint.x, imagePoint.y);
                }
                else
                {
                    s = fmt::format("{}, {}", (int)lroundf(imagePoint.x), (int)lroundf(imagePoint.y));
                }

                this->mousePosTextBox->SetLabelText(wxString(s));

                s = this->panel->getPixelValueString(imagePoint);
                this->pixelValueTextBox->SetLabelText(wxString(s));
                this->updateMouseOverShape(imagePoint);
            }
        }
    }

    void ImageScrollPanel::updateMouseOverShape(wxRealPoint imagePoint)
    {
        ShapeType type;
        int idx;
        ShapeSet& shapes = this->panel->getShapes();

        shapes.findShape((float)imagePoint.x, (float)imagePoint.y, 5.0f, type, idx); // idx == -1 for none found, so we keep that convention

        if ((type != lastMouseOverShapeType) || (idx != lastMouseOverShapeIndex))
        {
            if (this->onMouseOverShapeChangeCallback)
            {
                this->onMouseOverShapeChangeCallback(type, idx);
            }

            lastMouseOverShapeType = type;
            lastMouseOverShapeIndex = idx;
        }
    }

    void ImageScrollPanel::onImageRender()
    {
        std::tuple<float, float> intensityRange = this->panel->getLastIntensityRange();
        string s = fmt::format("{:.0f} to {:.0f}", std::get<0>(intensityRange), std::get<1>(intensityRange));
        this->intensityRangeTextBox->SetLabelText(wxString(s));
    }

    /**
     * @brief Pan the view either left/right or up/down.
     * @param doVert
     * @param doLeft
     */
    void ImageScrollPanel::panView(bool doVert, bool doUpLeft, bool doFullPage = false)
    {
        wxPoint origViewPoint = this->viewPoint;
        wxSize drawSize = panel->GetClientSize();
        wxSize fullImageSize = panel->getFullImageSize();
        wxRect panelViewRoi = panel->getViewRoi();

        int panFraction = doFullPage ? 1 : mouseWheelScrollFraction;

        if (doVert)
        {
            // vertical
            int pxMag = std::max(1, panelViewRoi.GetHeight() / panFraction);
            pxMag *= doUpLeft ? 1 : -1;

            this->viewPoint.y -= pxMag;

            // clip
            this->viewPoint.y = std::max(this->viewPoint.y, 0);

            int maxVal = fullImageSize.GetHeight() - drawSize.GetHeight() / this->zoomFactor;
            this->viewPoint.y = std::min(this->viewPoint.y, maxVal);
        }
        else
        {
            // horizontal
            // r is 120 or -120
            int pxMag = std::max(1, panelViewRoi.GetWidth() / panFraction);
            pxMag *= doUpLeft ? 1 : -1;

            this->viewPoint.x -= pxMag;

            // clip
            this->viewPoint.x = std::max(this->viewPoint.x, 0);

            int maxVal = fullImageSize.GetWidth() - drawSize.GetWidth() / this->zoomFactor;
            this->viewPoint.x = std::min(this->viewPoint.x, maxVal);
        }

        if (origViewPoint != this->viewPoint)
        {
            this->updateView();
        }
    }

    void ImageScrollPanel::zoomView(wxPoint mousePoint, bool doZoomIn)
    {
        wxSize drawSize = panel->GetClientSize();
        wxSize fullImageSize = panel->getFullImageSize();

        // zoom
        if (doZoomIn)
        {
            if (this->zoomFactor < this->maxZoom)
            {
                // zoom in
                this->zoomFactor *= zoomMultiplier;

                // also adjust viewpoint to zoom around mouse point
                wxRealPoint imgMousePoint;

                if (this->panel->pointToOrigImageCoords(mousePoint, imgMousePoint))
                {
                    this->viewPoint.x = imgMousePoint.x - (int)(mousePoint.x / this->zoomFactor);
                    this->viewPoint.y = imgMousePoint.y - (int)(mousePoint.y / this->zoomFactor);

                    this->updateView();
                }
            }
        }
        else
        {
            // limit zoom to one step past where whole image is visible in both axes
            float xZoomMin = (float)drawSize.x / fullImageSize.x;
            float yZoomMin = (float)drawSize.y / fullImageSize.y;

            if ((this->zoomFactor > xZoomMin) || (this->zoomFactor > yZoomMin))
            {
                // zoom out
                this->zoomFactor /= zoomMultiplier;

                // also adjust viewpoint to zoom around mouse point
                wxRealPoint imgMousePoint;

                if (this->panel->pointToOrigImageCoords(mousePoint, imgMousePoint))
                {
                    this->viewPoint.x = imgMousePoint.x - (int)(mousePoint.x / this->zoomFactor);
                    this->viewPoint.y = imgMousePoint.y - (int)(mousePoint.y / this->zoomFactor);
                    this->updateView();
                }
            }
        }
    }

    void ImageScrollPanel::onMouseWheelMoved(wxMouseEvent& event)
    {
        int r = event.GetWheelRotation();

        if (r)
        {
            if (event.ControlDown())
            {
                wxPoint mousePoint(event.GetX(), event.GetY());
                this->zoomView(mousePoint, r > 0);
            }
            else
            {
                this->panView(!event.ShiftDown(), r > 0);
            }
        }
    }

    void ImageScrollPanel::updateScrollbars()
    {
        wxRect panelViewRoi = panel->getViewRoi();
        wxSize fullImageSize = panel->getFullImageSize();

        this->hScrollBar->SetRange(fullImageSize.GetWidth());
        this->hScrollBar->SetThumbPosition(this->viewPoint.x);
        this->hScrollBar->SetThumbSize(panelViewRoi.GetWidth());

        this->vScrollBar->SetRange(fullImageSize.GetHeight());
        this->vScrollBar->SetThumbPosition(this->viewPoint.y);
        this->vScrollBar->SetThumbSize(panelViewRoi.GetHeight());
    }

    void ImageScrollPanel::onScrollbarChanged(wxScrollEvent& event)
    {
        int pos = event.GetPosition();

        if (event.GetOrientation() == wxHORIZONTAL)
        {
            this->viewPoint.x = pos;
        }
        else
        {
            this->viewPoint.y = pos;
        }

        this->updateView();
    }

    void ImageScrollPanel::onScrollbarThumbTrack(wxScrollEvent& event)
    {
        int pos = event.GetPosition();

        if (event.GetOrientation() == wxHORIZONTAL)
        {
            this->viewPoint.x = pos;
        }
        else
        {
            this->viewPoint.y = pos;
        }

        this->updateView();
    }

    void ImageScrollPanel::onKeyDown(wxKeyEvent& event)
    {
        if (this->checkHasImage())
        {
            int code = event.GetKeyCode();
            int modifiers = event.GetModifiers();

            if ((((code == wxKeyCode::WXK_HOME) || (code == wxKeyCode::WXK_NUMPAD_HOME)) && (modifiers == wxMOD_CONTROL)) ||
                ((code == 'F') && (modifiers == (wxMOD_CONTROL | wxMOD_SHIFT))))
            {
                this->setViewToFitImage();
            }
            else if ((code == wxKeyCode::WXK_HOME) || (code == wxKeyCode::WXK_NUMPAD_HOME))
            {
                this->setViewToFitImage();
            }
            else if (code == wxKeyCode::WXK_RIGHT)
            {
                this->panView(false, false);
            }
            else if (code == wxKeyCode::WXK_LEFT)
            {
                this->panView(false, true);
            }
            else if ((modifiers == wxMOD_CONTROL) && (code == wxKeyCode::WXK_DOWN))
            {
                this->zoomView(this->viewPoint, false);
            }
            else if ((modifiers == wxMOD_CONTROL) && (code == wxKeyCode::WXK_UP))
            {
                this->zoomView(this->viewPoint, true);
            }
            else if (code == wxKeyCode::WXK_DOWN)
            {
                this->panView(true, false);
            }
            else if (code == wxKeyCode::WXK_UP)
            {
                this->panView(true, true);
            }
            else if (code == wxKeyCode::WXK_PAGEDOWN)
            {
                this->panView(true, false, true);
            }
            else if (code == wxKeyCode::WXK_PAGEUP)
            {
                this->panView(true, true, true);
            }
            else if (code == wxKeyCode::WXK_ESCAPE)
            {
                this->isDrawing = false;
                this->panel->setDrawnRoi(cv::Rect2f());

                if (this->onDrawnRoiChangeCallback)
                {
                    this->onDrawnRoiChangeCallback();
                }
            }
        }

        // by default the main menu (e.g. Alt-F, X) will not receive any keyboard events while this control has focus unless we do this
        event.ResumePropagation(1);
        event.Skip();
    }

    void ImageScrollPanel::setRenderShapes(bool doRender)
    {
        if (this->panel)
        {
            ImageViewPanelSettings settings = this->panel->getSettings();
            settings.doRenderShapes = doRender;
            this->panel->setSettings(settings);
        }
    }

    bool ImageScrollPanel::getRenderShapes()
    {
        if (this->panel)
        {
            ImageViewPanelSettings settings = this->panel->getSettings();
            return settings.doRenderShapes;
        }
        else
        {
            return true;
        }
    }
}
