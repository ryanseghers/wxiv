// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <string>
#include <filesystem>
#include <fmt/core.h>
#include <fmt/xchar.h>
#include <opencv2/opencv.hpp>

#include "WxivMainFrame.h"
#include "ShapeSet.h"
#include "MiscUtil.h"
#include "WxivImage.h"
#include "ImageUtil.h"
#include "ImageListSourceDirectory.h"
#ifdef DO_DICOM
#include "ImageListSourceDcmDirectory.h"
#endif
#include "WxivUtil.h"
#include "TextDisplayDialog.h"
#include "StringUtil.h"
#include "WxWidgetsUtil.h"
#include "CollageSpecDialog.h"
#include "VectorUtil.h"

#include <wx/stdpaths.h>
#include <wx/config.h>
#include <wx/confbase.h>
#include <wx/fileconf.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <wx/display.h>
#include <wx/clipbrd.h>
#include <wx/image.h>
#include <wx/imaggif.h>
#include "wx/anidecod.h" // wxImageArray
#include "wx/wfstream.h" // wxFileOutputStream
#include "wx/gdicmn.h"   // wxNullPalette
#include "wx/quantize.h" // wxQuantize
#include <wx/aboutdlg.h>

#ifdef __linux__
// doesn't seem to be working
// #include "../icons/wxiv-icon-32.xpm"
#endif

using namespace std;
namespace fs = std::filesystem;

wxIMPLEMENT_APP(Wxiv::WxivApp);

namespace Wxiv
{
    const char* ConfigLastOpenPathKey = "LastOpenFilePath";

    bool WxivApp::OnInit()
    {
        // set a global file config (wxWidgets also may use this)
        wxFileConfig* config = new wxFileConfig("wxiv");
        wxConfigBase::Set(config);

        // on Windows, for setting app icon
        wxImage::AddHandler(new wxICOHandler());

        // on linux, need to add png handler for copy to clipboard, so do it here for simplicity
        wxImage::AddHandler(new wxPNGHandler());

        this->frame = new WxivMainFrame();
        this->frame->Show(true);

        return true;
    }

    WxivMainFrame::WxivMainFrame() : wxFrame(NULL, wxID_ANY, "wxiv", wxDefaultPosition, wxSize(1920, 1024))
    {
        setupIcons();
        buildMenus();
        buildMainLayout();
        CreateStatusBar();
        restoreWindow();
        restoreConfig();

        this->Bind(wxEVT_CLOSE_WINDOW, &WxivMainFrame::OnClose, this, wxID_ANY);

        // command-line arg for image/dir to open
        if (wxTheApp->argc > 1)
        {
            wxString s = wxTheApp->argv[1];
            wxFileName fn = wxFileName(s);

            if (wxDirExists(s)) // watch out fn.DirExists() is weird
            {
                this->loadDir(s);
            }
            else if (fn.FileExists())
            {
                this->loadImageAndDir(s);
            }
            else
            {
                alert(wxString("File or directory does not exist: ") + s);
                this->Close();
            }
        }
    }

    /**
     * Not too sure about this, but looks like this window icon apparently has to be set even though
     * the exe (on Windows) already has the icon.
     */
    void WxivMainFrame::setupIcons()
    {
#ifdef __linux__
// doesn't seem to be working
// this->SetIcon(wxIcon(wxiv_icon_32_xpm));
#elif _WIN32
        wxFileName path = findInstalledFile("wxiv-icon-32.ico");

        if (path.IsOk())
        {
            wxIconBundle icons(path.GetFullPath());

            if (icons.GetIconCount() > 0)
            {
                this->SetIcons(icons);
            }
        }
#endif
    }

    void WxivMainFrame::alert(const char* msg)
    {
        wxLogError(msg);
    }

    void WxivMainFrame::alert(const wxString& msg)
    {
        wxLogError(msg);
    }

    void WxivMainFrame::alert(const string& msg)
    {
        wxLogError(wxString(msg));
    }

