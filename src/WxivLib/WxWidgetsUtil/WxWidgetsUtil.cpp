// Copyright(c) 2022 Ryan Seghers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
#include <wx/wxprec.h>
#include <wx/confbase.h>
#include <wx/filename.h>
#include <wx/imaggif.h>
#include "wx/anidecod.h" // wxImageArray
#include "wx/quantize.h" // wxQuantize
#include "wx/wfstream.h" // wxFileOutputStream

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "WxWidgetsUtil.h"
#include "MiscUtil.h"
#include "StringUtil.h"

using namespace std;

namespace Wxiv
{
    void showAlertDialog(const char* msg)
    {
        wxLogMessage(msg);
    }

    void showAlertDialog(std::string msg)
    {
        wxLogMessage(msg.c_str());
    }

    void showMessageDialog(const char* msg)
    {
        wxMessageDialog dlg(nullptr, msg, "Notification", wxOK);
        dlg.ShowModal();
    }

    void showMessageDialog(string msg)
    {
        wxMessageDialog dlg(nullptr, msg, "Notification", wxOK);
        dlg.ShowModal();
    }

    void showHtmlDialog(std::string msg)
    {
        // MyAskDialog dlg(...);
        // if (dlg.ShowModal() == wxID_OK)
        // wxSize size(800, 600);
        // wxHtmlWindow w(this, wxID_ANY, wxDefaultPosition, size);
        // w.show

        // dlg.ShowModal();
    }

    /**
     * @brief Show save image file dialog.
     * @param configDirSaveKey Optional. If not empty then get/save the dir from config, using this key, to be the default dir for the dialog.
     * @param defaultFileName Without path, with extension.
     * @return The path selected or empty string for cancel.
     */
    wxString showSaveImageDialog(wxWindow* parent, const std::string& defaultExt, const wxString& configDirSaveKey, const wxString& defaultFileName)
    {
        // get default dir from config, empty is fine
        wxString defaultDir;
        wxConfigBase::Get()->Read(configDirSaveKey, &defaultDir, wxEmptyString);

        // TODO: support other formats, get them from ImageUtil
        vector<string> allFilterStrings;
        allFilterStrings.push_back("PNG files (*.png)|*.png");
        allFilterStrings.push_back("JPEG files (*.jpeg)|*.jpeg");
        allFilterStrings.push_back("JPEG files (*.jpg)|*.jpg");
        allFilterStrings.push_back("TIFF files (*.tif, *.tiff)|*.tif;*.tiff");
        allFilterStrings.push_back("BMP files (*.bmp)|*.bmp");

        // gif save is handled specially so by default do not show it as an option
        if (defaultExt == "gif")
        {
            allFilterStrings.push_back("GIF files (*.gif)|*.gif");
        }

        // put the default extension first in the list
        vector<string> orderedFilterStrings;

        for (const string& s : allFilterStrings)
        {
            if (checkStringContains(s, defaultExt))
            {
                orderedFilterStrings.insert(orderedFilterStrings.begin(), s);
            }
            else
            {
                orderedFilterStrings.push_back(s);
            }
        }

        // dialog
        string filterStr = stringJoin(orderedFilterStrings, "|");

        wxFileDialog saveFileDialog(parent, _("Save image file"), defaultDir, defaultFileName, filterStr.c_str(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveFileDialog.ShowModal() == wxID_CANCEL)
        {
            return wxString("");
        }

        wxString path = saveFileDialog.GetPath();

        // save dir for later
        wxFileName savedPath(path);
        wxString dir = savedPath.GetPath();
        wxConfigBase::Get()->Write(configDirSaveKey, dir);

        return path;
    }

    bool writeFile(const wxString& path, const vector<unsigned char>& buffer)
    {
        bool result = false;
        wxFile file(path, wxFile::write);

        if (file.IsOpened())
        {
            if (file.Write(buffer.data(), buffer.size()) == buffer.size())
            {
                result = true;
            }

            file.Close();
        }

        return result;
    }

    /**
    * @brief Save to gif using wxWidgets.
    * @return True for success, false for fail. This also throws for certain errors.
    */
    bool saveToGif(vector<wxImage>& images, const wxString& path, int delayMs)
    {
        // build image array
        bool worked = false;

        {
            wxImageArray imageArray;
            wxBusyCursor waitCursor;

            for (wxImage& wximg : images)
            {
                // have to quantize to <= 256 color
                // (the Quantize default argument value is 236 and I don't know why, but good odds they know better than I do)
                wxImage quantizedWximg;
                wxQuantize::Quantize(wximg, quantizedWximg);

                if (!quantizedWximg.HasPalette())
                {
                    throw std::runtime_error("Failed to quantize image and images without palettes cannot be saved as GIF.");
                }

                if (!quantizedWximg.IsOk())
                {
                    throw std::runtime_error("Quantized wxImage is not okay.");
                }

                imageArray.Add(quantizedWximg);
            }

            // save
            wxGIFHandler handler;
            wxFileOutputStream stream(path);

            worked = handler.SaveAnimation(imageArray, &stream, false, delayMs);
            stream.Close();
        }

        return worked;
    }
}
