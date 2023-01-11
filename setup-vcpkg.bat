echo OFF

:: cd to dir containing this script
cd %~dp0

%WXIV_VCPKG_ROOT%\vcpkg.exe update
%WXIV_VCPKG_ROOT%\vcpkg.exe remove --outdated --recurse
%WXIV_VCPKG_ROOT%\vcpkg.exe install --recurse fmt opencv4[core,jpeg,png,tiff] wxwidgets arrow[core,csv,filesystem,json,parquet] gtest
