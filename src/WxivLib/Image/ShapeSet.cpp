// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <string>
#include <filesystem>
#include <memory>
#include <limits>

#include "ShapeSet.h"
#include "MiscUtil.h"
#include "ArrowUtil.h"
#include "StringUtil.h"
#include "WxivUtil.h"

using namespace std;
namespace fs = std::filesystem;

namespace Wxiv
{
    /**
     * @brief Whether or not this is empty, pre-filter.
     */
    bool ShapeSet::empty()
    {
        if (this->ptable)
        {
            return (int)this->ptable->num_rows() == 0;
        }
        else
        {
            return 0;
        }
    }

    /**
     * @brief Whether or not this has any shapes after filtering.
     */
    bool ShapeSet::emptyPostFilter()
    {
        return points.empty() && rects.empty() && circleCenters.empty() && lines.empty();
    }

    void ShapeSet::clear()
    {
        this->clearShapeVectors();
        this->filterSpec.expressions.clear();
        this->ptable = nullptr;
    }

    int ShapeSet::getTotalCount()
    {
        if (this->ptable)
        {
            return (int)this->ptable->num_rows();
        }
        else
        {
            return 0;
        }
    }

    /**
     * @brief Filtering rebuilds the vectors.
     * @return
     */
    int ShapeSet::getFilteredCount()
    {
        return (int)(this->points.size() + this->circleCenters.size() + this->rects.size() + this->lines.size());
    }

    void ShapeSet::clearShapeVectors()
    {
        this->points.clear();
        this->pointColors.clear();
        this->pointTableIndices.clear();
        this->pointDim.clear();
        this->pointThickness.clear();

        this->circleCenters.clear();
        this->circleColors.clear();
        this->circleTableIndices.clear();
        this->circleRadius.clear();
        this->circleThickness.clear();

        this->rects.clear();
        this->rectColors.clear();
        this->rectTableIndices.clear();
        this->rectThickness.clear();

        this->lines.clear();
        this->lineColors.clear();
        this->lineTableIndices.clear();
        this->lineThickness.clear();
    }

    /**
     * @brief In a file you can specify 0xRRGGBB and Arrow loads to int64.
     */
    cv::Scalar toColorScalar(int v)
    {
        cv::Scalar s;
        s[2] = (uint8_t)(v & 0xFF);
        s[1] = (uint8_t)((v >> 8) & 0xFF);
        s[0] = (uint8_t)((v >> 16) & 0xFF);
        return s;
    }

