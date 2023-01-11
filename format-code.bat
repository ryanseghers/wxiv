:: Run clang-format on the code.

:: src
clang-format.exe -i src/*.cpp src/*.h

for /D %%a in (src/WxivLib/*) do clang-format.exe -i src/WxivLib/%%a/*.cpp src/WxivLib/%%a/*.h

:: tests
clang-format.exe -i tests/*.cpp tests/*.h

for /D %%a in (tests/*) do clang-format.exe -i tests/%%a/*.cpp