    void WxivMainFrame::restoreWindow()
    {
        bool isMaximized = wxConfigBase::Get()->ReadBool("IsMaximized", false);

        wxString windowRectStr;
        wxConfigBase::Get()->Read("WindowRect", &windowRectStr);

        if (isMaximized)
        {
            Maximize(true);
        }
        else if (!windowRectStr.empty())
        {
            wxRect windowRect;
            vector<string> words = splitString(windowRectStr.ToStdString(), "_");

            if (words.size() == 4)
            {
                windowRect.x = std::stoi(words[0]);
                windowRect.y = std::stoi(words[1]);
                windowRect.width = std::stoi(words[2]);
                windowRect.height = std::stoi(words[3]);

                // don't restore a bad position
                wxDisplay display(wxDisplay::GetFromWindow(this));
                wxRect screen = display.GetClientArea();
                const int sizeGuard = 200;

                if ((windowRect.x >= 0) && (windowRect.y >= 0) && (windowRect.x < screen.GetRight() - sizeGuard) &&
                    (windowRect.y < screen.GetBottom() - sizeGuard) && !windowRect.IsEmpty() && (windowRect.width > sizeGuard) &&
                    (windowRect.height > sizeGuard))
                {
                    SetSize(windowRect);
                }
            }
        }
    }

    void WxivMainFrame::restoreConfig()
    {
        mainSplitWindow->restoreConfig();
        imageListPanel->restoreConfig();

        // now that config is restored can sync menu options to restored settings
        if (this->mainSplitWindow)
        {
            this->doRenderShapesMenuItem->Check(this->mainSplitWindow->getRenderShapes());
            this->doRenderPixelValuesMenuItem->Check(this->mainSplitWindow->getRenderPixelValues());
        }
    }

    void WxivMainFrame::OnClose(wxCloseEvent& evt)
    {
        wxConfigBase::Get()->Write("IsMaximized", IsMaximized());
        wxConfigBase::Get()->Write("ImageListWidth", mainSplitter->GetSashPosition());
        mainSplitWindow->saveConfig();
        imageListPanel->saveConfig();

        if (!IsMaximized() && !IsIconized())
        {
            wxRect r = wxRect(GetPosition(), GetSize());
            string windowRectStr = fmt::format("{}_{}_{}_{}", r.x, r.y, r.width, r.height);
            wxConfigBase::Get()->Write("WindowRect", wxString(windowRectStr));
        }

        // save this as last loaded image
        if (this->imageListPanel->getSelectedItemCount() == 1)
        {
            std::shared_ptr<WxivImage> image = this->imageListPanel->getSelectedImage();

            if (image && image->getPath().IsOk())
            {
                wxConfigBase::Get()->Write("LastOpenFilePath", image->getPath().GetFullPath());
            }
        }

        // allow close to continue
        evt.Skip();
    }

    void WxivMainFrame::buildHelpMenu(wxMenuBar* menuBar)
    {
        this->menuHelp = new wxMenu;

        menuHelp->Append(wxID_HELP);
        Bind(wxEVT_MENU, &WxivMainFrame::onHelp, this, wxID_HELP);

        menuHelp->Append(ID_ShowReleaseNotes, "&Release Notes...", "Release Notes");
        Bind(wxEVT_MENU, &WxivMainFrame::onReleaseNotes, this, ID_ShowReleaseNotes);

        menuHelp->Append(wxID_ABOUT);
        Bind(wxEVT_MENU, &WxivMainFrame::onAbout, this, wxID_ABOUT);

        menuBar->Append(menuHelp, "&Help");
    }

