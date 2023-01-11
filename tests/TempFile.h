#pragma once

#include <string>

#include "WxWidgetsUtil.h"
#include <wx/stdpaths.h>

namespace WxivTests
{
    /**
     * @brief Provides a temp file, with specified extension, and deletes the file when obj is deleted.
     */
    class TempFile
    {
      private:
        wxFileName fileName;

        TempFile(){};

      public:
        TempFile(const wxString& prefix, const std::string& ext)
        {
            fileName = wxFileName::CreateTempFileName(prefix);
            fileName.SetExt(ext);
        }

        ~TempFile()
        {
            if (fileName.FileExists())
            {
                wxRemoveFile(fileName.GetFullPath());
            }
        }

        wxString GetFullPath()
        {
            return fileName.GetFullPath();
        }
    };
}
