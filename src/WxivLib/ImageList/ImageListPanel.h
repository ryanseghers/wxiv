// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <string>
#include <vector>

#include "WxWidgetsUtil.h"
#include <wx/splitter.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>

#include <opencv2/opencv.hpp>

#include "ImageListSource.h"

namespace Wxiv
{
    /**
     * @brief A panel with a filter box and a listview to show list of images.
     *
     * The wxListView items have Data which is the original index of the image in the imageListSource.
     */
    class ImageListPanel : public wxPanel
    {
        bool doCallbacks = true; // to avoid recursion from event handlers
        wxTextCtrl* filterTextBox = nullptr;
        wxListView* listView = nullptr;
        std::shared_ptr<ImageListSource> imageListSource;

        /**
         * @brief For example on new source or filter change.
         */
        std::function<void(void)> onListItemsChangeCallback;
        std::function<void(void)> onSelectionChangeCallback;

        void rebuildList();

        void insertListViewItem(int origIdx, std::shared_ptr<WxivImage> image);
        void onItemSelected(int idx);
        void onFilterTextChanged(wxCommandEvent& evt);
        std::vector<int> getSelectedListViewIndices();
        int listViewIndexToDataIndex(int listViewIndex);
        void selectImageByDataIndex(int origIdx);
        void toggleSelectedItemsCheckboxes();
        void setupIcons();
        int getNextImage(int idx, bool doReverse);

        void onListRightClick(wxListEvent& evt);
        void onPopupMenuClick(wxCommandEvent& evt);
        std::vector<int> getCheckedDataIndices();
        void checkBoxByDataIndex(int idx, bool checked);
        int findImageByDataIndex(int origIdx);
        void selectImage(int idx, bool isSelected);

      public:
        ImageListPanel(wxWindow* parent);

        void setOnListItemsChangeCallback(const std::function<void(void)>& f);
        void setOnSelectionChangeCallback(const std::function<void(void)>& f);
        void setSource(std::shared_ptr<ImageListSource> source);

        bool selectImage(const wxString& path);
        void selectNextImage(bool doReverse);
        void unselectAll();
        void updateForMultiPageImageLoaded();

        int getVisibleItemCount();
        int getSelectedItemCount();
        std::shared_ptr<WxivImage> getSelectedImage();
        std::shared_ptr<WxivImage> getImageByDataIndex(int origIdx);
        int getSelectedImageDataIndex();
        std::vector<std::shared_ptr<WxivImage>> getCheckedImages();
        std::vector<std::shared_ptr<WxivImage>> getSelectedImages();
        std::vector<std::shared_ptr<WxivImage>> getSelectedOrCheckedImages();
        bool checkAnySelectedOrCheckedImages();

        void saveConfig();
        void restoreConfig();
    };
}