    void WxivMainFrame::buildFileMenu(wxMenuBar* menuBar)
    {
        // Open
        this->menuFile = new wxMenu;
        menuFile->Append(ID_OpenFile, "&Open File...", "Open file");
        menuFile->Append(ID_OpenDir, "Open &Dir...", "Open directory");

        wxString lastOpenPath;

        if (wxConfigBase::Get()->Read(ConfigLastOpenPathKey, &lastOpenPath))
        {
            menuFile->Append(ID_OpenLast, "Open &Last (" + lastOpenPath + ")\tCtrl-L", "Open Last");
        }

        // change image
        menuFile->AppendSeparator();
        menuItemsForAnyImagesListed.push_back(menuFile->Append(ID_NextImage, "&Next image...\tAlt-right", "Change to next image"));
        menuItemsForAnyImagesListed.push_back(menuFile->Append(ID_PreviousImage, "&Previous image...\tAlt-left", "Change to previous image"));

        // reload
        menuFile->AppendSeparator();
        menuItemsForAnyImagesListed.push_back(menuFile->Append(ID_ReloadDir, "&Reload Dir\tCtrl-R", "Reload directory"));

        // save
        menuFile->AppendSeparator();
        menuItemsForSingleImageSelected.push_back(menuFile->Append(ID_SaveFile, "Save &As...", "Save image"));
        menuItemsForSingleImageSelected.push_back(menuFile->Append(ID_SaveViewToFile, "Save &View As...", "Save image view to file"));
        menuItemsForSingleImageSelected.push_back(
            menuFile->Append(ID_CopyViewToClipboard, "Copy View to C&lipboard", "Copy image view to clipboard"));

        // save multiple
        menuFile->AppendSeparator();
        menuItemsForAnySelectedOrChecked.push_back(
            menuFile->Append(ID_SaveToGif, "Save Selected/Checked to GIF...", "Save one or more checked or selected images to animated GIF"));
        menuItemsForAnySelectedOrChecked.push_back(
            menuFile->Append(ID_SaveToCollage, "Save Selected/Checked to collage...", "Save one or more checked or selected images to collage"));

        // path and file name
        menuFile->AppendSeparator();
        menuItemsForSingleImageSelected.push_back(menuFile->Append(ID_ClipFileName, "Copy File Name", "Copy file name to clipboard"));
        menuItemsForSingleImageSelected.push_back(menuFile->Append(ID_ClipFilePath, "Copy File Path", "Copy file path to clipboard"));

        menuFile->AppendSeparator();
        menuFile->Append(wxID_EXIT, "E&xit", "Exit application");
        menuBar->Append(menuFile, "&File");

        Bind(wxEVT_MENU, &WxivMainFrame::onOpenFile, this, ID_OpenFile);
        Bind(wxEVT_MENU, &WxivMainFrame::onOpenDir, this, ID_OpenDir);
        Bind(wxEVT_MENU, &WxivMainFrame::onOpenLast, this, ID_OpenLast);
        Bind(wxEVT_MENU, &WxivMainFrame::onReloadDir, this, ID_ReloadDir);

        Bind(wxEVT_MENU, &WxivMainFrame::onNextImage, this, ID_NextImage);
        Bind(wxEVT_MENU, &WxivMainFrame::onPreviousImage, this, ID_PreviousImage);

        Bind(wxEVT_MENU, &WxivMainFrame::onSaveImage, this, ID_SaveFile);
        Bind(wxEVT_MENU, &WxivMainFrame::onSaveViewToFile, this, ID_SaveViewToFile);
        Bind(wxEVT_MENU, &WxivMainFrame::onSaveToGif, this, ID_SaveToGif);
        Bind(wxEVT_MENU, &WxivMainFrame::onSaveToCollage, this, ID_SaveToCollage);
        Bind(wxEVT_MENU, &WxivMainFrame::onCopyViewToClipboard, this, ID_CopyViewToClipboard);

        Bind(wxEVT_MENU, &WxivMainFrame::onCopyFileName, this, ID_ClipFileName);
        Bind(wxEVT_MENU, &WxivMainFrame::onCopyFilePath, this, ID_ClipFilePath);

        Bind(
            wxEVT_MENU, [&](wxCommandEvent&) { this->Close(true); }, wxID_EXIT);

        this->enableDisableMenuItems();
    }

    void WxivMainFrame::buildToolsMenu(wxMenuBar* menuBar)
    {
        this->menuTools = new wxMenu;

        menuTools->Append(ID_FitViewToImage, "&Fit view\tCtrl-Shift-F", "Fit view to image");
        Bind(wxEVT_MENU, &WxivMainFrame::onFitViewToImage, this, ID_FitViewToImage);

        menuBar->Append(menuTools, "&Tools");
    }

    void WxivMainFrame::buildOptionsMenu(wxMenuBar* menuBar)
    {
        this->menuOptions = new wxMenu;

        this->doRenderShapesMenuItem =
            menuOptions->Append(ID_ToggleShapeRender, "&Render shapes\tCtrl-G", "Turn on/off rendering of shapes", wxITEM_CHECK);
        Bind(wxEVT_MENU, &WxivMainFrame::onToggleShapeRender, this, ID_ToggleShapeRender);

        this->doRenderPixelValuesMenuItem = menuOptions->Append(ID_ToggleRenderPixelValues, "&Render pixel values at high zoom",
            "Turn on/off rendering of pixel values onto the render surface at high zoom", wxITEM_CHECK);
        Bind(wxEVT_MENU, &WxivMainFrame::onToggleRenderPixelValues, this, ID_ToggleRenderPixelValues);

        menuOptions->Append(ID_ShowBrightnessSettings, "&Intensity auto-range...", "Show intensity auto-range settings");
        Bind(wxEVT_MENU, &WxivMainFrame::onShowBrightnessSettings, this, ID_ShowBrightnessSettings);

        menuBar->Append(menuOptions, "&Options");
    }

