// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <wx/wxprec.h>
#include <wx/splitter.h>
#include <wx/notebook.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <opencv2/opencv.hpp>

#include "ImageScrollPanel.h"
#include "WxivMainSplitWindow.h"
#include "ImageListPanel.h"
#include "ImageListSource.h"

const std::string WxivVersion = "0.0.1";

namespace Wxiv
{
    /**
     * @brief Top main frame class.
     */
    class WxivMainFrame : public wxFrame
    {
      public:
        WxivMainFrame();

      private:
        wxString lastOpenDir;
        ImageListPanel* imageListPanel = nullptr;
        WxivMainSplitWindow* mainSplitWindow = nullptr;
        std::shared_ptr<ImageListSource> imageListSource;
        wxSplitterWindow* mainSplitter;

        wxMenuBar* mainMenuBar = nullptr;
        wxMenu* menuHelp = nullptr;
        wxMenu* menuFile = nullptr;
        wxMenu* menuTools = nullptr;
        wxMenu* menuOptions = nullptr;

        wxMenuItem* doRenderShapesMenuItem = nullptr;

        // keep lists of menu items to enable/disable based on what is going on
        std::vector<wxMenuItem*> menuItemsForAnyImagesListed;
        std::vector<wxMenuItem*> menuItemsForSingleImageSelected;
        std::vector<wxMenuItem*> menuItemsForAnySelectedOrChecked;

        void OnClose(wxCloseEvent& evt);

        void setupIcons();
        void buildMenus();
        void enableDisableMenuItems();
        void buildFileMenu(wxMenuBar* menuBar);
        void buildHelpMenu(wxMenuBar* menuBar);
        void buildToolsMenu(wxMenuBar* menuBar);
        void buildOptionsMenu(wxMenuBar* menuBar);
        void buildMainLayout();
        void restoreWindow();
        void restoreConfig();
        wxString getDefaultSaveImageName();

        void loadDir(wxString dirPath);
        void loadImage(wxString path);
        void loadImageAndDir(wxString path);
        void saveImage(wxString path);

        void alert(const char* msg);
        void alert(const wxString& msg);
        void alert(const std::string& msg);
        void onAbout(wxCommandEvent& event);
        void showTextFile(wxString title, std::string fileName);
        void onHelp(wxCommandEvent& event);
        void onReleaseNotes(wxCommandEvent& event);
        void onOpenFile(wxCommandEvent& event);
        void onOpenDir(wxCommandEvent& event);
        void onOpenLast(wxCommandEvent& event);
        void onReloadDir(wxCommandEvent& event);
        void onImageListSelectionChange();
        void onImageListItemsChange();
        void onSaveImage(wxCommandEvent& event);
        void onSaveViewToFile(wxCommandEvent& event);
        void onSaveToGif(wxCommandEvent& event);
        void saveGifUsingWxGIFHandler(std::vector<std::shared_ptr<WxivImage>>& checkedImages, wxString path);
        void onCopyViewToClipboard(wxCommandEvent& event);
        void onCopyFileName(wxCommandEvent& event);
        void onCopyFilePath(wxCommandEvent& event);

        void onNextImage(wxCommandEvent& event);
        void onPreviousImage(wxCommandEvent& event);

        // Options
        void onToggleShapeRender(wxCommandEvent& event);

        // Tools
        void onFitViewToImage(wxCommandEvent& event);
    };

    enum
    {
        ID_OpenFile = 1,
        ID_OpenDir = 2,
        ID_OpenLast = 3,
        ID_NextImage = 4,
        ID_PreviousImage = 5,
        ID_FitViewToImage = 6,
        ID_SaveFile = 7,
        ID_SaveViewToFile = 8,
        ID_CopyViewToClipboard = 9,
        ID_ClipFileName = 10,
        ID_ClipFilePath = 11,
        ID_ReloadDir = 12,
        ID_SaveToGif = 13,
        ID_ToggleShapeRender = 14,
        ID_ShowReleaseNotes = 15,
    };

    class WxivApp : public wxApp
    {
        WxivMainFrame* frame = nullptr;

      public:
        virtual bool OnInit();
        // int FilterEvent(wxEvent& event);
    };
}
