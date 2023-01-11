Purpose
-----------
This is for viewing images to support a particular image processing development workflow.
This is designed to open a directory with several images in it and be able to step forward and backward through those images, and maintain zoom/view from image to image.
The workflow this supports is in your image processing code you save debug images after each step, then this GUI lets you examine those.
It is likely that each project that used this would fork it and then add domain-specific capabilities.

This is not for processing, editing, or modifying images.

I don't think it could ever make sense to try to develop image processing capabilities within this app, people should just use ImageJ or something else instead.
However, view filters (where some image processing operation is done just to view the image) are useful sometimes.


Primary Features
-----------
- Directory-oriented interface to look at sets of images.
- Maintain pan and zoom when stepping through images.
- Render shapes on top of the image from neighbor .geo.csv or .parquet neighbor file, including colors, line thickness, etc.
- Show, filter, and histogram metadata associated with each shape from the .geo.csv or .parquet neighbor file.
- Handles multi-page TIF images (though see limitation about non-ASCII paths).


Design Notes
-----------
Async/MT implementations are nicer and more usable. However there is significant cost in code complexity. I've decided that in this case the code simplicity is more valuable and have stayed away from async/MT implementation.
One consideration is that this intended for image processing developers who are likely to be aware when they are loading a large file, or a file over a slow network connection, and to be understanding of the non-async behavior.


Limitations
-----------
Image paths with non-ASCII characters are handled differently than paths without them. In the non-ASCII case multi-page TIFs are not handled.


Versioning
-----------
This has a bumpversion config file so you can use bumpversion to increment version. It just replaces the prior version string with new version string by text replace in the file(s) specified in the config file:
    pip install bumpversion
    bumpversion patch


Dependencies/Credits
-----------
WxWidgets (https://github.com/wxWidgets/wxWidgets)
OpenCV (https://github.com/opencv/opencv)
Apache Arrow/Parquet (https://arrow.apache.org/)
fmt (https://github.com/fmtlib/fmt)
cv-plot (https://github.com/Profactor/cv-plot)
debugbreak (https://github.com/scottt/debugbreak)


Development
-----------
Non-ASCII String Support
    wxWidgets has good non-ASCII string support. However, things are not perfectly smooth between platforms because on linux non-ASCII characters are handled as 8-bit utf-8 encoded, not wstring. So you cannot use wxString.ToStdWstring() on a string that has non-ascii characters.
    So on Windows wstring works fine, but not on Linux. Thus the strategy I've chosen is to stay with wxWidgets classes (wxString, wxFileName, wxDir, etc) as much as possible.
    
    The filesystem handling in Apache Arrow handles utf-8 std::string on both Windows and Linux, e.g. std::string(s.mb_str(wxConvUTF8)) works on both platforms.


Control hierarchy
-----------
WxivMainFrame: top frame is split vertically and has:
    left panel: ImageListPanel
        filter text box
        listview with image list
    right panel: WxivMainSplitWindow
        WxivMainSplitWindow is split vertically and
            left panel: Notebook with Stats, Shapes etc. tabs
            right panel: ImageScrollPanel
                ImageScrollPanel has scrollbars, a top toolbar, settings button, and an ImageViewPanel
                    ImageViewPanelSettingsPanel is a panel shown on a dialog to edit settings when per Settings button
                    ImageViewPanel stores and paints a roi of an image


Dev Machine Setup
------------------
The build needs to be able to git clone from github without prompting in order to build in VS.
So git needs to be installed.
And things need to be set up so that VS/git can access github repos without prompting:
    See: https://interworks.com/blog/2021/09/15/setting-up-ssh-agent-in-windows-for-passwordless-git-authentication/
    - ssh keys that work for github need to be in user home dir .ssh subdir.
    - VS won't prompt for password so some kind of credential management needs to be available.
    - git for windows installs it but it doesn't work for the primary case, ssh keys, without ssh agent started.
    - to do that, run elevated powershell:
        Get-Service ssh-agent | Set-Service -StartupType Automatic -PassThru | Start-Service
        and then reboot
    - then add the ssh key
        ssh-add <path to private key>
    - to test the ssh key in ssh-agent
        ssh -T git@github.com
        should work without prompting
    - configure git to work with ssh:
        git config --global core.sshCommand C:/Windows/System32/OpenSSH/ssh.exe


    Windows
    ------------------------
    - Since I have two clones of vcpkg I needed to differentiate, so I am using a new env var %WXIV_VCPKG_ROOT%
        It is referenced in the CMakeSettings.json file.
    - See top CMakeLists.txt comments about NSIS install in order to get setup to build installer, which didn't work for me with default NSIS installation.


    Linux
    ------------------------
    - sudo ./scripts/setup-build-machine.sh
    - set VCPKG_ROOT env var to point to a new dir or to an existing vcpkg repo clone
    - run build-all.sh


Future Features
------------------------
Here are some possible future features in approximately the order I might work on them:
- Add quads to list of supported shapes in neighbor shape metadata files (use dim1, dim2 as the second point, then four more columns for p3x,p3y and p4x,p4y).
    - Or a json format instead, which seems better vs adding overloaded columns to the table format.
- Add simple directory browsing (probably just show dirs in list panel, double-click to nav to them, and an Up button to go up a dir).
- Better support for the myriad file formats. Could at least try to deal with the known universe of image extensions and handle failures well.
