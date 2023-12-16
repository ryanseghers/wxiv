// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <vector>
#include <memory>

#include <opencv2/opencv.hpp>
#include "ArrowFilterExpression.h"
#include "Polygon.h"
#include "WxWidgetsUtil.h"
#include <wx/splitter.h>
#include <wx/filename.h>

namespace Wxiv
{
    /**
     * @brief Polygons are very different in that they have a variable number of points,
     * and are thus handled by different code.
     */
    enum class ShapeType
    {
        Unset = 0,
        Point = 1,
        Circle = 2,
        LineSegment = 3,
        Rect = 4,
        Quad = 5,
    };

    /**
     * @brief Container for shapes to render on top of the image based on Arrow table and Arrow filtering expressions.
     * This is designed for many shapes (with fixed numbers of points) and with many properties, basically a csv/table of shapes.
     *
     * Despite the name, this isn't a general purpose implementation, this is just for loading from csv/parquet and supporting
     * filtering and rendering.
     *
     * For each shape there are associated vectors of properties. If the associated vector is empty then a default is used.
     * If the associated vector is not empty then the entry corresponding to the shape is used, or the last one in the vector
     * if there are too few. Thus if there is a single entry in the vector it is used for all shapes.
     *
     * For each shape there is also an index into the Arrow table for other metadata in the original row.
     *
     * Filtering is enacted by rebuilding the vectors of shapes and properties.
     * This design anticipates rare filter changes.
     *
     * These will always render to an RGB surface, so colors are like cv::Scalar c({0, 254, 0})
     */
    struct ShapeSet
    {
        // Arrow table for arbitrary shape metadata
        std::shared_ptr<arrow::Table> ptable;

        // points (rendered as crosses)
        std::vector<cv::Point2f> points;
        std::vector<int> pointTableIndices; // indices into Arrow table
        std::vector<int> pointDim;
        std::vector<cv::Scalar> pointColors; // rgb
        std::vector<int> pointThickness;     // because (or if) they are rendered as pluses

        // rects
        std::vector<cv::Rect2f> rects;
        std::vector<int> rectTableIndices; // indices into Arrow table
        std::vector<int> rectThickness;
        std::vector<cv::Scalar> rectColors; // rgb

        // circles
        std::vector<cv::Point2f> circleCenters;
        std::vector<int> circleTableIndices; // indices into Arrow table
        std::vector<int> circleRadius;
        std::vector<int> circleThickness;
        std::vector<cv::Scalar> circleColors; // rgb

        // line segments
        std::vector<std::pair<cv::Point2f, cv::Point2f>> lines;
        std::vector<int> lineTableIndices; // indices into Arrow table
        std::vector<int> lineThickness;
        std::vector<cv::Scalar> lineColors; // rgb

        FilterSpec filterSpec;
        std::vector<int> postFilterTableIndices; // table indices of values that survive filter

        // TODO: this is not based on the ptable, doesn't obey same filtering, probably shouldn't be here
        std::vector<Polygon> polygons;

        void clear();
        void clearShapeVectors();
        void rebuildShapeVectors();

        int getTotalCount();
        int getFilteredCount();
        bool empty();
        bool emptyPostFilter();
        bool tryLoadNeighborShapesFile(const wxFileName& imagePath);
        bool findShape(float x, float y, float radius, ShapeType& type, int& idx);
        int getTableIndex(ShapeType type, int idx);
        std::string getValueString(int tableIdx, std::string colName);
        void applyFilter(FilterSpec& filterSpec);
        int size();
        std::vector<float> getFloatValues(std::string colName);
    };
}
