// Copyright(c) 2023 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once

namespace Wxiv
{
    namespace ImageUtil
    {
        /**
        * @brief Parameters to specify how to render a list of images in rows and columns into a single larger image,
        * with a caption for each image.
        */
        struct CollageSpec
        {
            /**
            * @brief Number of columns of images.
            * The number of rows is computed from the total number of images.
            */
            int colCount = 4;

            /**
            * @brief Output image width (of the large single image into which all of the input images have been rendered).
            * The height is computed from the number of columns, scaling, and this width.
            */
            int imageWidthPx = 2048;

            /**
            * @brief The margin between images and also between the edge images and the edge of the output image.
            */
            int marginPx = 16;

            /**
            * @brief OpenCV font face.
            */
            int fontFace = cv::FONT_HERSHEY_DUPLEX;

            /**
            * @brief OpenCV font scale. This is multipled by the base font size to produce the final font size.
            * I arbitrarily chose this default from a single scenario.
            */
            double fontScale = 0.6f;

            /**
            * @brief Background can be white or black. Default is white.
            */
            bool doBlackBackground = false;

            /**
             * @brief Whether or not to put captions under each image.
            */
            bool doCaptions = true;
        };
    }
}