    /**
     * @brief Clear and rebuild from the Arrow table rows.
     * This throws if there is a neighbor file but it doesn't meet a criteria.
     */
    void ShapeSet::rebuildShapeVectors()
    {
        if (ptable != nullptr)
        {
            vector<string> colNames = ptable->ColumnNames();

            auto typeCol = ArrowUtil::getColumn(ptable, "type");
            auto xCol = ArrowUtil::getColumn(ptable, "x");
            auto yCol = ArrowUtil::getColumn(ptable, "y");
            auto dim1Col = ArrowUtil::getColumn(ptable, "dim1");
            auto dim2Col = ArrowUtil::getColumn(ptable, "dim2");
            auto thicknessCol = ArrowUtil::getColumn(ptable, "thickness");
            auto colorCol = ArrowUtil::getColumn(ptable, "color");

            // some are required
            if ((typeCol != nullptr) && (xCol != nullptr) && (yCol != nullptr) && (dim1Col != nullptr))
            {
                this->clearShapeVectors();

                // get value arrays
                vector<int> types, thicknesses, colorInts;
                vector<float> dim1Values, dim2Values;
                ArrowUtil::getIntValues(typeCol, types, 0);
                ArrowUtil::getFloatValues(dim1Col, dim1Values);

                if (thicknessCol != nullptr)
                {
                    ArrowUtil::getIntValues(thicknessCol, thicknesses, 0);

                    // ensure gte 1 thickness
                    for (size_t i = 0; i < thicknesses.size(); i++)
                    {
                        if (thicknesses[i] < 1)
                        {
                            thicknesses[i] = 1;
                        }
                    }
                }

                vector<float> xValues, yValues;
                ArrowUtil::getFloatValues(xCol, xValues);
                ArrowUtil::getFloatValues(yCol, yValues);

                if (dim2Col != nullptr)
                {
                    ArrowUtil::getFloatValues(dim2Col, dim2Values);
                }

                if (colorCol != nullptr)
                {
                    ArrowUtil::getIntValues(colorCol, colorInts, 0);
                }

                this->postFilterTableIndices = filterSpec.getMatchingRows(ptable);
                size_t n = postFilterTableIndices.size();

                for (int idx = 0; idx < (int)n; idx++)
                {
                    int i = postFilterTableIndices[idx];

                    if ((types[i] == 4) && (dim1Values[i] >= 0) && !dim2Values.empty() && (dim2Values[i] >= 0))
                    {
                        // rect
                        this->rects.push_back(cv::Rect2f(xValues[i], yValues[i], dim1Values[i], dim2Values[i]));
                        this->rectTableIndices.push_back(i);

                        if (!thicknesses.empty())
                        {
                            this->rectThickness.push_back(thicknesses[i]);
                        }

                        if (!colorInts.empty())
                        {
                            this->rectColors.push_back(toColorScalar(colorInts[i]));
                        }
                    }
                    else if (types[i] == 1)
                    {
                        // point
                        this->points.push_back(cv::Point2f(xValues[i], yValues[i]));
                        this->pointTableIndices.push_back(i);

                        if (!dim1Values.empty())
                        {
                            this->pointDim.push_back(dim1Values[i]);
                        }

                        if (!thicknesses.empty())
                        {
                            this->pointThickness.push_back(thicknesses[i]);
                        }

                        if (!colorInts.empty())
                        {
                            this->pointColors.push_back(toColorScalar(colorInts[i]));
                        }
                    }
                    else if ((types[i] == 2) && (dim1Values[i] >= 0))
                    {
                        // circle
                        this->circleCenters.push_back(cv::Point2f(xValues[i], yValues[i]));
                        this->circleTableIndices.push_back(i);
                        this->circleRadius.push_back(dim1Values[i]);

                        if (!thicknesses.empty())
                        {
                            this->circleThickness.push_back(thicknesses[i]);
                        }

                        if (!colorInts.empty())
                        {
                            this->circleColors.push_back(toColorScalar(colorInts[i]));
                        }
                    }
                    else if ((types[i] == 3) && !dim2Values.empty())
                    {
                        // line segments
                        this->lines.push_back(
                            std::pair<cv::Point2f, cv::Point2f>(cv::Point2f(xValues[i], yValues[i]), cv::Point2f(dim1Values[i], dim2Values[i])));
                        this->lineTableIndices.push_back(i);

                        if (!thicknesses.empty())
                        {
                            this->lineThickness.push_back(thicknesses[i]);
                        }

                        if (!colorInts.empty())
                        {
                            this->lineColors.push_back(toColorScalar(colorInts[i]));
                        }
                    }
                }
            }
            else
            {
                // null ptable because we use that to indicate empty or not
                this->ptable = nullptr;

                if (typeCol == nullptr)
                    throw std::runtime_error("No 'type' column in neighbor file.");
                if (xCol == nullptr)
                    throw std::runtime_error("No 'x' column in neighbor file.");
                if (yCol == nullptr)
                    throw std::runtime_error("No 'y' column in neighbor file.");
                if (dim1Col == nullptr)
                    throw std::runtime_error("No 'dim1' column in neighbor file.");
            }
        }
        else
        {
            this->clearShapeVectors();
        }
    }

    /**
     * @brief Check if there is a neighbor .geo.csv or .parquet file and try to load it if so.
     * This throws if there is a neighbor file but it doesn't meet any of the criteria (must have a "type" field, etc).
     * @param imagePath
     * @return Whether a file was loaded or not.
     */
    bool ShapeSet::tryLoadNeighborShapesFile(const wxFileName& imagePath)
    {
        wxFileName geoPath(imagePath);
        geoPath.SetExt("geo.csv");

        if (geoPath.FileExists())
        {
            this->ptable = ArrowUtil::loadFile(toNativeString(geoPath.GetFullPath()));
        }
        else
        {
            wxFileName parquetPath(imagePath);
            parquetPath.SetExt("parquet");

            if (parquetPath.FileExists())
            {
                this->ptable = ArrowUtil::loadFile(toNativeString(parquetPath.GetFullPath()));
            }
        }

        if (this->ptable != nullptr)
        {
            this->rebuildShapeVectors();
            return true;
        }

        return false;
    }

    /**
     * @brief For code repetition findShape. Check dist from target point (x,y) to pt, if less than
     * radius then add information to the vectors.
     */
    inline void accumClosePoint(float x, float y, cv::Point2f pt, float radius, int index, ShapeType shapeType, vector<cv::Point2f>& closePoints,
        vector<ShapeType>& closeTypes, vector<int>& closeIndices)
    {
        float dx = fabsf(x - pt.x);
        float dy = fabsf(y - pt.y);
        float dist = std::max(dx, dy);

        if (dist <= radius)
        {
            closePoints.push_back(pt);
            closeTypes.push_back(shapeType);
            closeIndices.push_back(index);
        }
    }

