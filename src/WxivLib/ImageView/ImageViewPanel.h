// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <functional>
#include <opencv2/opencv.hpp>

#include "WxWidgetsUtil.h"
#include <wx/splitter.h>

#include "ShapeSet.h"
#include "ImageViewPanelSettings.h"

namespace Wxiv
{
    /**
     * @brief This stores and paints a ROI of an image and the shapes in that ROI.
     * This doesn't initiate resize, the owner (that presumably handles pan and zoom, if any) needs to call setViewRoi().
     * The primary purpose of this is to handle the image conversions, intensity, ranging etc to paint a portion of an image
     * to a panel.
     *
     * Usually this will be painting a fixed-aspect-ratio portion of an original image, however there is now a
     * just-started capability to just paint the whole image zoomed to fit the panel.
     * The API isn't all consistent with that paradigm and maybe that should be a separate class.
     *
     * There is some caching now for performance. In the chain of render images, often some of them do not need to be rebuilt.
     * The design is to have an "is valid" bool per image and set it to false when it needs to be rebuilt and to true when it is rebuilt.
     * The "is valid" bools only represent when the prior image in the chain is modified, not any other settings that might affect it.
     * This turns out to be fairly unfortunate with respect to renderToWxImage which wants to render a totally separate image,
     * but still worth it for perf, I think.
     */
    class ImageViewPanel : public wxWindow
    {
        ImageViewPanelSettings settings;

        // original image (empty for no image)
        cv::Mat orig;

        // keep pipeline of images as they will often not need to be reallocated
        bool isOrigSubImageValid = false; // for caching, when false it means this needs to be rebuilt
        cv::Mat origSubImage;             // viewRoi-sized (but not dc sized) sub-image of orig image
        float origSubImageAr = 0.0f;      // aspect ratio of origSubImage to preserve float precision (since origSubImage has integer dimensions).

        bool isOrigSubImageRangedValid = false; // for caching, when false it means this needs to be rebuilt
        cv::Mat origSubImageRanged;             // view-sized sub-image of orig image, intensity-ranged

        bool isScaledSubImageValid = false; // for caching, when false it means this needs to be rebuilt
        cv::Mat scaledSubImage;             // origSubImageRanged scaled to final size (but maybe only a portion of dc size)
        cv::Mat dcImgTmp;                   // size of the dc, type of the rendered image

        wxImage dcImage;        // RGB image, size of the dc
        cv::Mat dcImageWrapper; // points to dcImage's data

        // set by owner
        wxPoint viewPoint; // in orig image coords, upper-left corner of roi to view
        float zoom = 0.0f; // ratio of view pixels over orig pixels: view / orig
        cv::Rect2f viewRoi; // derived from viewPoint and zoom, float for sub-pixel when zoomed way in, but x,y always integral

        // when image doesn't cover draw area
        uint8_t background = 50;

        // when we do whole-image percentiles put values in here
        float wholeImageLowValue = 0.0f;
        float wholeImageHighValue = 0.0f;

        // the last rendered intensity range used
        float lastLowValue = 0.0f;
        float lastHighValue = 0.0f;

        // shapes to render
        ShapeSet shapes;

        // drawn/drawing roi
        cv::Rect2f drawnRoi;

        std::function<void(void)> onRenderCallback;

        void build();
        void paintEvent(wxPaintEvent& evt);
        void paintNow();

        // render image pipeline
        void updateOrigSubImage();
        void updateOrigSubImageRanged();
        cv::Rect2i updateScaledSubImage(int drawWidth, int drawHeight);
        void invalidateCaches();

        void render(wxDC& dc);
        void onEraseBackground(wxEraseEvent& event);

        void wxDrawShapes(wxDC& dc);
        void cvDrawShapes(ShapeSet& shapes, cv::Mat& imgRgb);
        void cvDrawRects(ShapeSet& shapes, cv::Mat& imgRgb, cv::Scalar cvColor, cv::Rect2f viewRect);
        void cvDrawPoints(ShapeSet& shapes, cv::Mat& imgRgb, cv::Scalar cvColor, cv::Rect2f viewRect);
        void cvDrawCircles(ShapeSet& shapes, cv::Mat& imgRgb, cv::Scalar cvColor, cv::Rect2f viewRect);
        void cvDrawLines(ShapeSet& shapes, cv::Mat& imgRgb, cv::Scalar cvColor, cv::Rect2f viewRect);

        void onImageRightClick(wxContextMenuEvent& evt);
        void onContextMenuClick(wxCommandEvent& evt);

        bool renderToWxImage(ShapeSet& inShapes, wxImage& wxImage, cv::Mat& wxImageWrapper);

      public:
        ImageViewPanel(wxWindow* parent);

        bool checkHasImage();
        void setImage(cv::Mat& newImage);
        void clearImage();
        cv::Mat getImage();
        wxBitmap getViewBitmap();
        wxImage getViewWxImageClone();

        /**
         * @brief The image as rendered, with auto-ranging and shapes.
         * @return
         */
        cv::Mat getRenderedImageClone();

        /**
         * @brief Render the specified Mat just like current image is being rendered.
         * The returned wxImage points to an existing object member that will be modified soon so take a copy
         * if needed.
         */
        wxImage renderToWxImage(ShapeSet& inShapes, cv::Mat& img);
        cv::Mat renderToImage(ShapeSet& inShapes, cv::Mat& img);

        void setView(wxPoint origPt, float zoom);
        void setViewToFitImage();
        void setShapes(ShapeSet& set);
        cv::Rect2f getDrawnRoi();
        void setDrawnRoi(cv::Rect2f roi);
        std::tuple<float, float> getLastIntensityRange();

        ShapeSet& getShapes();

        void setOnRenderCallback(const std::function<void(void)>& f);

        /**
         * @brief Background is rgb but not exposing that for now.
         * @param v
         */
        void setBackground(uint8_t v);

        // settings
        ImageViewPanelSettings getSettings();
        void setSettings(ImageViewPanelSettings newSettings);

        /**
         * @brief Calculate the zoom to view whole image with origin at 0,0.
         */
        float getZoomToFitImage();

        /**
         * @brief The view roi, in original image coords. Note that this is often off-image.
         * @return
         */
        wxRect getViewRoi();

        /**
         * @brief Convert a mouse (dc) coords point to image coords point.
         * @param mousePoint
         * @return
         */
        bool pointToOrigImageCoords(wxPoint mousePoint, wxRealPoint& imagePoint);
        wxPoint imageCoordsToScreen(wxRealPoint mousePoint);
        wxPoint imageCoordsToScreen(float x, float y);
        void imageCoordsToScreenCv(float x, float y, cv::Point2i& screenPoint);
        int imageLengthToScreen(float len);

        std::string getPixelValueString(wxPoint imagePoint);

        /**
         * @brief The draw surface size, in pixels.
         * @return
         */
        wxSize getDrawSize();
        wxSize getFullImageSize();
    };
}
