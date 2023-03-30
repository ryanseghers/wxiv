// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <wx/wxprec.h>
#include <wx/splitter.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>
#include <wx/grid.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <opencv2/opencv.hpp>

#include "WxivImage.h"
#include "ImageScrollPanel.h"
#include "ImageViewPanel.h"
#include "ImageUtil.h"
#include "ShapeSet.h"
#include "PixelStatsPanel.h"
#include "ProfilesPanel.h"
#include "ShapeMetadataPanel.h"

namespace Wxiv
{
    /**
     * @brief High level window that has horizontal splitter with notebook on the left and ImageScrollPanel on the right.
     */
    class WxivMainSplitWindow : public wxWindow
    {
        ImageScrollPanel* imageScrollPanel = nullptr;
        std::shared_ptr<WxivImage> currentImage = nullptr;
        wxSplitterWindow* mainSplitter = nullptr;

        wxNotebook* notebook = nullptr;

        PixelStatsPanel* statsPanel = nullptr;
        ProfilesPanel* profilesPanel = nullptr;
        ShapeMetadataPanel* shapesPanel = nullptr;

        void onImageViewChange();
        void onDrawnRoiChange();
        void onCurrentImageShapesChange();
        void onShapeFilterChange(ShapeSet& newShapes);

      public:
        WxivMainSplitWindow(wxWindow* parent);

        void setImage(std::shared_ptr<WxivImage> newImage);
        void clearImage();
        void setViewToFitImage();
        int getSashPosition();
        cv::Mat getCurrentImage();
        cv::Mat getCurrentViewImageClone();
        wxBitmap getCurrentViewImageBitmap();
        wxImage getViewWxImageClone();

        /**
         * @brief Render the specified Mat just like current image is being rendered.
         * The returned wxImage points to an existing object member that will be modified soon so take a copy
         * if needed.
         */
        wxImage renderToWxImage(std::shared_ptr<WxivImage> image);
        cv::Mat renderToImage(std::shared_ptr<WxivImage> image);

        void saveConfig();
        void restoreConfig();
        void setRenderShapes(bool doRender);
        bool getRenderShapes();
    };
}
