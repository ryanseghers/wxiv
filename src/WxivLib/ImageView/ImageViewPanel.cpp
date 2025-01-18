// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <fmt/core.h>
#include <opencv2/opencv.hpp>

#include "WxWidgetsUtil.h"
#include <wx/splitter.h>

#include "ImageUtil.h"
#include "ImageViewPanel.h"
#include "MiscUtil.h"
#include "WxWidgetsUtil.h"
#include "WxivUtil.h"

#define ID_POPUP_PLACEHOLDER 2000

using namespace std;

namespace Wxiv
{
    ImageViewPanel::ImageViewPanel(wxWindow* parent) : wxWindow(parent, -1, wxDefaultPosition, wxDefaultSize, wxALWAYS_SHOW_SB | wxWANTS_CHARS)
    {
        build();
    }

    void ImageViewPanel::build()
    {
        Bind(wxEVT_PAINT, &ImageViewPanel::paintEvent, this, wxID_ANY);
        Bind(wxEVT_ERASE_BACKGROUND, &ImageViewPanel::onEraseBackground, this, wxID_ANY);

        Bind(wxEVT_CONTEXT_MENU, [this](wxContextMenuEvent& evt) { onImageRightClick(evt); });
    }

    void ImageViewPanel::onContextMenuClick(wxCommandEvent& evt)
    {
        if (evt.GetId() == ID_POPUP_PLACEHOLDER)
        {
            showMessageDialog("Add image view context action here.");
        }
    }

