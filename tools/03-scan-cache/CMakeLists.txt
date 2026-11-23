cmake_minimum_required(VERSION 3.16)

project(PhotoSnapCacheScanner LANGUAGES CXX)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Sql Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Sql Network)

add_executable(photo-snap-cache-scanner
    ../../src/cache_scanner_main.cpp
    ../../src/imagecache.cpp
    ../../src/imagecache.h
    ../../src/imagemetadata.cpp
    ../../src/imagemetadata.h
)
target_compile_features(photo-snap-cache-scanner PRIVATE cxx_std_17)
target_link_libraries(photo-snap-cache-scanner PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
)

if(MSVC)
    target_compile_options(photo-snap-cache-scanner PRIVATE /W4 /permissive-)
else()
    target_compile_options(photo-snap-cache-scanner PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(QT_VERSION_MAJOR EQUAL 6 AND TARGET Qt6::QSQLiteDriverPlugin)
    qt_import_plugins(photo-snap-cache-scanner INCLUDE Qt6::QSQLiteDriverPlugin)
endif()

install(TARGETS photo-snap-cache-scanner RUNTIME DESTINATION bin)

if(QT_VERSION_MAJOR EQUAL 6 AND COMMAND qt_generate_deploy_app_script)
    qt_generate_deploy_app_script(
        TARGET photo-snap-cache-scanner
        OUTPUT_SCRIPT photo_snap_scanner_deploy_script
    )
    install(SCRIPT ${photo_snap_scanner_deploy_script})
endif()
