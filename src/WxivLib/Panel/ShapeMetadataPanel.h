// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include "WxWidgetsUtil.h"
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/grid.h>

#include <opencv2/opencv.hpp>
#include "WxivImage.h"
#include "ImageViewPanel.h"
#include "ImageViewPanelSettings.h"
#include "ImageScrollPanel.h"
#include "ShapeMetadataHistPanel.h"

namespace Wxiv
{
    /**
     * @brief This has a combo box to select subject of the stats (whole image, current view, roi, etc), a table with stats,
     * and a histogram.
     *
     * The table has metadata field values and filter values.
     * Currently just gte and lte filter values, if want to get more sophisticated probably build rows of filter spec controls
     * instead of embed in a table.
     */
    class ShapeMetadataPanel : public wxPanel
    {
        // taking a copy of this, and modifying it as we filter
        ShapeSet shapes;

        wxStaticText* shapeCountTextBox = nullptr;
        wxStaticText* filteredShapeCountTextBox = nullptr;
        wxButton* clearFiltersButton = nullptr;

        wxSplitterWindow* mainSplitter = nullptr; // vert splitter between table and histogram
        wxGrid* shapesGrid = nullptr;

        ShapeMetadataHistPanel* histPanel = nullptr;

        // callback when user changes the shape set via filter change
        std::function<void(ShapeSet& newShapes)> onShapeFilterChangeCallback;

        // to preserve superset of filter expressions as user steps through images that have different metadata fields
        // the pair has gte and lte expressions
        std::unordered_map<std::string, std::pair<ArrowFilterExpression, ArrowFilterExpression>> filterExpressions;

        void onGridCellChanged(wxGridEvent& event);
        void onGridCellEditorShown(wxGridEvent& event);

        void ensureShapesTableRows();
        void recolorFilterExpressionCell(int row, int col);
        void recolorFilterExpressionCells();
        void restoreFilterExpressions();

        void refreshControls();
        void rebuildAndApplyFilterSpec();
        void clearFilterExpression(int row, int col);
        void onShapesFilterChange();
        FilterSpec buildFilterSpec();
        void onClearFiltersClicked(wxCommandEvent& event);
        void onKeyDown(wxKeyEvent& event);

        void build();

      public:
        ShapeMetadataPanel(wxWindow* parent);

        void setOnShapeFilterChangeCallback(std::function<void(ShapeSet& newShapes)> f);

        void onMouseOverShapeChange(ShapeType type, int idx);
        void setShapes(ShapeSet& newShapeSet);
        void clearShapes();
        void saveConfig();
        void restoreConfig();
    };
}