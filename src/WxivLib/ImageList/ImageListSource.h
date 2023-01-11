// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>

#include "WxivImage.h"

namespace Wxiv
{
    /**
     * @brief Abstract class representing something that can load images.
     * The source provides the initial order for display in the list view.
     */
    class ImageListSource
    {
      public:
        ImageListSource();
        virtual ~ImageListSource() = 0;

        /**
         * @brief Load images from some source location as specified by a string, e.g. directory path, or
         * could be some other string like a URL.
         */
        virtual void load(wxString location) = 0;
        virtual int getImageCount() = 0;
        virtual std::shared_ptr<WxivImage> getImage(int idx) = 0;
        virtual void addImagePages(int idx, std::vector<std::shared_ptr<WxivImage>>& pages) = 0;
    };
}
