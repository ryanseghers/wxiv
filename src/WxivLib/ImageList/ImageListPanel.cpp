// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <vector>
#include <cmath>
#include <filesystem>
#include <fmt/core.h>

#include "WxWidgetsUtil.h"
#include <wx/wxprec.h>
#include <wx/splitter.h>
#include <wx/confbase.h>
#include <wx/clipbrd.h>
#include <wx/filename.h>

#include <opencv2/opencv.hpp>

#include "ImageListPanel.h"
#include "ImageUtil.h"
#include "MiscUtil.h"
#include "WxivUtil.h"
#include "StringUtil.h"

using namespace std;
namespace fs = std::filesystem;

#define ID_POPUP_COPY_NAME 2001
#define ID_POPUP_COPY_PATH 2002
#define ID_POPUP_TOGGLE_CHECKBOXES 2003

namespace Wxiv
{
    ImageListPanel::ImageListPanel(wxWindow* parent) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL)
    {
        auto mainSizer = new wxBoxSizer(wxVERTICAL);

        // filter label and text box
        auto filterLineSizer = new wxBoxSizer(wxHORIZONTAL);
        filterLineSizer->Add(new wxStaticText(this, wxID_ANY, "Filter"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
        filterTextBox = new wxTextCtrl(this, wxID_ANY, "");
        filterLineSizer->Add(filterTextBox, 1, wxALIGN_CENTER_VERTICAL | wxALL, 4);
        filterTextBox->Bind(wxEVT_TEXT, [&](wxCommandEvent& evt) { onFilterTextChanged(evt); });
        mainSizer->Add(filterLineSizer, 0, wxEXPAND | wxTOP, 8);

        // listview
        // trying to set styles for sort, but changes column display too: , -1, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SORT_ASCENDING
        this->listView = new wxListView(this);
        this->listView->EnableCheckBoxes(true);
        listView->AppendColumn("Name");
        listView->AppendColumn("Type");
        listView->SetColumnWidth(0, 200);
        listView->SetColumnWidth(1, 80);
        mainSizer->Add(this->listView, 1, wxEXPAND | wxALL, 8);

        listView->Bind(wxEVT_LIST_ITEM_SELECTED, [&](wxListEvent& evt) { onItemSelected(evt.m_itemIndex); });

        // col header click handler (would be used for sorting)
        listView->Bind(wxEVT_LIST_COL_CLICK, [&](wxListEvent& evt) { wxMessageBox("Clicked list col"); });

        listView->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, [&](wxListEvent& evt) { onListRightClick(evt); });

        this->SetSizerAndFit(mainSizer);
    }

    void ImageListPanel::saveConfig()
    {
        wxConfigBase::Get()->Write("ImageListPanelCol0Width", listView->GetColumnWidth(0));
        wxConfigBase::Get()->Write("ImageListPanelCol1Width", listView->GetColumnWidth(1));
    }

    void ImageListPanel::restoreConfig()
    {
        listView->SetColumnWidth(0, wxConfigBase::Get()->ReadLong("ImageListPanelCol0Width", 200));
        listView->SetColumnWidth(1, wxConfigBase::Get()->ReadLong("ImageListPanelCol1Width", 80));
    }

    /**
     * @brief This maybe works but I did not get the icon to be rendered with items.
     */
    void ImageListPanel::setupIcons()
    {
        // define icons
        wxImageList* imageList = new wxImageList(32, 32);
        auto iconPath = _T("C:\\Dev\\wxWidgets\\include\\wx\\msw\\floppy.ico");
        wxIconLocation loc(iconPath);
        wxIcon icon(loc);

        if (icon.IsOk())
        {
            imageList->Add(icon);
        }

        // wxIcon* icon = IconGetter::GetExecutableIcon(_T("c:\\windows\\system32\\shell32.dll"), 32, i);

        // if (icon)
        //{
        //    imageList->Add(*icon);
        //}

        listView->AssignImageList(imageList, wxIMAGE_LIST_NORMAL);
    }

    void ImageListPanel::setOnSelectionChangeCallback(const std::function<void(void)>& f)
    {
        this->onSelectionChangeCallback = f;
    }

    void ImageListPanel::setOnListItemsChangeCallback(const std::function<void(void)>& f)
    {
        this->onListItemsChangeCallback = f;
    }

    void ImageListPanel::insertListViewItem(int origIdx, std::shared_ptr<WxivImage> image)
    {
        wxListItem item0;
        item0.SetId(origIdx);
        item0.SetData(origIdx);
        item0.SetText(image->getDisplayName());

        //// icon: did not work
        // int imageIndex = 0;
        // item0.SetImage(imageIndex);

        // this is the only api to add an item
        int listIndex = listView->InsertItem(item0);

        // set the type column
        listView->SetItem(listIndex, 1, image->getTypeStr());
    }

    /**
     * @brief Get a vector of the original list indices (data indices) of items whose checkboxes are checked.
     * This includes ones that are not visible due to filter.
     */
    std::vector<int> ImageListPanel::getCheckedDataIndices()
    {
        vector<int> checkedDataIndices;
        int n = this->listView->GetItemCount();

        if (n > 0)
        {
            for (int i = 0; i < n; i++)
            {
                if (this->listView->IsItemChecked(i))
                {
                    long thisOrigIndex = (long)this->listView->GetItemData(i);
                    checkedDataIndices.push_back(thisOrigIndex);
                }
            }
        }

        return checkedDataIndices;
    }

    void ImageListPanel::onPopupMenuClick(wxCommandEvent& evt)
    {
        if (evt.GetId() == ID_POPUP_TOGGLE_CHECKBOXES)
        {
            this->toggleSelectedItemsCheckboxes();
        }
        else if ((evt.GetId() == ID_POPUP_COPY_PATH) || (evt.GetId() == ID_POPUP_COPY_NAME))
        {
            std::shared_ptr<WxivImage> image = this->getSelectedImage();

            if (image)
            {
                bool doName = (evt.GetId() == ID_POPUP_COPY_NAME);
                copyImageNameOrPathToClipboard(image, doName);
            }
        }
    }

    void ImageListPanel::onListRightClick(wxListEvent& evt)
    {
        void* data = reinterpret_cast<void*>(evt.GetItem().GetData());
        wxMenu mnu;
        mnu.SetClientData(data);

        if (this->getSelectedItemCount() == 1)
        {
            mnu.Append(ID_POPUP_COPY_NAME, "Copy Name");
            mnu.Append(ID_POPUP_COPY_PATH, "Copy Path");
        }

        mnu.Append(ID_POPUP_TOGGLE_CHECKBOXES, "Toggle Checkbox");
        mnu.Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ImageListPanel::onPopupMenuClick), NULL, this);
        PopupMenu(&mnu);
    }

    void ImageListPanel::toggleSelectedItemsCheckboxes()
    {
        vector<int> indices = getSelectedListViewIndices();

        for (int i : indices)
        {
            this->listView->CheckItem(i, !this->listView->IsItemChecked(i));
        }
    }

    /**
     * @brief From an original data index, set item's checkbox.
     */
    void ImageListPanel::checkBoxByDataIndex(int idx, bool checked)
    {
        int n = this->listView->GetItemCount();

        for (int i = 0; i < n; i++)
        {
            long thisOrigIndex = (long)this->listView->GetItemData(i);

            if (idx == thisOrigIndex)
            {
                this->listView->CheckItem(i, checked);
            }
        }
    }

    /**
     * @brief Rebuild list from items in source.
     */
    void ImageListPanel::rebuildList()
    {
        vector<int> checkedDataIndices = getCheckedDataIndices();
        this->listView->DeleteAllItems();

        int n = this->imageListSource->getImageCount();

        if (n > 0)
        {
            // only ones that pass the filter
            wxString filter = this->filterTextBox->GetValue();

            for (int i = 0; i < n; i++)
            {
                std::shared_ptr<WxivImage> img = this->imageListSource->getImage(i);

                if (filter.empty() || img->getPath().GetName().Contains(filter))
                {
                    this->insertListViewItem(i, img);
                }
            }
        }

        // re-apply checkboxes
        if (!checkedDataIndices.empty())
        {
            for (int i : checkedDataIndices)
            {
                checkBoxByDataIndex(i, true);
            }
        }

        if (this->doCallbacks && this->onListItemsChangeCallback)
        {
            this->onListItemsChangeCallback();
        }
    }

    int ImageListPanel::getVisibleItemCount()
    {
        return this->listView->GetItemCount();
    }

    void ImageListPanel::setSource(std::shared_ptr<ImageListSource> source)
    {
        this->imageListSource = source;
        this->rebuildList();
    }

    /**
     * @brief In order to also focus.
     */
    void ImageListPanel::selectImage(int idx, bool isSelected)
    {
        this->listView->Select(idx, isSelected);

        if (isSelected)
        {
            this->listView->Focus(idx);
        }
    }

    /**
     * @brief Select image (and unselected everything else) if not already and if found.
     * @param path
     * @return Whether image is now selected or not.
     */
    bool ImageListPanel::selectImage(const wxString& path)
    {
        wxString basename = wxFileName(path).GetName();
        long idx = this->listView->FindItem(0, basename);

        if ((idx >= 0) && !this->listView->IsSelected(idx))
        {
            // don't need to do callback twice
            this->doCallbacks = false;
            this->unselectAll();
            this->doCallbacks = true;

            this->selectImage(idx, true);
            return true;
        }
        else
        {
            return false;
        }
    }

    void ImageListPanel::onItemSelected(int idx)
    {
        if (this->doCallbacks && this->onSelectionChangeCallback)
        {
            this->onSelectionChangeCallback();
        }
    }

    int ImageListPanel::getSelectedItemCount()
    {
        return this->listView->GetSelectedItemCount();
    }

    bool ImageListPanel::checkAnySelectedOrCheckedImages()
    {
        std::vector<int> checkedIndices = getCheckedDataIndices();
        return getSelectedItemCount() > 0 || checkedIndices.size() > 0;
    }

    /**
     * @brief Ordering and filtering mean index in ListView is not index in original data.
     * @return
     */
    int ImageListPanel::listViewIndexToDataIndex(int listViewIndex)
    {
        long origIndex = (long)this->listView->GetItemData(listViewIndex);
        return origIndex;
    }

    /**
     * @brief Used for filter changes.
     * @param origIdx
     * @return The found index, or -1 for not found.
     */
    int ImageListPanel::findImageByDataIndex(int origIdx)
    {
        int n = this->listView->GetItemCount();

        // iterate items in list because not all visible
        if ((n > 0) && (origIdx >= 0))
        {
            int lastIdx = -1;

            while ((lastIdx = this->listView->GetNextItem(lastIdx)) >= 0)
            {
                long thisOrigIndex = (long)this->listView->GetItemData(lastIdx);

                if (thisOrigIndex == origIdx)
                {
                    return lastIdx;
                }
            }
        }

        return -1;
    }

    std::shared_ptr<WxivImage> ImageListPanel::getSelectedImage()
    {
        if (this->listView->GetSelectedItemCount() == 1)
        {
            int listViewIndex = (int)this->listView->GetFirstSelected();
            long origIndex = listViewIndexToDataIndex(listViewIndex);
            return imageListSource->getImage(origIndex);
        }
        else
        {
            return nullptr;
        }
    }

    /**
     * @brief Get the orig data index, not list view index.
     * @return
     */
    int ImageListPanel::getSelectedImageDataIndex()
    {
        int idx = (int)this->listView->GetFirstSelected();

        if (idx >= 0)
        {
            long origIndex = listViewIndexToDataIndex(idx);
            return origIndex;
        }
        else
        {
            return -1;
        }
    }

    /**
     * @brief Get listview indices for selected items (not translated to original data indices).
     * @return
     */
    vector<int> ImageListPanel::getSelectedListViewIndices()
    {
        vector<int> v;
        int idx = (int)this->listView->GetFirstSelected();

        while (idx >= 0)
        {
            v.push_back(idx);
            idx = (int)this->listView->GetNextSelected(idx);
        }

        return v;
    }

    /**
     * @brief Get selected images, if any.
     */
    vector<std::shared_ptr<WxivImage>> ImageListPanel::getSelectedImages()
    {
        vector<std::shared_ptr<WxivImage>> v;
        int idx = (int)this->listView->GetFirstSelected();

        while (idx >= 0)
        {
            long origIndex = listViewIndexToDataIndex(idx);
            v.push_back(getImageByDataIndex(origIndex));
            idx = (int)this->listView->GetNextSelected(idx);
        }

        return v;
    }

    void ImageListPanel::unselectAll()
    {
        vector<int> v = getSelectedListViewIndices();
        this->SetEvtHandlerEnabled(false); // don't need these selection changed events, at leaset for now

        for (int idx : v)
        {
            this->selectImage(idx, false);
        }

        this->SetEvtHandlerEnabled(true);

        if (this->doCallbacks && this->onSelectionChangeCallback)
        {
            this->onSelectionChangeCallback();
        }
    }

    /**
     * @brief Get next or prior image from the specified index, handling checkbox case.
     * Magic return -1 means none.
     */
    int ImageListPanel::getNextImage(int idx, bool doReverse)
    {
        int n = this->listView->GetItemCount();

        if (n > 0)
        {
            // handle case where none yet selected
            if (idx < 0)
            {
                // nothing selected so select first or last
                idx = doReverse ? n - 1 : 0;
            }
            else
            {
                // there is one selected
                idx = doReverse ? idx - 1 : idx + 1;
            }

            // now we have a target index, but account for checkboxes
            if (this->listView->HasCheckBoxes())
            {
                if (!getCheckedDataIndices().empty())
                {
                    // some are checked so we scan to the next checked one
                    int delta = doReverse ? -1 : 1;

                    for (int i = idx; i >= 0 && i < n; i += delta)
                    {
                        if (this->listView->IsItemChecked(i))
                        {
                            // found one
                            return i;
                        }
                    }

                    // none in this direction
                    return -1;
                }
                else
                {
                    // all unchecked so already have good index
                    return idx;
                }
            }
            else
            {
                // no checkboxes so already have good index
                return idx;
            }
        }
        else
        {
            return -1;
        }
    }

    void ImageListPanel::selectNextImage(bool doReverse)
    {
        int n = this->listView->GetItemCount();

        if (n > 0)
        {
            int idx = (int)this->listView->GetFirstSelected();
            int newIdx = getNextImage(idx, doReverse);

            if (newIdx >= 0)
            {
                this->unselectAll();
                this->selectImage(newIdx, true);
            }
        }
    }

    /**
     * @brief Selects the specified image and deselects all others.
     * Used for filter changes.
     * @param origIdx
     */
    void ImageListPanel::selectImageByDataIndex(int origIdx)
    {
        int n = this->listView->GetItemCount();
        int idx = findImageByDataIndex(origIdx);

        if (idx >= 0)
        {
            for (int i = 0; i < n; i++)
            {
                if (i == idx)
                {
                    this->selectImage(i, true);
                }
                else
                {
                    this->selectImage(i, false);
                }
            }
        }
    }

    /**
     * @brief Get ref to orig, find it by original data index.
     * @param origIdx
     */
    std::shared_ptr<WxivImage> ImageListPanel::getImageByDataIndex(int origIdx)
    {
        return imageListSource->getImage(origIdx);
    }

    void ImageListPanel::onFilterTextChanged(wxCommandEvent& evt)
    {
        // right now updating list on-the-fly, so quickly, so don't try to preserve image load
        int origIndex = getSelectedImageDataIndex();
        this->unselectAll();
        this->rebuildList();
        int n = this->listView->GetItemCount();

        if ((n > 0) && (origIndex >= 0))
        {
            // one was selected, try select if in list
            selectImageByDataIndex(origIndex);
        }
    }

    /**
     * @brief The currently selected image was just loaded, and it has secondary pages.
     */
    void ImageListPanel::updateForMultiPageImageLoaded()
    {
        int idx = (int)this->listView->GetFirstSelected();

        this->doCallbacks = false;
        this->unselectAll();
        this->rebuildList();

        if (idx >= 0)
        {
            // there is one selected
            this->selectImage(idx, true);
        }

        this->doCallbacks = true;
    }

    std::vector<std::shared_ptr<WxivImage>> ImageListPanel::getCheckedImages()
    {
        std::vector<int> indices = getCheckedDataIndices();
        std::vector<std::shared_ptr<WxivImage>> images;

        for (int idx : indices)
        {
            images.push_back(getImageByDataIndex(idx));
        }

        return images;
    }

    /**
     * @brief If any are checked, get checked images, else get selected images.
     * @return
     */
    std::vector<std::shared_ptr<WxivImage>> ImageListPanel::getSelectedOrCheckedImages()
    {
        std::vector<std::shared_ptr<WxivImage>> checkedImages = getCheckedImages();

        if (!checkedImages.empty())
        {
            return checkedImages;
        }

        return getSelectedImages();
    }
}
