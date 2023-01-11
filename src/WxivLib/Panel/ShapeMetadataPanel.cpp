// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include "WxWidgetsUtil.h"
#include <wx/splitter.h>

#include <fmt/core.h>
#include <opencv2/opencv.hpp>

#include "ShapeMetadataPanel.h"
#include <CvPlot/cvplot.h>

using namespace std;

namespace Wxiv
{
    /**
     * @brief Constructor.
     * @param parent
     * @param imagePanel We keep this in order to re/compute stats.
     */
    ShapeMetadataPanel::ShapeMetadataPanel(wxWindow* parent) : wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize)
    {
        build();
    }

    void ShapeMetadataPanel::setOnShapeFilterChangeCallback(std::function<void(ShapeSet& newShapes)> f)
    {
        this->onShapeFilterChangeCallback = f;
    }

    void ShapeMetadataPanel::setShapes(ShapeSet& newShapeSet)
    {
        if (newShapeSet.empty())
        {
            this->clearShapes();
        }
        else
        {
            this->shapes = newShapeSet;
            this->histPanel->setShapes(&this->shapes);
            this->refreshControls();
            this->ensureShapesTableRows();
            this->restoreFilterExpressions();
            this->rebuildAndApplyFilterSpec();
        }
    }

    /**
     * @brief Called when we switch to an image that has no shapes.
     */
    void ShapeMetadataPanel::clearShapes()
    {
        this->shapes.clear();
        this->histPanel->clearShapes();
        this->refreshControls();
        this->ensureShapesTableRows();
    }

    void ShapeMetadataPanel::saveConfig()
    {
        wxConfigBase::Get()->Write("ShapeMetadataPanelSashPosition", mainSplitter->GetSashPosition());
        this->histPanel->saveConfig();
    }

    void ShapeMetadataPanel::restoreConfig()
    {
        int sashPosition = wxConfigBase::Get()->Read("ShapeMetadataPanelSashPosition", 300);
        this->mainSplitter->SetSashPosition(sashPosition);
        this->histPanel->restoreConfig();
    }

    void ShapeMetadataPanel::build()
    {
        auto sizer = new wxBoxSizer(wxVERTICAL);

        const int defaultBorder = 4;
        const int labelBorderFlags = wxALL;
        const int labelBorder = 4;
        const int textBoxBorderFlags = wxALL;
        const int textBoxBorder = 4;
        const int labelFlags = wxFIXED | labelBorderFlags | wxALIGN_CENTER_VERTICAL;
        const int gridSizerGap = 1;

        // top controls (shape count, filtered shape count, ...)
        wxStaticBoxSizer* topControlsBoxSizer = new wxStaticBoxSizer(wxVERTICAL, this, "");
        auto topControlsGridSizer = new wxFlexGridSizer(3, 2, gridSizerGap, gridSizerGap);
        sizer->Add(topControlsBoxSizer, 0, wxEXPAND | wxALL, defaultBorder);
        topControlsBoxSizer->Add(topControlsGridSizer);

        // shape count
        topControlsGridSizer->Add(
            new wxStaticText(topControlsBoxSizer->GetStaticBox(), wxID_ANY, wxString("Shape Count")), 0, labelFlags, labelBorder);
        this->shapeCountTextBox = new wxStaticText(topControlsBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString);
        topControlsGridSizer->Add(shapeCountTextBox, 1, wxFIXED | textBoxBorderFlags, textBoxBorder);

        // filtered shape count
        topControlsGridSizer->Add(
            new wxStaticText(topControlsBoxSizer->GetStaticBox(), wxID_ANY, wxString("Filtered Shape Count")), 0, labelFlags, labelBorder);
        this->filteredShapeCountTextBox = new wxStaticText(topControlsBoxSizer->GetStaticBox(), wxID_ANY, wxEmptyString);
        topControlsGridSizer->Add(filteredShapeCountTextBox, 1, wxFIXED | textBoxBorderFlags, textBoxBorder);

        // clear filters button
        this->clearFiltersButton = new wxButton(topControlsBoxSizer->GetStaticBox(), wxID_ANY, "Clear Filters");
        ;
        topControlsGridSizer->Add(clearFiltersButton, 0, wxALL | wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL, 8);
        this->Bind(wxEVT_BUTTON, &ShapeMetadataPanel::onClearFiltersClicked, this, wxID_ANY);

        // vert splitter for grid and hist plot
        mainSplitter = new wxSplitterWindow(this, wxID_ANY, wxPoint(0, 400), wxDefaultSize, wxSP_BORDER | wxSP_LIVE_UPDATE);
        sizer->Add(mainSplitter, 1, wxEXPAND | wxALL, 5);

        // field values grid
        this->shapesGrid = new wxGrid(mainSplitter, -1);
        this->Bind(wxEVT_GRID_CELL_CHANGED, &ShapeMetadataPanel::onGridCellChanged, this, wxID_ANY);
        this->Bind(wxEVT_GRID_EDITOR_SHOWN, &ShapeMetadataPanel::onGridCellEditorShown, this, wxID_ANY);
        this->shapesGrid->Bind(wxEVT_CHAR, &ShapeMetadataPanel::onKeyDown, this, wxID_ANY);

        // hist and controls
        wxPanel* histPlotWrapPanel = new wxPanel(mainSplitter); // just because splitter needs a window
        wxStaticBoxSizer* histControlsBoxSizer = new wxStaticBoxSizer(wxVERTICAL, histPlotWrapPanel);
        histPanel = new ShapeMetadataHistPanel(histControlsBoxSizer->GetStaticBox());
        histControlsBoxSizer->Add(histPanel, 1, wxEXPAND);
        histPlotWrapPanel->SetSizerAndFit(histControlsBoxSizer);

        mainSplitter->SetMinimumPaneSize(100);
        mainSplitter->SplitHorizontally(this->shapesGrid, histPlotWrapPanel);

        this->SetSizerAndFit(sizer);
    }

    /**
     * @brief Remove the specified cell's entry from filterExpressions.
     */
    void ShapeMetadataPanel::clearFilterExpression(int row, int col)
    {
        auto key = this->shapesGrid->GetRowLabelValue(row).ToStdString();

        if (!key.empty() && this->filterExpressions.contains(key))
        {
            if (col == 1)
            {
                this->filterExpressions[key].first.clear();
            }
            else if (col == 2)
            {
                this->filterExpressions[key].second.clear();
            }

            // if empty can remove the whole thing
            if (this->filterExpressions[key].first.empty() && this->filterExpressions[key].second.empty())
            {
                this->filterExpressions.erase(key);
            }
        }
    }

    /**
     * @brief All this to delete cell contents rather than open for edit and delete first char.
     * @param event
     */
    void ShapeMetadataPanel::onKeyDown(wxKeyEvent& event)
    {
        if (event.GetKeyCode() == wxKeyCode::WXK_DELETE)
        {
            bool anyChanged = false;

            // cursor is not the same as selection
            {
                int col = this->shapesGrid->GetGridCursorCol();
                int row = this->shapesGrid->GetGridCursorRow();

                if ((col >= 0) && (row >= 0))
                {
                    if (!this->shapesGrid->GetCellValue(row, col).empty())
                    {
                        this->shapesGrid->SetCellValue(row, col, "");
                        this->recolorFilterExpressionCell(row, col);
                        this->clearFilterExpression(row, col);
                        anyChanged = true;
                    }
                }
            }

            // all kinds of ways to get selected cells, this seems to work
            for (const auto& block : this->shapesGrid->GetSelectedBlocks())
            {
                auto tl = block.GetTopLeft();
                auto br = block.GetBottomRight();

                for (int row = tl.GetRow(); row <= br.GetRow(); row++)
                {
                    for (int col = tl.GetCol(); col <= br.GetCol(); col++)
                    {
                        if (!this->shapesGrid->GetCellValue(row, col).empty())
                        {
                            this->shapesGrid->SetCellValue(row, col, "");
                            this->recolorFilterExpressionCell(row, col);
                            this->clearFilterExpression(row, col);
                            anyChanged = true;
                        }
                    }
                }
            }

            if (anyChanged)
            {
                this->rebuildAndApplyFilterSpec();

                // not calling Skip() means we eat this event
                return;
            }
        }

        // if we get here we decided not to do anything with this key, so skip it so rest of system can still see it
        event.Skip();
    }

    /**
     * @brief The filtering is applied in the shape set.
     */
    void ShapeMetadataPanel::onShapesFilterChange()
    {
        this->refreshControls();

        // hist panel has ref to shape set already
        this->histPanel->refreshControls();

        if (this->onShapeFilterChangeCallback)
        {
            this->onShapeFilterChangeCallback(this->shapes);
        }
    }

    void ShapeMetadataPanel::onClearFiltersClicked(wxCommandEvent& event)
    {
        for (int i = 0; i < this->shapesGrid->GetNumberRows(); i++)
        {
            this->shapesGrid->SetCellValue(i, 1, "");
            this->shapesGrid->SetCellValue(i, 2, "");
        }

        this->recolorFilterExpressionCells();
        this->filterExpressions.clear();
        this->rebuildAndApplyFilterSpec();
    }

    /**
     * @brief Set a filter value grid cell's background and foreground colors.
     * We highlight filter cells with values, and show invalid filter string with text color.
     */
    void ShapeMetadataPanel::recolorFilterExpressionCell(int row, int col)
    {
        wxColour normalFgColor = this->shapesGrid->GetForegroundColour();
        wxColour normalBgColor = this->shapesGrid->GetDefaultCellBackgroundColour();
        wxColour bgHighlightColor("yellow");
        wxColour errorFgColor("red");

        auto val = this->shapesGrid->GetCellValue(row, col);
        auto key = this->shapesGrid->GetRowLabelValue(row).ToStdString();

        if (val.empty())
        {
            this->shapesGrid->SetCellBackgroundColour(row, col, normalBgColor);
            this->shapesGrid->SetCellTextColour(row, col, normalFgColor);
        }
        else
        {
            ArrowFilterExpression e(key, col == 1 ? ArrowFilterOpEnum::Gte : ArrowFilterOpEnum::Lte, val.ToStdString());

            if (e.checkIsValid(this->shapes.ptable))
            {
                this->shapesGrid->SetCellBackgroundColour(row, col, bgHighlightColor);
                this->shapesGrid->SetCellTextColour(row, col, normalFgColor);
            }
            else
            {
                this->shapesGrid->SetCellBackgroundColour(row, col, normalBgColor);
                this->shapesGrid->SetCellTextColour(row, col, errorFgColor);
            }
        }
    }

    /**
     * @brief Set filter value grid cells background and foreground colors.
     * We highlight filter cells with values, and show invalid filter string with text color.
     */
    void ShapeMetadataPanel::recolorFilterExpressionCells()
    {
        for (int r = 0; r < this->shapesGrid->GetNumberRows(); r++)
        {
            for (int c : {1, 2})
            {
                recolorFilterExpressionCell(r, c);
            }
        }
    }

    /**
     * @brief Put filter expressions from our dictionary into the grid.
     */
    void ShapeMetadataPanel::restoreFilterExpressions()
    {
        for (int i = 0; i < this->shapesGrid->GetNumberRows(); i++)
        {
            string fieldName = this->shapesGrid->GetRowLabelValue(i).ToStdString();

            if (!fieldName.empty())
            {
                // restore filter expression from our dictionary
                if (this->filterExpressions.contains(fieldName))
                {
                    auto p = this->filterExpressions[fieldName];

                    for (auto e : {p.first, p.second})
                    {
                        if (e.op == ArrowFilterOpEnum::Gte)
                        {
                            this->shapesGrid->SetCellValue(i, 1, e.valueStr);
                        }
                        else if (e.op == ArrowFilterOpEnum::Gte)
                        {
                            this->shapesGrid->SetCellValue(i, 2, e.valueStr);
                        }
                    }
                }
                else
                {
                    this->shapesGrid->SetCellValue(i, 1, "");
                    this->shapesGrid->SetCellValue(i, 2, "");
                }
            }
        }

        this->recolorFilterExpressionCells();
    }

    /**
     * @brief Ensure rows for all metadata columns in shape metadata.
     * This applies saved filter expressions when initializing rows.
     */
    void ShapeMetadataPanel::ensureShapesTableRows()
    {
        wxColour normalFgColor = this->shapesGrid->GetForegroundColour();
        wxColour normalBgColor = this->shapesGrid->GetDefaultCellBackgroundColour();

        if (!this->shapes.empty() && (this->shapes.ptable != nullptr))
        {
            // a row per column in metadata
            auto ptable = this->shapes.ptable;
            vector<string> fieldNames = ptable->ColumnNames();
            std::sort(fieldNames.begin(), fieldNames.end());

            // only create once
            if (this->shapesGrid->GetNumberCols() != 3)
            {
                this->shapesGrid->CreateGrid(fieldNames.size(), 3);
                this->shapesGrid->SetColLabelValue(0, "Value");
                this->shapesGrid->SetColLabelValue(1, "Filter >=");
                this->shapesGrid->SetColLabelValue(2, "Filter <=");
                this->shapesGrid->SetColLabelAlignment(wxALIGN_CENTER_HORIZONTAL, wxALIGN_CENTER_VERTICAL);
                this->shapesGrid->SetRowLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTER_VERTICAL);

                // col widths
                this->shapesGrid->SetColMinimalAcceptableWidth(50);
                this->shapesGrid->SetColMinimalWidth(0, 80); // value col shows floats

                // fit cols to label widths
                for (int i = 0; i < this->shapesGrid->GetNumberCols(); i++)
                {
                    this->shapesGrid->AutoSizeColLabelSize(i);
                }
            }

            // ensure number of rows
            if (this->shapesGrid->GetNumberRows() < fieldNames.size())
            {
                // add rows for current number of fields
                int n = fieldNames.size() - this->shapesGrid->GetNumberRows();
                this->shapesGrid->AppendRows(n);
            }
            else if (this->shapesGrid->GetNumberRows() > fieldNames.size())
            {
                this->shapesGrid->DeleteRows(0, this->shapesGrid->GetNumberRows() - fieldNames.size());
            }

            // row labels
            for (int i = 0; i < fieldNames.size(); i++)
            {
                string fieldName = fieldNames[i];

                if (this->shapesGrid->GetRowLabelValue(i) != fieldName)
                {
                    // initializing or changing which field this row is for
                    this->shapesGrid->SetRowLabelValue(i, fieldName);
                    this->shapesGrid->SetReadOnly(i, 0, true);
                    this->shapesGrid->SetReadOnly(i, 1, false);
                    this->shapesGrid->SetReadOnly(i, 2, false);

                    this->shapesGrid->SetCellValue(i, 1, "");
                    this->shapesGrid->SetCellValue(i, 2, "");
                    this->shapesGrid->SetCellBackgroundColour(i, 1, normalBgColor);
                    this->shapesGrid->SetCellTextColour(i, 2, normalFgColor);
                }
            }

            this->shapesGrid->SetColLabelSize(wxGRID_AUTOSIZE);
            this->shapesGrid->SetRowLabelSize(wxGRID_AUTOSIZE);
        }
        else
        {
            this->shapesGrid->ClearGrid();

            // ClearGrid doesn't clear row labels
            for (int i = 0; i < this->shapesGrid->GetNumberRows(); i++)
            {
                this->shapesGrid->SetRowLabelValue(i, "");
                this->shapesGrid->SetCellBackgroundColour(i, 1, normalBgColor);
                this->shapesGrid->SetCellTextColour(i, 2, normalFgColor);
            }
        }
    }

    void setComboBoxMinimumWidth(wxComboBox* box)
    {
        int maxWidth = 0;

        for (int i = 0; i < (int)box->GetCount(); i++)
        {
            int width;
            box->GetTextExtent(box->GetString(i), &width, NULL);
            maxWidth = std::max(maxWidth, width);
        }

        box->SetMinSize(wxSize(maxWidth + 8, -1));
    }

    void ShapeMetadataPanel::refreshControls()
    {
        // set the count textboxes
        if (!this->shapes.empty())
        {
            this->shapeCountTextBox->SetLabel(fmt::format("{}", this->shapes.getTotalCount()));
            this->filteredShapeCountTextBox->SetLabel(fmt::format("{}", this->shapes.getFilteredCount()));
        }
        else
        {
            this->shapeCountTextBox->SetLabel("");
            this->filteredShapeCountTextBox->SetLabel("");
        }
    }

    /**
     * @brief Build a filter spec from contents of the grid.
     * This sets values in filterExpressions but note that only fields present in this image are set.
     */
    FilterSpec ShapeMetadataPanel::buildFilterSpec()
    {
        FilterSpec filterSpec;
        const int gteColIndex = 1;
        const int lteColIndex = 2;

        for (int i = 0; i < this->shapesGrid->GetNumberRows(); i++)
        {
            auto key = this->shapesGrid->GetRowLabelValue(i).ToStdString();
            auto gteValue = this->shapesGrid->GetCellValue(i, gteColIndex);
            auto lteValue = this->shapesGrid->GetCellValue(i, lteColIndex);

            if (!key.empty())
            {
                // preserve in filterExpressions, even if empty
                std::pair<ArrowFilterExpression, ArrowFilterExpression> filterExpressionPair;

                if (!gteValue.empty())
                {
                    ArrowFilterExpression e(key, ArrowFilterOpEnum::Gte, gteValue.ToStdString());

                    if (e.checkIsValid(this->shapes.ptable))
                    {
                        filterSpec.expressions.push_back(e);
                        filterExpressionPair.first = e;
                    }
                }

                if (!lteValue.empty())
                {
                    ArrowFilterExpression e(key, ArrowFilterOpEnum::Lte, lteValue.ToStdString());

                    if (e.checkIsValid(this->shapes.ptable))
                    {
                        filterSpec.expressions.push_back(e);
                        filterExpressionPair.second = e;
                    }
                }

                // put in our expressions
                if (!filterExpressionPair.first.empty() || !filterExpressionPair.second.empty())
                {
                    this->filterExpressions[key] = filterExpressionPair;
                }
            }
        }

        return filterSpec;
    }

    /**
     * @brief rebuild filter spec from the contents of the grid, and apply the new filter spec.
     */
    void ShapeMetadataPanel::rebuildAndApplyFilterSpec()
    {
        FilterSpec filterSpec = this->buildFilterSpec();
        this->shapes.applyFilter(filterSpec);
        this->onShapesFilterChange();
    }

    /**
     * @brief To set color back to normal so that it doesn't look red during editing.
     * TODO: This should probably use wxValidator.
     */
    void ShapeMetadataPanel::onGridCellEditorShown(wxGridEvent& event)
    {
        wxColour normalFgColor = this->shapesGrid->GetForegroundColour();
        this->shapesGrid->SetCellTextColour(event.GetRow(), event.GetCol(), normalFgColor);
    }

    /**
     * @brief Done changing.
     * TODO: This should probably use wxValidator.
     */
    void ShapeMetadataPanel::onGridCellChanged(wxGridEvent& event)
    {
        int row = event.GetRow();
        int col = event.GetCol();

        if ((col == 1) || (col == 2))
        {
            this->clearFilterExpression(row, col);
            this->rebuildAndApplyFilterSpec();
            recolorFilterExpressionCell(row, col);
        }
    }

    /**
     * @brief When the mouse-over shape changes, including changing to having no nearby shape,
     * as indicated by idx == -1.
     * The index is the index in the ShapeSet.
     * @param type
     * @param idx
     */
    void ShapeMetadataPanel::onMouseOverShapeChange(ShapeType type, int idx)
    {
        // idx == -1 means no shape
        if (!this->shapes.emptyPostFilter() && (idx >= 0))
        {
            int tableIndex = this->shapes.getTableIndex(type, idx); // the index into the Arrow table of this shape

            // grid
            // for each row in our table, try to get value for this shape from arrow table
            for (int i = 0; i < this->shapesGrid->GetNumberRows(); i++)
            {
                string key = this->shapesGrid->GetRowLabelValue(i).ToStdString();
                string value = this->shapes.getValueString(tableIndex, key);
                this->shapesGrid->SetCellValue(i, 0, value);
            }
        }
        else
        {
            // grid
            for (int i = 0; i < this->shapesGrid->GetNumberRows(); i++)
            {
                this->shapesGrid->SetCellValue(i, 0, "");
            }
        }
    }
}