    void ImageViewPanel::onImageRightClick(wxContextMenuEvent& evt)
    {
        if (!getImage().empty())
        {
            wxMenu mnu;
            mnu.Append(ID_POPUP_PLACEHOLDER, "Placeholder...");
            mnu.Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ImageViewPanel::onContextMenuClick), NULL, this);
            PopupMenu(&mnu);
        }
    }

    ShapeSet& ImageViewPanel::getShapes()
    {
        return this->shapes;
    }

    wxSize ImageViewPanel::getDrawSize()
    {
        return this->GetClientSize();
    }

    wxSize ImageViewPanel::getFullImageSize()
    {
        if (!this->orig.empty())
        {
            return wxSize(orig.cols, orig.rows);
        }
        else
        {
            return wxSize();
        }
    }

    bool ImageViewPanel::checkHasImage()
    {
        return !this->orig.empty();
    }

    void ImageViewPanel::setImage(cv::Mat& newImage)
    {
        this->orig = newImage;
        this->invalidateCaches();
        Refresh();

        // trigger lazy update of percentile depending on mode
        wholeImageLowValue = 0.0f;
        wholeImageHighValue = 0.0f;
    }

    /**
     * @brief Get a cv::Mat of the original image, not a clone.
     */
    cv::Mat ImageViewPanel::getImage()
    {
        return this->orig;
    }

    void ImageViewPanel::setShapes(ShapeSet& set)
    {
        // avoid render in the common case of having no shapes and setting no shapes
        bool skipRender = this->shapes.empty() && set.empty();

        this->shapes = set;

        if (!skipRender)
        {
            this->paintNow();
        }
    }

    cv::Rect2f ImageViewPanel::getDrawnRoi()
    {
        return this->drawnRoi;
    }

    void ImageViewPanel::setDrawnRoi(cv::Rect2f roi)
    {
        this->drawnRoi = roi;
        this->Refresh();
    }

    ImageViewPanelSettings ImageViewPanel::getSettings()
    {
        return this->settings;
    }

    void ImageViewPanel::setSettings(ImageViewPanelSettings newSettings)
    {
        this->settings = newSettings;
        this->Refresh();
    }

    void ImageViewPanel::setOnRenderCallback(const std::function<void(void)>& f)
    {
        this->onRenderCallback = f;
    }

    /**
     * @brief Specifies where in the source image to display on the panel.
     * @param origPt Upper left point in original image to display at upper left corner of the panel.
     * @param inZoom Zoom as ratio of view pixels to source pixels. So 10 means 1 source pixel becomes 10 view pixels.
     */
    void ImageViewPanel::setView(wxPoint origPt, float inZoom)
    {
        this->viewPoint = origPt;
        this->zoom = inZoom;

        // viewRoi
        wxSize clientSize = this->GetClientSize();
        float origImageWidth = clientSize.x / inZoom;
        float origImageHeight = clientSize.y / inZoom;
        this->viewRoi = cv::Rect2f(origPt.x, origPt.y, origImageWidth, origImageHeight);

        Refresh();
    }

    void ImageViewPanel::setViewToFitImage()
    {
        this->setView(wxPoint(0, 0), this->getZoomToFitImage());
    }

    std::tuple<float, float> ImageViewPanel::getLastIntensityRange()
    {
        return std::tuple<float, float>(this->lastLowValue, this->lastHighValue);
    }

    float ImageViewPanel::getZoomToFitImage()
    {
        wxSize clientSize = this->GetClientSize();
        float zx = (float)clientSize.x / (float)this->orig.cols;
        float zy = (float)clientSize.y / (float)this->orig.rows;
        return std::min(zx, zy);
    }

    wxRect ImageViewPanel::getViewRoi()
    {
        return wxRect(lroundf(this->viewRoi.x), lroundf(this->viewRoi.y), lroundf(this->viewRoi.width), lroundf(this->viewRoi.height));
    }

    void ImageViewPanel::onEraseBackground(wxEraseEvent& event)
    {
        // eat this event to prevent flicker
    }

    /*
     * Called by the system of by wxWidgets when the panel needs
     * to be redrawn. You can also trigger this call by
     * calling Refresh()/Update().
     */
    void ImageViewPanel::paintEvent(wxPaintEvent& evt)
    {
        wxPaintDC dc(this);
        render(dc);
    }

    /*
     * Alternatively, you can use a clientDC to paint on the panel
     * at any time. Using this generally does not free you from
     * catching paint events, since it is possible that e.g. the window
     * manager throws away your drawing when the window comes to the
     * background, and expects you will redraw it when the window comes
     * back (by sending a paint event).
     */
    void ImageViewPanel::paintNow()
    {
        wxClientDC dc(this);
        render(dc);
    }

    bool ImageViewPanel::pointToOrigImageCoords(wxPoint mousePoint, wxRealPoint& imagePoint)
    {
        if (!this->orig.empty())
        {
            imagePoint.x = mousePoint.x / this->zoom + this->viewPoint.x - 0.5f;
            imagePoint.y = mousePoint.y / this->zoom + this->viewPoint.y - 0.5f;
            return true;
        }
        else
        {
            return false;
        }
    }

    wxPoint ImageViewPanel::imageCoordsToScreen(wxRealPoint imagePoint)
    {
        int x = this->zoom * (imagePoint.x - this->viewPoint.x + 0.5f);
        int y = this->zoom * (imagePoint.y - this->viewPoint.y + 0.5f);
        return wxPoint(x, y);
    }

    int ImageViewPanel::imageLengthToScreen(float len)
    {
        return this->zoom * len;
    }

    wxPoint ImageViewPanel::imageCoordsToScreen(float x, float y)
    {
        int xi = (int)(this->zoom * (x - this->viewPoint.x + 0.5f));
        int yi = (int)(this->zoom * (y - this->viewPoint.y + 0.5f));
        return wxPoint(xi, yi);
    }

    inline void ImageViewPanel::imageCoordsToScreenCv(float x, float y, cv::Point2i& screenPoint)
    {
        screenPoint.x = (int)(this->zoom * (x - this->viewPoint.x + 0.5f));
        screenPoint.y = (int)(this->zoom * (y - this->viewPoint.y + 0.5f));
    }

    std::string ImageViewPanel::getPixelValueString(wxPoint imagePoint)
    {
        return ImageUtil::getPixelValueString(this->orig, cv::Point2i(imagePoint.x, imagePoint.y));
    }

    cv::Rect2i wxToCvRect(wxRect r)
    {
        return cv::Rect2i(r.x, r.y, r.width, r.height);
    }

    /**
     * @brief Just trying this out. This is much slower but could be used to avoid OpenCV dependency.
     * @param dc
     */
    void ImageViewPanel::wxDrawShapes(wxDC& dc)
    {
        // shapes to dc
        wxColour color("green");
        dc.SetPen(wxPen(color));
        dc.SetBrush(wxBrush(color));
        int pointRadius = 9; // in view coords

        for (auto& pt : this->shapes.points)
        {
            if (viewRoi.contains(pt))
            {
                wxPoint screenPoint = imageCoordsToScreen(wxPoint(pt.x, pt.y));

                // plus
                dc.DrawLine(screenPoint.x - pointRadius, screenPoint.y, screenPoint.x + pointRadius, screenPoint.y);
                dc.DrawLine(screenPoint.x, screenPoint.y - pointRadius, screenPoint.x, screenPoint.y + pointRadius);
                dc.DrawCircle(screenPoint, wxCoord(pointRadius));
            }
        }
    }

    void ImageViewPanel::cvDrawRects(ShapeSet& inShapes, cv::Mat& imgRgb, cv::Scalar cvColor, cv::Rect2f viewRect)
    {
        int nShapes = inShapes.rects.size();
        int nColors = inShapes.rectColors.size();
        int nThickness = inShapes.rectThickness.size();
        cv::Point2i p1, p2;
        int thickness = 1;

        for (int i = 0; i < nShapes; i++)
        {
            cv::Rect2f& rect = inShapes.rects[i];

            if (!(rect | viewRect).empty())
            {
                if (nColors > 0)
                {
                    cvColor = inShapes.rectColors[std::min(i, nColors - 1)];
                }

                if (nThickness > 0)
                {
                    thickness = inShapes.rectThickness[std::min(i, nThickness - 1)];
                }

                imageCoordsToScreenCv(rect.x, rect.y, p1);
                imageCoordsToScreenCv(rect.x + rect.width, rect.y + rect.height, p2);
                cv::rectangle(imgRgb, p1, p2, cvColor, thickness);
            }
        }
    }

    void ImageViewPanel::cvDrawPoints(ShapeSet& inShapes, cv::Mat& imgRgb, cv::Scalar cvColor, cv::Rect2f viewRect)
    {
        cv::Vec3b cvColorVec;
        cvColorVec[0] = cvColor[0];
        cvColorVec[1] = cvColor[1];
        cvColorVec[2] = cvColor[2];

        // optimize, avoid roi.contains() calls
        int x0 = (int)viewRoi.x;
        int x1 = (int)viewRoi.x + (int)viewRoi.width;
        int y0 = (int)viewRoi.y;
        int y1 = (int)viewRoi.y + (int)viewRoi.height;

        cv::Point2i screenPoint;
        int plusRadius = 1; // in rendered image pixels
        int thickness = 2;  // for lines
        int nShapes = inShapes.points.size();
        int nColors = inShapes.pointColors.size();
        int nDims = inShapes.pointDim.size();
        int nThickness = inShapes.pointThickness.size();

        // I did optimize the common case where all points are same size and was not enough improvement to be
        // worth the extra lines of code
        for (int i = 0; i < nShapes; i++)
        {
            cv::Point2f& pt = inShapes.points[i];

            if ((pt.x >= x0) && (pt.x < x1) && (pt.y >= y0) && (pt.y < y1))
            {
                imageCoordsToScreenCv(pt.x, pt.y, screenPoint);

                if (nColors > 0)
                {
                    cvColor = inShapes.pointColors[std::min(i, nColors - 1)];

                    cvColorVec[0] = cvColor[0];
                    cvColorVec[1] = cvColor[1];
                    cvColorVec[2] = cvColor[2];
                }

                if (nDims > 0)
                {
                    int pointDim = inShapes.pointDim[std::min(i, nDims - 1)];

                    if (pointDim == 0)
                    {
                        plusRadius = 0;
                    }
                    else if (pointDim < 0)
                    {
                        // interpret it as screen pixels
                        plusRadius = -pointDim;
                    }
                    else
                    {
                        // interpret it as image (world) pixels
                        // lroundf is slow, and this is always positive
                        plusRadius = (int)(this->zoom * pointDim / 2 + 0.5f);
                    }
                }

                // plus
                // for perf, below certain number of screen pixels, not worth drawing lines (it pops in and out and looks bad but still think it's
                // worth it)
                if (plusRadius > 1)
                {
                    if (nThickness > 0)
                    {
                        thickness = inShapes.pointThickness[std::min(i, nThickness - 1)];
                        thickness = std::clamp(thickness, 1, plusRadius);
                    }

                    cv::line(imgRgb, cv::Point2i(screenPoint.x - plusRadius, screenPoint.y), cv::Point2i(screenPoint.x + plusRadius, screenPoint.y),
                        cvColor, thickness);
                    cv::line(imgRgb, cv::Point2i(screenPoint.x, screenPoint.y - plusRadius), cv::Point2i(screenPoint.x, screenPoint.y + plusRadius),
                        cvColor, thickness);
                }
                else
                {
                    // just do single pixel
                    // have to check on screen again
                    if ((screenPoint.x >= 0) && (screenPoint.x < imgRgb.cols) && (screenPoint.y >= 0) && (screenPoint.y < imgRgb.rows))
                    {
                        imgRgb.at<cv::Vec3b>(screenPoint.y, screenPoint.x) = cvColorVec;
                    }
                }
            }
        }
    }

    void ImageViewPanel::cvDrawCircles(ShapeSet& inShapes, cv::Mat& imgRgb, cv::Scalar cvColor, cv::Rect2f viewRect)
    {
        int nShapes = inShapes.circleCenters.size();
        int nColors = inShapes.circleColors.size();
        int nThickness = inShapes.circleThickness.size();
        int nRadius = inShapes.circleRadius.size();

        cv::Point2i screenPoint;
        int thickness = 1;

        // optimize, avoid roi.contains() calls
        int x0 = (int)viewRoi.x;
        int x1 = (int)viewRoi.x + (int)viewRoi.width;
        int y0 = (int)viewRoi.y;
        int y1 = (int)viewRoi.y + (int)viewRoi.height;

        for (int i = 0; i < nShapes; i++)
        {
            cv::Point2f& pt = inShapes.circleCenters[i];

            if ((pt.x >= x0) && (pt.x < x1) && (pt.y >= y0) && (pt.y < y1))
            {
                imageCoordsToScreenCv(pt.x, pt.y, screenPoint);

                if (nColors > 0)
                {
                    cvColor = inShapes.circleColors[std::min(i, nColors - 1)];
                }

                if (nThickness > 0)
                {
                    thickness = inShapes.circleThickness[std::min(i, nThickness - 1)];
                }

                int radius = imageLengthToScreen(inShapes.circleRadius[std::min(i, nRadius - 1)]);
                cv::circle(imgRgb, screenPoint, radius, cvColor, thickness);
            }
        }
    }

    void ImageViewPanel::cvDrawLines(ShapeSet& inShapes, cv::Mat& imgRgb, cv::Scalar cvColor, cv::Rect2f viewRect)
    {
        int nShapes = inShapes.lines.size();
        int nColors = inShapes.lineColors.size();
        int nThickness = inShapes.lineThickness.size();
        int thickness = 1;
        cv::Point2i p1, p2;

        for (int i = 0; i < nShapes; i++)
        {
            auto& pair = inShapes.lines[i];

            if (nColors > 0)
            {
                cvColor = inShapes.lineColors[std::min(i, nColors - 1)];
            }

            if (nThickness > 0)
            {
                thickness = inShapes.lineThickness[std::min(i, nThickness - 1)];
            }

            imageCoordsToScreenCv(pair.first.x, pair.first.y, p1);
            imageCoordsToScreenCv(pair.second.x, pair.second.y, p2);
            cv::line(imgRgb, p1, p2, cvColor, thickness);
        }
    }

    void ImageViewPanel::cvDrawPolygons(ShapeSet& inShapes, cv::Mat& imgRgb, cv::Rect2f viewRect)
    {
        int nShapes = inShapes.polygons.size();

        for (int i = 0; i < nShapes; i++)
        {
            auto& poly = inShapes.polygons[i];

            // PERF: maybe keep vector<vector<cv::Point2i>> around to avoid mem alloc
            vector<cv::Point2i> xformPoly;

            for (int j = 0; j < poly.points.size(); j++)
            {
                cv::Point2i p1;
                imageCoordsToScreenCv(poly.points[j].x, poly.points[j].y, p1);
                xformPoly.push_back(p1);
            }

            cv::polylines(imgRgb, xformPoly, true, poly.colorRgb, poly.lineThickness);
        }
    }

    /**
     * @brief OpenCV draw shapes onto the specified rgb image.
     * @param imgRgb
     */
    void ImageViewPanel::cvDrawShapes(ShapeSet& inShapes, cv::Mat& imgRgb)
    {
        cv::Scalar cvColor({0, 255, 0});

        // drawnRoi
        if (!this->drawnRoi.empty())
        {
            cv::Point2i p1, p2;
            imageCoordsToScreenCv(this->drawnRoi.x, this->drawnRoi.y, p1);
            imageCoordsToScreenCv(this->drawnRoi.x + this->drawnRoi.width, this->drawnRoi.y + this->drawnRoi.height, p2);
            cv::rectangle(imgRgb, p1, p2, cvColor, 1);
        }

        cvDrawRects(inShapes, imgRgb, cvColor, viewRoi);
        cvDrawPoints(inShapes, imgRgb, cvColor, viewRoi);
        cvDrawCircles(inShapes, imgRgb, cvColor, viewRoi);
        cvDrawLines(inShapes, imgRgb, cvColor, viewRoi);
        cvDrawPolygons(inShapes, imgRgb, viewRoi);
    }

    void ImageViewPanel::setBackground(uint8_t v)
    {
        this->background = v;
    }

    /**
     * @brief Maybe update origSubImage, which is just a roi from the orig image.
     */
    void ImageViewPanel::updateOrigSubImage()
    {
        static cv::Rect2i lastCvSrcIntersectRoi;
        cv::Rect2i origImageRoi(0, 0, orig.cols, orig.rows);

        // sub-images are integer sized despite float view roi, so preserve exact aspect ratio of portion of orig image to display
        // (and note that viewRoi often goes off orig image)
        cv::Rect2f origImageRoi2f(0, 0, orig.cols, orig.rows);
        cv::Rect2f cvSrcIntersectRoi2f = origImageRoi2f & viewRoi;
        origSubImageAr = cvSrcIntersectRoi2f.width / cvSrcIntersectRoi2f.height;

        // get sub-image of original that will be used (may not be same shape as dc)
        cv::Rect2i cvViewCeilRoi((int)viewRoi.x, (int)viewRoi.y, (int)ceilf(viewRoi.width), (int)ceilf(viewRoi.height));
        cv::Rect2i cvSrcIntersectRoi = origImageRoi & cvViewCeilRoi;

        if (!isOrigSubImageValid || (cvSrcIntersectRoi != lastCvSrcIntersectRoi))
        {
            origSubImage = orig(cvSrcIntersectRoi);
            isOrigSubImageValid = true;
            isOrigSubImageRangedValid = false; // origSubImage has changed
            lastCvSrcIntersectRoi = cvSrcIntersectRoi;
        }
    }

    /**
     * @brief Maybe update origSubImageRanged, which is origSubImage converted to 8u via intensity ranging.
     */
    void ImageViewPanel::updateOrigSubImageRanged()
    {
        // keep settings to look for any changes, this is overly broad but simple and settings should
        // not be changing often
        static ImageViewPanelSettings lastSettings;

        // try avoid work
        if (!this->isOrigSubImageRangedValid || (lastSettings != this->settings))
        {
            // intensity ranging
            float lowVal, highVal;

            if (this->settings.intensityRangeParams.mode == IntensityRangeMode::NoOp)
            {
                // raw cast
                origSubImage.convertTo(origSubImageRanged, CV_8U);
            }
            else
            {
                if (this->settings.intensityRangeParams.mode == IntensityRangeMode::ViewPercentile)
                {
                    float lowPct = this->settings.intensityRangeParams.viewRoiLowPercentile;
                    float highPct = this->settings.intensityRangeParams.viewRoiHighPercentile;
                    std::pair<float, float> t = ImageUtil::histPercentiles(origSubImage, lowPct, highPct);
                    lowVal = t.first;
                    highVal = t.second;
                }
                else if (this->settings.intensityRangeParams.mode == IntensityRangeMode::WholeImagePercentile)
                {
                    // compute if not already computed
                    if (wholeImageHighValue <= wholeImageLowValue)
                    {
                        float lowPct = this->settings.intensityRangeParams.wholeImageLowPercentile;
                        float highPct = this->settings.intensityRangeParams.wholeImageHighPercentile;
                        std::pair<float, float> t = ImageUtil::histPercentiles(orig, lowPct, highPct);
                        wholeImageLowValue = t.first;
                        wholeImageHighValue = t.second;
                    }

                    lowVal = wholeImageLowValue;
                    highVal = wholeImageHighValue;
                }
                else if (this->settings.intensityRangeParams.mode == IntensityRangeMode::Explicit)
                {
                    lowVal = this->settings.intensityRangeParams.explicitLowValue;
                    highVal = this->settings.intensityRangeParams.explicitHighValue;
                }
                else
                {
                    throw std::runtime_error("This intensity range mode not implemented yet.");
                }

                ImageUtil::imgTo8u(origSubImage, origSubImageRanged, lowVal, highVal);
                this->isOrigSubImageRangedValid = true;
                this->isScaledSubImageValid = false;
                this->lastLowValue = lowVal;
                this->lastHighValue = highVal;

                // If origSubImage is 32F, then also build a nan mask where the values are NANs.
                if (origSubImage.type() == CV_32F)
                {
                    // Use the fact that NAN != NAN
                    // (This is setting some pixels but not all, for some reason)
                    //origSubImageNanMask = (origSubImage != origSubImage);

                    // Set to 255 where origSubImage is NAN
                    origSubImageNanMask.create(origSubImage.size(), CV_8U);

                    for (int y = 0; y < origSubImage.rows; y++)
                    {
                        for (int x = 0; x < origSubImage.cols; x++)
                        {
                            if (std::isnan(origSubImage.at<float>(y, x)))
                            {
                                origSubImageNanMask.at<uint8_t>(y, x) = 255;
                            }
                            else
                            {
                                origSubImageNanMask.at<uint8_t>(y, x) = 0;
                            }
                        }
                    }
                }
            }

            lastSettings = this->settings;
        }
    }

    /**
     * @brief Maybe update scaledSubImage which is origSubImage resized to render size.
     */
    cv::Rect2i ImageViewPanel::updateScaledSubImage(int drawWidth, int drawHeight)
    {
        // avoid work by keeping last values and check if this time is different
        static float lastZoom = NAN;
        static cv::Rect2i lastCopyRoi;

        // always recompute the copy roi
        cv::Rect2i copyRoi; // both src and dst roi for copy from origSubImageRanged to draw-surface image

        if (this->settings.doScaleToFit)
        {
            if (this->settings.doScaleMaintainAspectRatio)
            {
                // resize to fit and maintain aspect ratio
                float origAr = origSubImageAr;
                float drawAr = (float)drawWidth / drawHeight;
                int arWidth, arHeight; // ar-preserving dims to resize to

                if (origAr >= drawAr)
                {
                    // width-constrained
                    arWidth = drawWidth;
                    arHeight = std::min(drawHeight, (int)(arWidth / origAr + 0.5f));
                }
                else
                {
                    // height-constrained
                    arHeight = drawHeight;
                    arWidth = std::min(drawWidth, (int)(arHeight * origAr + 0.5f));
                }

                copyRoi = cv::Rect2i(0, 0, arWidth, arHeight);
            }
            else
            {
                // resize to fit regardless of aspect ratio
                copyRoi = cv::Rect2i(0, 0, drawWidth, drawHeight);
            }
        }
        else
        {
            // preserve aspect ratio
            // scale the sub-image (may not be same shape as dc) per zoom
            int newWidth = std::min(drawWidth, (int)lroundf(origSubImageRanged.cols * this->zoom));
            int newHeight = std::min(drawHeight, (int)lroundf(origSubImageRanged.rows * this->zoom));

            // put sub-image into dc-sized image (because scaled sub-image may not cover whole dc, e.g. due to aspect ratio)
            copyRoi = cv::Rect2i(0, 0, newWidth, newHeight);
        }

        if (!this->isScaledSubImageValid || (copyRoi != lastCopyRoi) || (this->zoom != lastZoom))
        {
            int interp = this->zoom < 1.0f ? cv::INTER_AREA : cv::INTER_NEAREST;
            cv::resize(origSubImageRanged, scaledSubImage, cv::Size(), zoom, zoom, interp);

            if (origSubImageNanMask.size().width > 0)
            {
                cv::resize(origSubImageNanMask, scaledSubImageNanMask, cv::Size(), zoom, zoom, cv::INTER_NEAREST);
            }

            // ensure copy roi is not off scaled sub-image
            copyRoi.width = std::min(copyRoi.width, scaledSubImage.cols);
            copyRoi.height = std::min(copyRoi.height, scaledSubImage.rows);

            this->isScaledSubImageValid = true;
            lastCopyRoi = copyRoi;
            lastZoom = this->zoom;
        }

        return copyRoi;
    }

    /**
     * @brief Render pixel value strings onto the image.
     * This is not optimized.
     * @param img
     */
    void ImageViewPanel::renderPixelStrings(cv::Mat& img)
    {
        const bool doPixelRects = false; // for debugging a zoom issue
        cv::Rect2i imgRoi(0, 0, img.cols, img.rows);

        // maybe these should be settings, but not sure it's worth the lines of code and the dialog real-estate
        int fontFace = cv::FONT_HERSHEY_DUPLEX;
        float fontScale = 0.45f;
        int baseline;
        auto charSize = cv::getTextSize(std::string("0"), fontFace, fontScale, 1, &baseline);

        // maybe reduce font scale because ARGB strings are pretty long
        if (origSubImage.channels() > 3)
        {
            string argbString = std::string("255, 255, 255, 255");

            while ((fontScale >= 0.1f) && (cv::getTextSize(argbString, fontFace, fontScale, 1, &baseline).width > zoom))
            {
                fontScale -= 0.05f;
            }
        }

        for (int y = 0; y < origSubImage.rows; y++)
        {
            for (int x = 0; x < origSubImage.cols; x++)
            {
                int x0 = (int)(x * zoom + 0.5f);
                int y0 = (int)(y * zoom + 0.5f);

                // locate the text from upper left of pixel plus 1-char margin
                int xr = x0 + charSize.width;
                int yr = y0 + charSize.height * 2; // yr is location of bottom left of text

                // orig sub image is over-sized so have to check if we are off image here
                if (imgRoi.contains(cv::Point2i(xr, yr)))
                {
                    string s = ImageUtil::getPixelValueString(origSubImage, cv::Point2f(x, y));

                    // pick a color for the text that has most contrast with image,
                    // use rendered color instead of orig color because of auto-ranging, and also note different orig image types but render always
                    // rgb
                    cv::Vec3b renderedColor = img.at<cv::Vec3b>(yr, xr);
                    uchar c = ((renderedColor[0] + renderedColor[1] + renderedColor[2]) > (128 * 3)) ? 0 : 255;
                    cv::Scalar color = cv::Scalar(c, c, c);

                    // this may be better, but is also a whole lot more expensive
                    // cv::Scalar color = ImageUtil::computeTextColor(img, cv::Point(xr, yr));

                    cv::putText(img, s, cv::Point(xr, yr), fontFace, fontScale, color);

                    if (doPixelRects)
                    {
                        cv::rectangle(img, cv::Rect(x0, y0, (int)zoom, (int)zoom), color);
                    }
                }
            }
        }
    }

    /**
     * @brief Render this->orig to specified wxImage and cv::Mat wrapper.
     * @param wxImage Output. This is created and rendered to if render happens.
     * @param wxImageWrapper Output. This is a wrapper around the wxImg argument and this is also set in here.
     * @return true if anything (more than background color) is rendered.
     */
    bool ImageViewPanel::renderToWxImage(ShapeSet& inShapes, wxImage& wxImg, cv::Mat& wxImgWrapper)
    {
        if (!orig.empty() && (viewRoi.width > 0) && (viewRoi.height > 0))
        {
            // maybe update origSubImage, which is just a roi out of orig
            this->updateOrigSubImage();

            // get render size
            wxClientDC dc(this);
            int drawWidth, drawHeight;
            dc.GetSize(&drawWidth, &drawHeight);

            // intensity range
            bool doRender = true;

            if ((origSubImage.type() == CV_16U) 
                || (origSubImage.type() == CV_16S) 
                || (origSubImage.type() == CV_8U) 
                || (origSubImage.type() == CV_32F) 
                ||(origSubImage.type() == CV_32S))
            {
                updateOrigSubImageRanged();
            }
            else if ((origSubImage.type() == CV_8UC3) || (origSubImage.type() == CV_8UC4))
            {
                origSubImageRanged = origSubImage;
                this->isOrigSubImageRangedValid = true;
                this->isScaledSubImageValid = false;
            }
            else
            {
                doRender = false;
            }

            // ensure wxImgWrapper cv::Mat that wraps the wx image from which we will draw to the DC
            wxImg.Create(drawWidth, drawHeight, false);
            wxImgWrapper = cv::Mat(drawHeight, drawWidth, CV_8UC3, wxImg.GetData());

            // fill bg for error cases and also when render doesn't cover whole thing
            // PERF: could avoid this sometimes
            wxImgWrapper = cv::Scalar(this->background, this->background, this->background);

            if (doRender)
            {
                // maybe rebuild scaledSubImage, and get the copy roi which is both src and dst roi for copy from scaledSubImage to draw-surface image
                cv::Rect2i copyRoi = this->updateScaledSubImage(drawWidth, drawHeight);

                // get pixels to wxImgWrapper, which is RGB (and is the dc)
                if (scaledSubImage.type() == CV_8U)
                {
                    // dcImgTmp is size of the dc, so we copy to it and then convert to rgb from there
                    dcImgTmp.create(drawHeight, drawWidth, CV_8U);
                    dcImgTmp = this->background; // fill bg
                    scaledSubImage(copyRoi).copyTo(dcImgTmp(copyRoi));

                    // to rgb in a wxImage
                    cv::cvtColor(dcImgTmp, wxImgWrapper, cv::COLOR_GRAY2BGR);
                }
                else if (scaledSubImage.type() == CV_8UC3)
                {
                    cv::cvtColor(scaledSubImage(copyRoi), wxImgWrapper(copyRoi), cv::COLOR_BGR2RGB);
                }
                else if (scaledSubImage.type() == CV_8UC4)
                {
                    // yes this needs to be "*2RGB", apparently
                    cv::cvtColor(scaledSubImage(copyRoi), wxImgWrapper(copyRoi), cv::COLOR_BGRA2RGB);
                }
                else if (scaledSubImage.type() == CV_32F)
                {
                    // dcImgTmp is size of the dc, so we copy to it and then convert to rgb from there
                    dcImgTmp.create(drawHeight, drawWidth, CV_8U);
                    dcImgTmp = this->background; // fill bg
                    cv::convertScaleAbs(scaledSubImage(copyRoi), dcImgTmp(copyRoi));

                    // to rgb in a wxImage
                    cv::cvtColor(dcImgTmp, wxImgWrapper, cv::COLOR_GRAY2BGR);
                }
                else
                {
                    bail("unhandled image type");
                    return false;
                }

                // scaledSubImageNanMask is same size as scaledSubImage, and is a mask of where the values are NANs.
                // Render the set pixels of it to the wxImgWrapper as blue.
                if (scaledSubImageNanMask.size().width > 0)
                {
                    wxImgWrapper(copyRoi).setTo(cv::Scalar(0, 70, 70), scaledSubImageNanMask(copyRoi));
                }

                // pixel value strings (before shapes because we use rendered color (as opposed to orig color) for text color)
                if (this->settings.doRenderPixelValues && (zoom >= settings.maxZoom))
                {
                    renderPixelStrings(wxImgWrapper);
                }

                // shapes
                if (this->settings.doRenderShapes)
                {
                    cvDrawShapes(inShapes, wxImgWrapper);
                }

                return true;
            }
        }

        return false;
    }

    /**
     * @brief Set all the isValid bools to false so that next render will rebuild everything.
     */
    void ImageViewPanel::invalidateCaches()
    {
        this->isOrigSubImageRangedValid = false;
        this->isOrigSubImageValid = false;
        this->isScaledSubImageValid = false;
    }

    /**
     * @brief Render a cv::Mat to a wxImg using the current view roi and settings.
     * The return is pointing to the same data as this backer wxImage so only temporarily is the intended content.
     *
     * This DOES participate in the caching strategy of this class, meaning this is modifying object fields,
     * which is rather unfortunate but I'm choosing not to untangle that now.
     *
     * @return The resulting image.
     */
    wxImage ImageViewPanel::renderToWxImage(ShapeSet& inShapes, cv::Mat& img)
    {
        // temporarily overwrite orig image with the incoming image to be rendered
        cv::Mat oldOrig = this->orig;
        this->orig = img;

        // invalidate caches because we are rendering a new image
        this->invalidateCaches();

        renderToWxImage(inShapes, this->dcImage, this->dcImageWrapper);

        // restore
        this->orig = oldOrig;

        // invalidate again because the cached data is from this image, not orig
        this->invalidateCaches();

        return this->dcImage;
    }

    /**
     * @brief Call renderToWxImage and then return getRenderedImageClone() to get as cv::Mat.
     * @return The resulting image
     */
    cv::Mat ImageViewPanel::renderToImage(ShapeSet& inShapes, cv::Mat& img)
    {
        renderToWxImage(inShapes, img);
        return getRenderedImageClone();
    }

    /*
     * Render view of image to the dc.
     *
     * Some of the image copies could be replaced by a single custom op.
     * The intensity hist could potentially be lagging: compute during this render and used in next render.
     */
    void ImageViewPanel::render(wxDC& dc)
    {
        if (renderToWxImage(this->shapes, this->dcImage, this->dcImageWrapper))
        {
            // do not see how to avoid creating a new bitmap each time
            wxBitmap viewBitmap(this->dcImage);
            dc.DrawBitmap(viewBitmap, 0, 0, false);
        }
        else
        {
            dc.Clear();
        }

        if (this->onRenderCallback)
        {
            this->onRenderCallback();
        }
    }

    void ImageViewPanel::clearImage()
    {
        // set an empty image
        cv::Mat img;
        this->setImage(img);
    }

    /**
     * @brief Assume no need to re-render for this call.
     * The DC image is RGB but we want Mat to always be BGR so this does that conversion.
     * @return
     */
    cv::Mat ImageViewPanel::getRenderedImageClone()
    {
        cv::Mat bgr;
        cv::cvtColor(this->dcImageWrapper, bgr, cv::COLOR_RGB2BGR);
        return bgr;
    }

    /**
     * @brief Return a wxBitmap of current render. wxBitmap uses ref counting.
     * @return
     */
    wxBitmap ImageViewPanel::getViewBitmap()
    {
        wxBitmap viewBitmap(this->dcImage);
        return viewBitmap;
    }

    wxImage ImageViewPanel::getViewWxImageClone()
    {
        return this->dcImage.Copy();
    }
}
