cmake_minimum_required (VERSION 3.21)
project(WxivLib LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../CMakeCommon.txt)

# cvplot from my local source because have made changes
include(FetchContent)
set (CMAKE_POLICY_DEFAULT_CMP0077 NEW) # for cv-plot warnings
set (CVPLOT_WITH_TESTS OFF)
set (CVPLOT_WITH_EXAMPLES OFF)
set (CVPLOT_HEADER_ONLY OFF)
set (CVPLOT_INSTALL OFF)

include(FetchContent)

FetchContent_Declare(
	CvPlot
	GIT_REPOSITORY "https://github.com/ryanseghers/cv-plot.git"
	GIT_TAG        f18dac61  # bar plot branch
	SOURCE_SUBDIR ./CvPlot
)

FetchContent_MakeAvailable(CvPlot)

find_package(Arrow CONFIG REQUIRED)
find_package(Parquet CONFIG REQUIRED)
find_package(wxWidgets CONFIG REQUIRED)
set (wxUSE_STL ON)

if(DO_DICOM)
    find_package(DCMTK CONFIG REQUIRED)
endif()

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/../icons/appicon.rc")

set(SOURCE_FILES
	Dialog/CollageSpecDialog.h
	Dialog/CollageSpecDialog.cpp

	Image/Polygon.h
	Image/Polygon.cpp
	Image/ShapeSet.h
	Image/ShapeSet.cpp
	Image/WxivImage.h
	Image/WxivImage.cpp
	Image/WxivImageUtil.h
	Image/WxivImageUtil.cpp

	ImageList/ImageListPanel.h
	ImageList/ImageListPanel.cpp
	ImageList/ImageListSource.h
	ImageList/ImageListSource.cpp
	ImageList/ImageListSourceDirectory.h
	ImageList/ImageListSourceDirectory.cpp
	ImageList/ImageListSourceDcmDirectory.h
	ImageList/ImageListSourceDcmDirectory.cpp

	ImageView/ImageScrollPanel.h
	ImageView/ImageScrollPanel.cpp
	ImageView/ImageViewPanel.h
	ImageView/ImageViewPanel.cpp
	ImageView/ImageViewPanelSettings.h
	ImageView/ImageViewPanelSettings.cpp
	ImageView/ImageViewPanelSettingsPanel.h
	ImageView/ImageViewPanelSettingsPanel.cpp
	ImageView/IntensityRangeParams.h
	ImageView/IntensityRangeParams.cpp
	ImageView/ManualScrollPanel.h
	ImageView/ManualScrollPanel.cpp

	Panel/HistChartPanel.h
	Panel/HistChartPanel.cpp
	Panel/PixelStatsPanel.h
	Panel/PixelStatsPanel.cpp
	Panel/ProfilesPanel.h
	Panel/ProfilesPanel.cpp
	Panel/ShapeMetadataHistPanel.h
	Panel/ShapeMetadataHistPanel.cpp
	Panel/ShapeMetadataPanel.h
	Panel/ShapeMetadataPanel.cpp

	ArrowUtil/ArrowFilterExpression.h
	ArrowUtil/ArrowFilterExpression.cpp
	ArrowUtil/ArrowUtil.h
	ArrowUtil/ArrowUtil.cpp

	OpenCVUtil/CollageSpec.h
	OpenCVUtil/FloatHist.h
	OpenCVUtil/FloatHist.cpp
	OpenCVUtil/ImageUtil.h
	OpenCVUtil/ImageUtil.cpp

	BaseUtil/MathUtil.h
	BaseUtil/MathUtil.cpp
	BaseUtil/MiscUtil.h
	BaseUtil/MiscUtil.cpp
	BaseUtil/StringUtil.h
	BaseUtil/StringUtil.cpp
	BaseUtil/VectorUtil.h
	BaseUtil/VectorUtil.cpp

	ThirdParty/debugbreak.h

	WxWidgetsUtil/TextDisplayDialog.h
	WxWidgetsUtil/TextDisplayDialog.cpp
	WxWidgetsUtil/WxivUtil.h
	WxWidgetsUtil/WxivUtil.cpp
	WxWidgetsUtil/WxWidgetsUtil.h
	WxWidgetsUtil/WxWidgetsUtil.cpp

	WxivMainSplitWindow.h
	WxivMainSplitWindow.cpp
)

if(DO_DICOM)
    list(APPEND SOURCE_FILES
        Dicom/Contour.h
        Dicom/DicomUtil.h
        Dicom/DicomUtil.cpp
    )
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

add_library(WxivLib STATIC ${SOURCE_FILES})

if(DO_DICOM)
    if ("${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}" STREQUAL "/")
        message(WARNING "No vcpkg install dir found, DCMTK definitions are not set")
    else()
        target_compile_definitions(WxivLib
            PUBLIC
                "DCMTK_PREFIX=${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}"
                "DCM_DICT_DEFAULT_PATH=${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/dcmtk/dicom.dic:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/dcmtk/private.dic"
                "DEFAULT_CONFIGURATION_DIR=${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/etc/dcmtk/"
                "DEFAULT_SUPPORT_DATA_DIR=${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/dcmtk/"
        )
    endif()
endif()

target_include_directories(WxivLib PUBLIC "." "./Dialog" "./Image" "./ImageList" "./ImageView" "./Panel" "./Util"
	"./WxWidgetsUtil" "./ArrowUtil" "./BaseUtil" "./ThirdParty" "./OpenCVUtil" "./Dicom" ${debugbreak_SOURCE_DIR})

target_link_libraries(WxivLib PUBLIC wx::core wx::base ${OpenCV_LIBS} fmt::fmt CvPlot::CvPlot)

if(DO_DICOM)
    target_link_libraries(WxivLib PUBLIC DCMTK::DCMTK)
    #add_compile_definitions(DO_DICOM)
endif()

target_link_libraries(WxivLib PUBLIC "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>")
target_link_libraries(WxivLib PUBLIC "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Parquet::parquet_static,Parquet::parquet_shared>")

IF (WIN32)
    # Do this or else msvc doesn't have correct __cplusplus value for DCMTK
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
ENDIF()