    void WxivMainFrame::buildCaptureMenu(wxMenuBar* menuBar)
    {
        this->menuCapture = new wxMenu;

        menuCapture->Append(ID_AddViewToCaptureList, "&Add view to capture list\tCtrl-Shift-A", "Add view to capture list");
        Bind(wxEVT_MENU, &WxivMainFrame::onAddViewToCaptureList, this, ID_AddViewToCaptureList);

        this->menuItemClearCaptureList = menuCapture->Append(ID_ClearCaptureList, "&Clear capture list (0 items)", "Clear capture list");
        Bind(wxEVT_MENU, &WxivMainFrame::onClearCaptureList, this, ID_ClearCaptureList);

        menuCapture->Append(ID_SaveCaptureListToGif, "&Save captures to GIF", "Save capture list to GIF");
        Bind(wxEVT_MENU, &WxivMainFrame::onSaveCaptureListToGif, this, ID_SaveCaptureListToGif);

        menuCapture->Append(ID_SaveCaptureListToCollage, "Save captures to co&llage", "Save capture list to collage");
        Bind(wxEVT_MENU, &WxivMainFrame::onSaveCaptureListToCollage, this, ID_SaveCaptureListToCollage);

        menuBar->Append(menuCapture, "&Capture");
    }

    void WxivMainFrame::updateClearCaptureListMenuItem()
    {
        if (this->menuItemClearCaptureList)
        {
            size_t n = this->captureList.size();
            this->menuItemClearCaptureList->SetItemLabel(fmt::format("&Clear capture list ({} items)", n));
        }
    }

    void WxivMainFrame::buildMenus()
    {
        this->mainMenuBar = new wxMenuBar;

        buildFileMenu(mainMenuBar);
        buildCaptureMenu(mainMenuBar);
        buildToolsMenu(mainMenuBar);
        buildOptionsMenu(mainMenuBar);
        buildHelpMenu(mainMenuBar);

        SetMenuBar(mainMenuBar);
    }

    void WxivMainFrame::enableDisableMenuItems()
    {
        bool isAnyImagesListed = ((this->imageListPanel != nullptr) && (this->imageListPanel->getVisibleItemCount() > 0));

        for (wxMenuItem* p : menuItemsForAnyImagesListed)
        {
            p->Enable(isAnyImagesListed);
        }

        bool isSingleImageSelected = ((this->imageListPanel != nullptr) && (this->imageListPanel->getSelectedItemCount() == 1));

        for (wxMenuItem* p : menuItemsForSingleImageSelected)
        {
            p->Enable(isSingleImageSelected);
        }

        bool isAnySelectedOrChecked = ((this->imageListPanel != nullptr) && this->imageListPanel->checkAnySelectedOrCheckedImages());

        for (wxMenuItem* p : menuItemsForAnySelectedOrChecked)
        {
            p->Enable(isAnySelectedOrChecked);
        }
    }

    void WxivMainFrame::buildMainLayout()
    {
        mainSplitter = new wxSplitterWindow(this, wxID_ANY, wxPoint(0, 0), wxSize(300, 400), wxSP_BORDER | wxSP_LIVE_UPDATE);

        imageListPanel = new ImageListPanel(mainSplitter);
        imageListPanel->setOnSelectionChangeCallback([&](void) { this->onImageListSelectionChange(); });
        imageListPanel->setOnListItemsChangeCallback([&](void) { this->onImageListItemsChange(); });

        mainSplitWindow = new WxivMainSplitWindow(mainSplitter);
        mainSplitter->SplitVertically(imageListPanel, mainSplitWindow);

        // restore sash position
        int sashPosition = wxConfigBase::Get()->Read("ImageListWidth", 300);
        mainSplitter->SetSashPosition(sashPosition);
    }

    void WxivMainFrame::onAbout(wxCommandEvent& event)
    {
        wxAboutDialogInfo aboutInfo;
        aboutInfo.SetName("wxiv");
        aboutInfo.SetVersion(WxivVersion);
        aboutInfo.SetDescription(_("WxivImage Viewer built with wxWidgets, OpenCV, CvPlot, Apache Arrow/Parquet, and {fmt}."));
        aboutInfo.SetCopyright("(C) 2022");
        aboutInfo.SetWebSite("http://no-site-yet");
        aboutInfo.AddDeveloper("Ryan Seghers");
        wxAboutBox(aboutInfo);
    }

