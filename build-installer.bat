:: After building Release config in visual studio.

:: cd to dir containing this script
cd %~dp0

cd out\build\x64-Release
cpack 
