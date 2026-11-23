cmake_minimum_required(VERSION 3.16)

project(PhotoSnap VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Network)

add_executable(photo-snap WIN32 MACOSX_BUNDLE
    src/main.cpp
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
)

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

# Qt 6.3 and newer can generate a platform-aware deployment script that
# copies Qt libraries, plugins, and runtime dependencies beside the app.
if(QT_VERSION_MAJOR EQUAL 6 AND COMMAND qt_generate_deploy_app_script)
    qt_generate_deploy_app_script(
        TARGET photo-snap
        OUTPUT_SCRIPT photo_snap_deploy_script
    )
    install(SCRIPT ${photo_snap_deploy_script})
    set(PHOTOSNAP_QT_DEPLOYMENT_API ON CACHE INTERNAL "Qt runtime deployment is configured")
else()
    set(PHOTOSNAP_QT_DEPLOYMENT_API OFF CACHE INTERNAL "Qt runtime deployment is configured")
endif()

add_subdirectory(tools/03-scan-cache)