    void WxivMainFrame::showTextFile(wxString title, std::string fileName)
    {
        wxFileName filePath = findInstalledFile(fileName);

        if (filePath.IsOk())
        {
            wxSize size(800, 600);
            TextDisplayDialog dlg(this, title, size);
            dlg.loadFile(filePath);
            dlg.ShowModal();
        }
        else
        {
            // maybe there's just a different relative path between executable and help file in your dev environment
            alert(fmt::format("Could not find the wxiv text file {}.\n\nThere may be an installation problem.", fileName));
        }
    }

    void WxivMainFrame::onHelp(wxCommandEvent& event)
    {
        showTextFile("wxiv Help", "wxiv-help.txt");
    }

    void WxivMainFrame::onReleaseNotes(wxCommandEvent& event)
    {
        showTextFile("wxiv Release Notes", "wxiv-release-notes.txt");
    }

    void WxivMainFrame::createImageListSourceForDir(wxString dirPath)
    {
        // Use DICOM if more than half are DICOM files.
        vector<wxString> paths = listFilesInDir(dirPath);
        auto predicate = [=](const wxString& s) -> bool { return wxFileName(s).GetExt().Lower() == "dcm"; };
        vector<wxString> dcmPaths = vectorSelect<wxString>(paths, predicate);

        if (dcmPaths.size() > paths.size() / 2)
        {
#ifdef DO_DICOM
            this->imageListSource = std::make_shared<ImageListSourceDcmDirectory>();
#else
            alert("This was not compiled with DICOM support.");
#endif
        }
        else
        {
            this->imageListSource = std::make_shared<ImageListSourceDirectory>();
        }
    }

    void WxivMainFrame::loadDir(wxString dirPath)
    {
        createImageListSourceForDir(dirPath);

        if (this->imageListSource == nullptr)
            return;

        try
        {
            wxBusyCursor wait;
            this->imageListSource->load(dirPath);
        }
        catch (exception& ex)
        {
            wxString msg = "Exception loading directory:\n";
            msg += dirPath;
            msg += "\n\n";
            msg += ex.what();
            alert(msg);
        }

        imageListPanel->setSource(this->imageListSource);
        lastOpenDir = dirPath;
    }

    void WxivMainFrame::loadImage(wxString imagePath)
    {
        // looks like maybe a bug on linux where if we programmatically select a list item,
        // then user clicks another list item, it does not unselect the prior, so for now just
        // do not programmatically select.
        if (!(wxPlatformInfo::Get().GetOperatingSystemId() & wxOS_UNIX))
        {
            imageListPanel->selectImage(imagePath);
        }
    }

    /**
     * @brief Also loads the dir.
     * @param path
     */
    void WxivMainFrame::loadImageAndDir(wxString path)
    {
        wxString dirPath = wxFileName(path).GetPath();

        if (!dirPath.empty())
        {
            loadDir(dirPath);
            loadImage(path);
        }
    }

    void WxivMainFrame::saveImage(wxString path)
    {
        cv::Mat img = mainSplitWindow->getCurrentImage();

        // defer prompting user what to do for format mismatches and just auto-convert for now
        wxSaveImage(path, img);
    }

    void WxivMainFrame::onOpenFile(wxCommandEvent& event)
    {
        wxFileDialog openFileDialog(this, _("Open image file"), "", "",
            "WxivImage files|*.tif;*.tiff;*.png;*.jpeg;*.jpg"
            "|TIFF files (*.tif)|*.tif"
            "|JPEG files (*.jpeg)|*.jpeg"
            "|JPEG files (*.jpg)|*.jpg"
            "|PNG files (*.png)|*.png",
            wxFD_OPEN | wxFD_FILE_MUST_EXIST);

        if (openFileDialog.ShowModal() == wxID_CANCEL)
        {
            return;
        }

        wxString wxPath = openFileDialog.GetPath();
        loadImageAndDir(wxPath.ToStdString());
    }

    void WxivMainFrame::onOpenDir(wxCommandEvent& event)
    {
        wxDirDialog dlg(this, "Choose input directory", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);

        if (dlg.ShowModal() == wxID_CANCEL)
        {
            return;
        }

        wxString wxPath = dlg.GetPath();

        if (wxPath.empty())
        {
            return;
        }

        wxConfigBase::Get()->Write(ConfigLastOpenPathKey, wxPath);
        loadDir(wxPath);
    }