    /**
     * @brief Find a shape by location. This is post-filter because it uses the vectors.
     * Coincident points aren't handled, one of them is chosen arbitrarily based on order in the vectors.
     * This is the trivial implementation and thus slow. For perf probably need to index
     * the shapes, via tiling or kd-tree or something.
     * If shapes have limited density (cannot have too many shapes all in the same spot) then tiling is much
     * faster than kd-tree.
     * @param x
     * @param y
     * @param radius Chebyshev search radius. Rect uses cv::Rect2f::contains rather than radius.
     * @param type Output. The type of shape found.
     * @param idx Output. The index, within the specified type's vectors, of the found shape.
     * This will be -1 for none found.
     * @return True if any shape found within the specified radius.
     */
    bool ShapeSet::findShape(float x, float y, float radius, ShapeType& type, int& idx)
    {
        // Find all close shapes, then find closest of those.
        // Simple implementation for now, will certainly need some kind of index later.
        vector<cv::Point2f> closePoints;
        vector<ShapeType> closeTypes;
        vector<int> closeIndices;

        // points
        for (int i = 0; i < points.size(); i++)
        {
            accumClosePoint(x, y, points[i], radius, i, ShapeType::Point, closePoints, closeTypes, closeIndices);
        }

        // rect (use contains)
        cv::Point2f pt(x, y);

        for (int i = 0; i < rects.size(); i++)
        {
            if (rects[i].contains(pt))
            {
                closePoints.push_back(cv::Point2f(rects[i].x + rects[i].width / 2, rects[i].y + rects[i].height / 2));
                closeTypes.push_back(ShapeType::Rect);
                closeIndices.push_back(i);
            }
        }

        // circles (use circle radius)
        for (int i = 0; i < circleCenters.size(); i++)
        {
            accumClosePoint(x, y, circleCenters[i], circleRadius[i], i, ShapeType::Circle, closePoints, closeTypes, closeIndices);
        }

        // lines (just endpoints for simplicity and perf)
        for (int i = 0; i < lines.size(); i++)
        {
            accumClosePoint(x, y, lines[i].first, radius, i, ShapeType::LineSegment, closePoints, closeTypes, closeIndices);
            accumClosePoint(x, y, lines[i].second, radius, i, ShapeType::LineSegment, closePoints, closeTypes, closeIndices);
        }

        // find closest of the close-enough points
        float closestDist = std::numeric_limits<float>::infinity();
        idx = -1;

        for (int i = 0; i < closePoints.size(); i++)
        {
            float dx = closePoints[i].x - x;
            float dy = closePoints[i].y - y;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist < closestDist)
            {
                closestDist = dist;
                type = closeTypes[i];
                idx = closeIndices[i];
            }
        }

        return idx >= 0;
    }

    /**
     * @brief Get Arrow table index from shape type and shape index.
     * @param type Which shape type.
     * @param idx Index within this shape type.
     * @return The table index, or -1 for out of bounds.
     */
    int ShapeSet::getTableIndex(ShapeType type, int idx)
    {
        switch (type)
        {
        case ShapeType::Point:
            return this->pointTableIndices[idx];
        case ShapeType::Circle:
            return this->circleTableIndices[idx];
        case ShapeType::Rect:
            return this->rectTableIndices[idx];
        case ShapeType::LineSegment:
            return this->lineTableIndices[idx];
        default:
            throw std::runtime_error("ShapeSet unhandled shape type.");
        }
    }

    std::string ShapeSet::getValueString(int tableIdx, std::string colName)
    {
        auto col = this->ptable->GetColumnByName(colName);

        if (col)
        {
            auto result = col->GetScalar(tableIdx);

            if (result.ok())
            {
                auto val = *result;
                return val->ToString();
            }
        }

        return string();
    }

    void ShapeSet::applyFilter(FilterSpec& inFilterSpec)
    {
        if (!this->empty())
        {
            this->filterSpec = inFilterSpec;
            this->rebuildShapeVectors();
        }
    }

    int ShapeSet::size()
    {
        return this->ptable->num_rows();
    }

    std::vector<float> ShapeSet::getFloatValues(std::string colName)
    {
        std::vector<float> values;

        if (this->ptable)
        {
            auto col = this->ptable->GetColumnByName(colName);

            if (col)
            {
                // just convert everything to float for now
                std::vector<float> allValues;

                if (ArrowUtil::getFloatValues(col, allValues))
                {
                    if (!this->postFilterTableIndices.empty())
                    {
                        // have filter
                        values.reserve(this->postFilterTableIndices.size());

                        for (int i = 0; i < this->postFilterTableIndices.size(); i++)
                        {
                            values.push_back(allValues[this->postFilterTableIndices[i]]);
                        }
                    }
                    else
                    {
                        // no filter
                        return allValues;
                    }
                }
            }
        }

        return values;
    }
}
