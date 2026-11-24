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

if(APPLE)
    set_target_properties(photo-snap-cache-scanner PROPERTIES
        INSTALL_RPATH "@executable_path/../Frameworks"
    )
    set(scanner_install_dir "photo-snap.app/Contents/MacOS")
else()
    set(scanner_install_dir bin)
endif()

install(TARGETS photo-snap-cache-scanner RUNTIME DESTINATION "${scanner_install_dir}")

if(QT_VERSION_MAJOR EQUAL 6 AND COMMAND qt_generate_deploy_app_script AND NOT APPLE)
    set(scanner_deploy_options)
    if(UNIX AND Qt6Core_VERSION VERSION_GREATER_EQUAL "6.10")
        list(APPEND scanner_deploy_options EXCLUDE_PLUGIN_TYPES egldeviceintegrations generic)
        set(QT_DEPLOY_USE_PATCHELF ON)
    endif()

    qt_generate_deploy_app_script(
        TARGET photo-snap-cache-scanner
        OUTPUT_SCRIPT photo_snap_scanner_deploy_script
        ${scanner_deploy_options}
    )
    install(SCRIPT ${photo_snap_scanner_deploy_script})
endif()
