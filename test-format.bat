:: Run clang-format on the code to test if anything would be formatted.

:: src
clang-format.exe -i src/*.cpp src/*.h

for /D %%a in (src/WxivLib/*) do clang-format.exe --Werror --dry-run --ferror-limit 1 src/WxivLib/%%a/*.cpp src/WxivLib/%%a/*.h

:: tests
clang-format.exe -i tests/*.cpp tests/*.h

for /D %%a in (tests/*) do clang-format.exe --Werror --dry-run --ferror-limit 1 tests/%%a/*.cpp