    void WxivMainFrame::onReloadDir(wxCommandEvent& event)
    {
        if (!lastOpenDir.empty())
        {
            // try to preserve image selection, if any
            wxString imagePath;
            auto image = imageListPanel->getSelectedImage();

            if (image)
            {
                imagePath = image->getPath().GetFullPath();
            }

            // lastOpenDir has current open dir
            loadDir(lastOpenDir);

            // restore selection
            if (!imagePath.empty())
            {
                loadImage(imagePath);
            }
        }
    }

    void WxivMainFrame::onOpenLast(wxCommandEvent& event)
    {
        wxString wpath;
        wxConfigBase::Get()->Read(ConfigLastOpenPathKey, &wpath);

        if (!wpath.empty())
        {
            wxFileName path(wpath);

            if (path.FileExists())
            {
                loadImageAndDir(wpath);
            }
            else if (wxDirExists(wpath))
            {
                if (path.HasExt())
                {
                    wxString ext = path.GetExt();

                    if (ImageUtil::checkSupportedExtension(toNativeString(ext)))
                    {
                        // it was an image, but apparently removed, and its parent dir still exists, so open parent dir
                        wpath = path.GetPath();
                    }
                }

                loadDir(wpath);
            }
            else
            {
                alert("The last opened file or directory does not exist.");
            }
        }
    }

    /**
     * @brief For example load dir or filter change.
     */
    void WxivMainFrame::onImageListItemsChange()
    {
        this->enableDisableMenuItems();
    }

    /**
     * @brief Callback from image panel when a single image is selected.
     * @param image
     */
    void WxivMainFrame::onImageListSelectionChange()
    {
        if (this->imageListPanel->getSelectedItemCount() == 1)
        {
            std::shared_ptr<WxivImage> image = this->imageListPanel->getSelectedImage();

            if (image && !image->getIsLoaded())
            {
                try
                {
                    // is not loaded yet
                    wxBusyCursor waitCursor;
                    int origIdx = this->imageListPanel->getSelectedImageDataIndex();

                    if (this->imageListSource->loadImage(image))
                    {
                        if (image->getPages().size() > 0)
                        {
                            // multiple pages
                            this->imageListSource->addImagePages(origIdx, image->getPages());
                            this->imageListPanel->updateForMultiPageImageLoaded();
                        }
                    }
                }
                catch (exception& ex)
                {
                    wxString msg = "Exception loading image:\n";
                    msg += image->getPath().GetFullPath();
                    msg += "\n\n";
                    msg += ex.what();
                    alert(msg);
                }
            }

            this->mainSplitWindow->setImage(image);

            if (image->checkIsShapeSetLoadError())
            {
                alert(image->getShapeSetLoadError());
            }
        }
        else
        {
            this->mainSplitWindow->clearImage();
        }

        this->enableDisableMenuItems();
    }

    void WxivMainFrame::onNextImage(wxCommandEvent& event)
    {
        // selection change event comes back to this object onImageListSelectionChange
        this->imageListPanel->selectNextImage(false);
    }

    void WxivMainFrame::onPreviousImage(wxCommandEvent& event)
    {
        // selection change event comes back to this object onImageListSelectionChange
        this->imageListPanel->selectNextImage(true);
    }

    void WxivMainFrame::onFitViewToImage(wxCommandEvent& event)
    {
        this->mainSplitWindow->setViewToFitImage();
    }

    void WxivMainFrame::onToggleShapeRender(wxCommandEvent& event)
    {
        if (this->menuOptions != nullptr)
        {
            bool doRender = this->doRenderShapesMenuItem->IsChecked();
            this->mainSplitWindow->setRenderShapes(doRender);
        }
    }

    void WxivMainFrame::onToggleRenderPixelValues(wxCommandEvent& event)
    {
        if (this->menuOptions != nullptr)
        {
            bool doRender = this->doRenderPixelValuesMenuItem->IsChecked();
            this->mainSplitWindow->setRenderPixelValues(doRender);
        }
    }

    void WxivMainFrame::onShowBrightnessSettings(wxCommandEvent& event)
    {
        this->mainSplitWindow->showBrightnessSettingsDialog();
    }

