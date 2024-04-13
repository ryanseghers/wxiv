// Copyright(c) 2023 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <string>
#include <memory>
#include <opencv2/opencv.hpp>

#include "WxivImage.h"
#include "ImageListSource.h"
#include "ImageListSourceDirectory.h"
#include "WxWidgetsUtil.h"
#include "Contour.h"

namespace Wxiv
{
    /**
     * @brief Concrete ImageListSource sub-class that loads from a local dir of dcm files.
     */
    class ImageListSourceDcmDirectory : public ImageListSourceDirectory
    {
      private:
        /**
         * @brief Each contour has all images/slices (one vector of points per slice/image).
         */
        std::vector<Contour> contours;

      protected:
        virtual bool checkSupportedFile(const wxString& name) override;

      public:
        bool loadImage(std::shared_ptr<WxivImage> image) override;
        void load(wxString dirPath) override;
    };
}
