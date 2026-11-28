cmake_minimum_required(VERSION 3.16)

project(PhotoSnap VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Network Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Network Test)
include(CTest)

set(PHOTOSNAP_PLATFORM_ICON)
if(WIN32)
    enable_language(RC)
    list(APPEND PHOTOSNAP_PLATFORM_ICON resources/photosnap.rc)
elseif(APPLE)
    set_source_files_properties(resources/photosnap.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    list(APPEND PHOTOSNAP_PLATFORM_ICON resources/photosnap.icns)
endif()

add_executable(photo-snap WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/language.cpp
    src/language.h
    resources/photosnap.qrc
    ${PHOTOSNAP_PLATFORM_ICON}
    src/mainwindow.cpp
    src/mainwindow.h
    src/photoview.cpp
    src/photoview.h
    src/imagecache.cpp
    src/imagecache.h
    src/imagemetadata.cpp
    src/imagemetadata.h
    src/cachebrowserwindow.cpp
    src/cachebrowserwindow.h
    src/imagelibrarywidget.cpp
    src/imagelibrarywidget.h
    src/selectedimages.h
    src/core/ImageAdjustments.h
    src/core/ImageDocument.cpp
    src/core/ImageDocument.h
    src/core/PreviewRenderer.cpp
    src/core/PreviewRenderer.h
    src/core/ImageTransform.cpp
    src/core/ImageTransform.h
    src/ui/CropOverlay.cpp
    src/ui/CropOverlay.h
    src/core/ImageFilters.cpp
    src/core/ImageFilters.h
    src/ui/AdjustmentPanel.cpp
    src/ui/AdjustmentPanel.h
    src/commands/AdjustmentCommand.cpp
    src/commands/AdjustmentCommand.h
)

if(APPLE)
    set_target_properties(photo-snap PROPERTIES MACOSX_BUNDLE_ICON_FILE photosnap.icns)
endif()

target_link_libraries(photo-snap PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Sql Qt${QT_VERSION_MAJOR}::Network)

if(QT_VERSION_MAJOR EQUAL 6 AND TARGET Qt6::QSQLiteDriverPlugin)
    qt_import_plugins(photo-snap INCLUDE Qt6::QSQLiteDriverPlugin)
endif()

if(MSVC)
    target_compile_options(photo-snap PRIVATE /W4 /permissive-)
else()
    target_compile_options(photo-snap PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(GNUInstallDirs)
set(CMAKE_INSTALL_BINDIR "bin")

install(TARGETS photo-snap
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
    install(FILES resources/photosnap.png
        DESTINATION share/icons/hicolor/512x512/apps
        RENAME photosnap.png
    )
    install(FILES resources/photo-snap.desktop DESTINATION share/applications)
endif()

# Qt 6.3 and newer can generate a platform-aware deployment script that
# copies Qt libraries, plugins, and runtime dependencies beside the app.
if(QT_VERSION_MAJOR EQUAL 6 AND COMMAND qt_generate_deploy_app_script)
    set(PHOTOSNAP_QT_DEPLOY_OPTIONS)
    if(UNIX AND NOT APPLE AND Qt6Core_VERSION VERSION_GREATER_EQUAL "6.10")
        list(APPEND PHOTOSNAP_QT_DEPLOY_OPTIONS EXCLUDE_PLUGIN_TYPES egldeviceintegrations generic)
        set(QT_DEPLOY_USE_PATCHELF ON CACHE BOOL "Use patchelf when adjusting deployed Qt plugin RPATHs")
    endif()

    qt_generate_deploy_app_script(
        TARGET photo-snap
        OUTPUT_SCRIPT photo_snap_deploy_script
        ${PHOTOSNAP_QT_DEPLOY_OPTIONS}
    )
    install(SCRIPT ${photo_snap_deploy_script})
    set(PHOTOSNAP_QT_DEPLOYMENT_API ON CACHE INTERNAL "Qt runtime deployment is configured")
else()
    set(PHOTOSNAP_QT_DEPLOYMENT_API OFF CACHE INTERNAL "Qt runtime deployment is configured")
endif()

add_subdirectory(tools/03-scan-cache)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