    void WxivMainFrame::onSaveImage(wxCommandEvent& event)
    {
        wxString path = showSaveImageDialog(this, "jpeg", "SaveImageDir", this->getDefaultSaveImageName());

        if (path.empty())
        {
            return;
        }

        saveImage(path);
    }

    void WxivMainFrame::onSaveViewToFile(wxCommandEvent& event)
    {
        wxString path = showSaveImageDialog(this, "jpeg", "SaveImageDir", this->getDefaultSaveImageName());

        if (path.empty())
        {
            return;
        }

        cv::Mat img = mainSplitWindow->getCurrentViewImageClone();

        if (!img.empty())
        {
            // should be rgb
            wxSaveImage(path, img);
        }
        else
        {
            alert("Current view is empty.");
        }
    }

    void WxivMainFrame::saveWxImagesToGif(vector<wxImage>& wxImages, const wxString& path)
    {
        // timing for GIF animation playback
        const int delayMs = 1000;

        if (!wxImages.empty())
        {
            try
            {
                if (saveToGif(wxImages, path, delayMs))
                {
                    showMessageDialog("Done creating GIF");
                }
                else
                {
                    alert("Save to GIF failed.");
                }
            }
            catch (std::runtime_error& ex)
            {
                alert(ex.what());
            }
        }
        else
        {
            alert("No images to save to GIF.");
        }
    }

    void WxivMainFrame::saveImagesToCollage(vector<cv::Mat>& images, vector<string>& captions, const wxString& path)
    {
        if (!images.empty())
        {
            try
            {
                ImageUtil::CollageSpec spec;
                loadCollageSpecFromConfig(spec);

                CollageSpecDialog dlg(this, spec);

                if (dlg.ShowModal() == wxID_OK)
                {
                    saveCollageSpecToConfig(spec);
                    cv::Mat collage;
                    ImageUtil::renderCollage(images, captions, spec, collage);
                    wxSaveImage(path, collage);
                    showMessageDialog("Done creating collage");
                }
            }
            catch (std::runtime_error& ex)
            {
                alert(ex.what());
            }
        }
        else
        {
            alert("No images to save to collage.");
        }
    }

    /**
     * @brief Save to gif using wxWidgets.
     * Point is to save the image as rendered, not just raw image, so have to use panel to render per view and settings.
     */
    void WxivMainFrame::saveWxivImagesToGif(vector<std::shared_ptr<WxivImage>>& checkedImages, const wxString& path)
    {
        if (!checkedImages.empty())
        {
            // build image array
            vector<wxImage> wxImages;

            for (std::shared_ptr<WxivImage> img : checkedImages)
            {
                // load and render
                if (this->imageListSource->loadImage(img))
                {
                    wxImages.push_back(mainSplitWindow->renderToWxImage(img));
                }
                else
                {
                    alert("Failed to load image:\n" + img->getPath().GetFullPath());
                }
            }

            saveWxImagesToGif(wxImages, path);
        }
        else
        {
            alert("You must check the checkboxes for images you want to include.");
        }
    }

    /**
     * @brief Save to collage using wxWidgets.
     * Point is to save the image as rendered, not just raw image, so have to use panel to render per view and settings.
     */
    void WxivMainFrame::saveWxivImagesToCollage(
        vector<std::shared_ptr<WxivImage>>& checkedImages, std::vector<std::string>& captions, const wxString& path)
    {
        if (!checkedImages.empty())
        {
            // build image array
            vector<cv::Mat> images;

            for (std::shared_ptr<WxivImage> img : checkedImages)
            {
                // load and render
                if (this->imageListSource->loadImage(img))
                {
                    images.push_back(mainSplitWindow->renderToImage(img));
                }
                else
                {
                    alert("Failed to load image:\n" + img->getPath().GetFullPath());
                }
            }

            saveImagesToCollage(images, captions, path);
        }
        else
        {
            alert("You must check the checkboxes for images you want to include.");
        }
    }

    wxString WxivMainFrame::getDefaultSaveImageName()
    {
        wxString name;

        auto image = this->imageListPanel->getSelectedImage();

        if (image)
        {
            wxFileName fn = image->getPath();

            if (fn.IsOk())
            {
                name = fn.GetName();
            }
        }

        return name;
    }

