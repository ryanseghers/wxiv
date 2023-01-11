// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <string>
#include <memory>
#include <opencv2/opencv.hpp>

#include "WxivImage.h"
#include "ImageListSource.h"
#include "WxWidgetsUtil.h"

namespace Wxiv
{
    /**
     * @brief Concrete ImageListSource sub-class that loads from a local dir.
     * The source provides the initial order for display in the list view.
     */
    class ImageListSourceDirectory : public ImageListSource
    {
        /**
         * @brief Images may be created in here but not actually loaded from disk yet.
         */
        std::vector<std::shared_ptr<WxivImage>> images;

      public:
        ImageListSourceDirectory();
        ~ImageListSourceDirectory() override;

        void load(wxString dirPath) override;
        int getImageCount() override;
        std::shared_ptr<WxivImage> getImage(int idx) override;
        void addImagePages(int idx, std::vector<std::shared_ptr<WxivImage>>& pages) override;
    };
}
