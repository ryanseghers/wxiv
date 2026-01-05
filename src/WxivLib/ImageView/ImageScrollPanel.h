// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once

#include <opencv2/opencv.hpp>

#include "WxWidgetsUtil.h"
#include <wx/splitter.h>
#include "WxivImage.h"
#include "ImageViewPanel.h"
#include "ShapeSet.h"

namespace Wxiv
{
    /**
     * @brief Contains an ImageViewPanel and scrollbars so controls zoom and pan, and has a toolbar with image dims etc.
     */
    class ImageScrollPanel : public wxWindow
    {
        ImageViewPanel* panel;

        wxPanel* toolbarPanel;

#ifdef __linux__
        // trouble with readonly wxTextCtrl on linux
        wxStaticText* imageTypeTextBox;
        wxStaticText* mousePosTextBox;
        wxStaticText* pixelValueTextBox;
        wxStaticText* drawnTextBox;
        wxStaticText* intensityRangeTextBox;
#else
        wxTextCtrl* imageTypeTextBox;
        wxTextCtrl* mousePosTextBox;
        wxTextCtrl* pixelValueTextBox;
        wxTextCtrl* drawnTextBox;
        wxTextCtrl* intensityRangeTextBox;
#endif

        // scrollbar units are pixels
        wxScrollBar* hScrollBar;
        wxScrollBar* vScrollBar;

        // We pan and zoom around in original image coords space and then render to our draw surface.
        // We can be viewing off the original image in any direction and zoom amount.
        wxPoint viewPoint;

        // zoom as view_px / original_px
        // 0 means not initialized
        float zoomFactor = 0.0f;

        // size of zoom changes
        float zoomMultiplier = 1.5f;

        // mouse wheel pan as fraction of view size in that axis
        int mouseWheelScrollFraction = 8;

        // derived from viewPoint and zoomFactor
        wxRect viewRoi;

        // drawn/drawing rect
        bool isDrawing = false;

        cv::Size2i currentImageSize;

        // view change means upper-left corner + zoom change
        std::function<void(void)> onViewChangeCallback;

        // when user is drawing a roi by mouse click and drag
        std::function<void(void)> onDrawnRoiChangeCallback;

        // we event when moused-over shape changes, so keep track of last so we know when changes
        ShapeType lastMouseOverShapeType;
        int lastMouseOverShapeIndex = -1;

        /**
         * @brief When mouse-over shape changes.
         * The index -1 means no shape is near the mouse.
         */
        std::function<void(ShapeType, int)> onMouseOverShapeChangeCallback;

        void build();
        void buildToolbar();

        void onScrollbarChanged(wxScrollEvent& event);
        void onScrollbarThumbTrack(wxScrollEvent& event);
        void onSize(wxSizeEvent& event);
        void onMouseWheelMoved(wxMouseEvent& event);
        void onMouseMoved(wxMouseEvent& event);
        void onMouseLeftUp(wxMouseEvent& event);
        void onKeyDown(wxKeyEvent& event);
        void onImageRender();

        void updateMouseOverShape(wxRealPoint imagePoint);

        void panView(bool doVert, bool doLeft, bool doFullPage);
        void zoomView(wxPoint mousePoint, bool doZoomIn);
        void updateView();
        void updateScrollbars();
        void updateDrawnRoiTextBox();

      public:
        ImageScrollPanel(wxWindow* parent);

        void setOnViewChangeCallback(const std::function<void(void)>& f);

        /**
         * @brief Currently just called on finish drawing roi and clearing roi.
         * @param f
         */
        void setOnDrawnRoiChangeCallback(const std::function<void(void)>& f);

        /**
         * @brief Called when mouse-over shape changes.
         */
        void setOnMouseOverShapeChangeCallback(const std::function<void(ShapeType, int)>& f);

        bool checkHasImage();
        void setImage(cv::Mat& newImage);
        void clearImage();
        cv::Mat getImage();
        void setShapes(ShapeSet& set);
        cv::Rect2f getDrawnRoi();
        void setViewToFitImage();

        /**
         * @brief The view ROi from image view panel. Note that this is often off-image.
         * @return
         */
        wxRect getViewRoi();

        /**
         * @brief Get the viewed portion of the image.
         * This does not have rendered shapes.
         * This may not be the view aspect ratio.
         * @return
         */
        cv::Mat getOrigViewImage();

        /**
         * @brief Viewed portion, with rendered shapes.
         * @return
         */
        cv::Mat getViewImageClone();

        /**
         * @brief Render the specified WxivImage just like current image is being rendered.
         * The returned wxImage points to an existing object member that will be modified soon so take a copy
         * if needed.
         */
        wxImage renderToWxImage(std::shared_ptr<WxivImage> image);
        cv::Mat renderToImage(std::shared_ptr<WxivImage> image);

        /**
         * @brief Pointer to orig so short lifetime, and gets overwritten every render.
         * @return
         */
        wxBitmap getViewBitmap();

        wxImage getViewWxImageClone();

        ImageViewPanelSettings getSettings();
        void setSettings(ImageViewPanelSettings newSettings);

        void setRenderShapes(bool doRender);
        bool getRenderShapes();

        void setRenderPixelValues(bool doRender);
        bool getRenderPixelValues();

        void showBrightnessSettingsDialog();
    };
}