    /**
     * @brief Save checked images to GIF.
     */
    void WxivMainFrame::onSaveToGif(wxCommandEvent& event)
    {
        wxString path = showSaveImageDialog(this, "gif", "SaveImageDir", this->getDefaultSaveImageName());

        if (path.empty())
        {
            return;
        }

        // sequentially select images to load and render them
        vector<std::shared_ptr<WxivImage>> images = imageListPanel->getSelectedOrCheckedImages();

        if (!images.empty())
        {
            // this one for grayscale with vivid color overlays
            saveWxivImagesToGif(images, path);
        }
        else
        {
            alert("You must check the checkboxes for images you want to include.");
        }
    }

    /**
     * @brief Save checked images to collage.
     */
    void WxivMainFrame::onSaveToCollage(wxCommandEvent& event)
    {
        wxString path = showSaveImageDialog(this, "png", "SaveImageDir", "capture-collage");

        if (path.empty())
        {
            return;
        }

        // sequentially select images to load and render them
        vector<std::shared_ptr<WxivImage>> images = imageListPanel->getSelectedOrCheckedImages();

        if (!images.empty())
        {
            vector<string> captions;

            for (const auto& img : images)
            {
                captions.push_back(toNativeString(img->getDisplayName()));
            }

            saveWxivImagesToCollage(images, captions, path);
        }
        else
        {
            alert("You must check the checkboxes for images you want to include.");
        }
    }

    void WxivMainFrame::onCopyViewToClipboard(wxCommandEvent& event)
    {
        wxBitmap bmp = mainSplitWindow->getCurrentViewImageBitmap();

        if (bmp.IsOk())
        {
            // the clipboard takes ownership of this obj and docs say do not delete it
            wxBitmapDataObject* obj = new wxBitmapDataObject(bmp);

            if (obj != nullptr)
            {
                if (wxTheClipboard->Open())
                {
                    wxTheClipboard->SetData(obj);
                    wxTheClipboard->Close();
                }
                else
                {
                    alert("Unable to open clipboard.");
                }
            }
            else
            {
                alert("Unable to convert image to bitmap data object.");
            }
        }
        else
        {
            alert("Current view bitmap is empty.");
        }
    }

    void WxivMainFrame::onCopyFileName(wxCommandEvent& event)
    {
        if (this->imageListPanel->getSelectedItemCount() == 1)
        {
            std::shared_ptr<WxivImage> image = this->imageListPanel->getSelectedImage();
            copyImageNameOrPathToClipboard(image, true);
        }
        else
        {
            alert("Current view bitmap is empty.");
        }
    }

    void WxivMainFrame::onCopyFilePath(wxCommandEvent& event)
    {
        if (this->imageListPanel->getSelectedItemCount() == 1)
        {
            std::shared_ptr<WxivImage> image = this->imageListPanel->getSelectedImage();
            copyImageNameOrPathToClipboard(image, false);
        }
        else
        {
            alert("Current view bitmap is empty.");
        }
    }

    void WxivMainFrame::onAddViewToCaptureList(wxCommandEvent& event)
    {
        wxImage img = mainSplitWindow->getViewWxImageClone();

        if (img.IsOk())
        {
            captureList.push_back(img);
            captureListMat.push_back(mainSplitWindow->getCurrentViewImageClone());
            captureListCaptions.push_back(toNativeString(imageListPanel->getSelectedImage()->getDisplayName()));
            updateClearCaptureListMenuItem();

            this->SetStatusText(fmt::format("Capture {} added", captureListMat.size()));
        }
        else
        {
            alert("The current view image is not okay so cannot add it to the capture list.");
        }
    }

    void WxivMainFrame::onClearCaptureList(wxCommandEvent& event)
    {
        captureList.clear();
        captureListMat.clear();
        captureListCaptions.clear();
        updateClearCaptureListMenuItem();
        this->SetStatusText("");
    }

    void WxivMainFrame::onSaveCaptureListToGif(wxCommandEvent& event)
    {
        if (!captureList.empty())
        {
            wxString path = showSaveImageDialog(this, "gif", "SaveImageDir", "capture");

            if (!path.empty())
            {
                saveWxImagesToGif(captureList, path);
            }
        }
        else
        {
            alert("There are no images in the capture list.");
        }
    }

    void WxivMainFrame::onSaveCaptureListToCollage(wxCommandEvent& event)
    {
        if (!captureListMat.empty())
        {
            wxString path = showSaveImageDialog(this, "png", "SaveImageDir", "capture-collage");

            if (!path.empty())
            {
                saveImagesToCollage(captureListMat, captureListCaptions, path);
            }
        }
        else
        {
            alert("There are no images in the capture list.");
        }
    }
}
