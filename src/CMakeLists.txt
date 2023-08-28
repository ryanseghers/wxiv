# wxiv executable
cmake_minimum_required (VERSION 3.21)
project(wxiv LANGUAGES CXX)
set(APP_NAME wxiv)

option(DO_DICOM "Whether or not to include DICOM directory parse capability." ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/../CMakeCommon.txt)

if (NOT TARGET WxivLib)
    add_subdirectory("./WxivLib" WxivLib)
endif()

find_package(wxWidgets CONFIG REQUIRED)
set (wxUSE_STL ON)

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/../icons/appicon.rc")

set(SOURCE_FILES
	WxivMainFrame.cpp
	WxivMainFrame.h
)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

IF (WIN32)
    add_executable(${APP_NAME} WIN32 ${SOURCE_FILES} ${APP_ICON_RESOURCE_WINDOWS})

	# runtime dependency resolution picks up some things that cannot be found and installed
	# and fmt.dll is inferred but not a real thing, for some reasons
	# without DESTINATION . the dependencies are installed in subdir ./bin, but with that cannot seem to get start menu shortcut not to still point to bin subdir
	install(RUNTIME_DEPENDENCY_SET wxiv_dependencies PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" POST_EXCLUDE_REGEXES ".*system32/.*\\.dll")

	# specifying CONFIGURATIONS Release here somehow prevents wxiv.exe from being installed
	# without DESTINATION . the dependencies are installed in subdir ./bin
	install(TARGETS ${APP_NAME} RUNTIME_DEPENDENCY_SET wxiv_dependencies RUNTIME)
ELSEIF(APPLE)
    # icon
    set(MACOSX_BUNDLE_ICON_FILE wxiv)
    set(ICON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../icons/wxiv.icns)
    set_source_files_properties(${ICON_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    # exe
    add_executable(${APP_NAME} MACOSX_BUNDLE ${ICON_PATH} ${SOURCE_FILES})

    install(TARGETS ${APP_NAME}
        BUNDLE DESTINATION . COMPONENT Runtime
        RUNTIME DESTINATION bin COMPONENT Runtime
    )

    set(APP_DIR "\${CMAKE_INSTALL_PREFIX}/${APP_NAME}.app")
    set(DIRS ${CMAKE_BINARY_DIR})

    # doc
    target_sources(${APP_NAME} PUBLIC ../doc/wxiv-help.txt ../doc/wxiv-release-notes.txt)
    SET_SOURCE_FILES_PROPERTIES(../doc/wxiv-help.txt ../doc/wxiv-release-notes.txt PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    install(CODE "include(BundleUtilities)
        fixup_bundle(\"${APP_DIR}\" \"\" \"${DIRS}\")")

ELSEIF(UNIX)
    # would also happen for mac exept for above explicit check for APPLE
    add_executable(${APP_NAME} ${SOURCE_FILES})
	install(TARGETS ${APP_NAME} RUNTIME_DEPENDENCY_SET wxiv_dependencies RUNTIME)
ENDIF()

target_link_libraries(${APP_NAME} PRIVATE wx::core wx::base WxivLib)

if(DO_DICOM)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDO_DICOM")
endif()
