# wxiv tests
cmake_minimum_required (VERSION 3.16)
project (WxivLibTest)

include(${CMAKE_CURRENT_SOURCE_DIR}/../CMakeCommon.txt)

if (NOT TARGET WxivLib)
    add_subdirectory("../src/WxivLib" WxivLib)
endif()

find_package(GTest CONFIG REQUIRED)

set(SOURCE_FILES
	main.cpp
	TempFile.h

	ArrowUtilTests/ArrowUtilTests.cpp
	ArrowUtilTests/FilterSpecTests.cpp
	BaseUtilTests/StringUtilTests.cpp
	OpenCVUtilTests/ImageUtilTests.cpp
	ImageTests/WxivImageTests.cpp
	WxWidgetsUtilTests/WxivUtilTests.cpp
	WxWidgetsUtilTests/WxWidgetsUtilTests.cpp
	)

# Add source to this project's executable.
add_executable(wxivtest ${SOURCE_FILES} )

target_include_directories(wxivtest PUBLIC ".")

target_link_libraries(wxivtest PRIVATE WxivLib GTest::gtest_main)

#
# CTest on top of googletest, with test discovery
# (took trial and error between here and the tests CMakeLists.txt to get VS to discover the tests)
#
include(GoogleTest)
enable_testing()
gtest_discover_tests(wxivtest)
