# top wxiv project
cmake_minimum_required (VERSION 3.16)
project(wxiv LANGUAGES CXX VERSION 0.1.0)

include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeCommon.txt)

IF (WIN32)
    # For VS instrumentation profiler
    # Enable these lines, then do a clean build, then run analyzer from exe.
    #add_link_options("/PROFILE")
    #set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
endif()

find_package(fmt CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED)

if (NOT TARGET wxiv)
    add_subdirectory("src" wxiv)
endif()

if (NOT TARGET wxivTests)
    add_subdirectory("tests" wxivTests)
endif()

IF (WIN32)
    # you must install language files with your NSIS installation (from sourceforge) and for me it was not checked by default
    # language files go here: C:/Program Files (x86)/NSIS/Contrib/Language files
    # if not there, the nsis run fails because cpack generates this file _CPack_Packages/win64/NSIS/project.nsi to require Albanian and a bunch of other languages
    #
    # After successful build, go to out\build\x64-Release and run:
    #    cpack
    # It builds installer to here: out/build/x64-Release/wxiv-0.1.0-win64.exe
    set(CPACK_GENERATOR NSIS)

    # this specifies for installer to prompt whether to modify path or not, wxit probably wouldn't often be run from command line
    #set(CPACK_NSIS_MODIFY_PATH "ON")

    # this seems to already be the default
	#set(CPACK_NSIS_PACKAGE_NAME wxiv)

    # if this isn't set then the default is project name and version
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "wxiv")

    # this apparently controls setting the start menu shortcut
	set(CPACK_PACKAGE_EXECUTABLES "wxiv" "wxiv")

    install(FILES doc/wxiv-help.txt doc/wxiv-release-notes.txt icons/wxiv-icon.ico DESTINATION share)

ELSEIF (APPLE)
    set(CPACK_GENERATOR DragNDrop)

    # the files are handled in the CMakeLists.txt that defines the bundle
ELSEIF (UNIX)
    # would also happen for mac exept for above explicit check for APPLE

    # By default this installs to /usr/wxiv/bin
    set(CPACK_GENERATOR DEB)

    # without gsettings-desktop-schemas I get this on any wxwidgets dialog ctor: no GSettings schemas are installed on the machine
    # without libgtk-3-common I get this on file open dialog (at least): Settings schema 'org.gtk.Settings.FileChooser' is not installed
    # unable to fix this: Failed to load module "canberra-gtk-module" by adding libcanberra-gtk(3)-module
    # unable to fix this: Fontconfig error: Cannot load default config file: (null)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgtk-3-common, gsettings-desktop-schemas")

    install(FILES doc/wxiv-help.txt doc/wxiv-release-notes.txt icons/wxiv-icon.ico DESTINATION share)

ENDIF()

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "wxiv developers")
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "wxiv developers")
set(CPACK_PACKAGE_CONTACT "wxiv developers")

include(CPack)

#
# CTest on top of googletest, with test discovery
# (took trial and error between here and the tests CMakeLists.txt to get VS to discover the tests)
#
include(GoogleTest)
enable_testing()
