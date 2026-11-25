#!/usr/bin/env bash
set -Eeuo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
output_dir="$project_root/output"
build_dir="$output_dir/build"
mkdir -p -- "$output_dir"

if ! command -v cmake >/dev/null 2>&1; then
    echo 'CMake was not found on PATH. Install CMake 3.16 or newer.' >&2
    exit 1
fi

cmake_args=(
    -S "$project_root"
    -B "$build_dir"
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
    -DCMAKE_INSTALL_BINDIR=bin
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$build_dir/runtime"
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=$build_dir/runtime"
)

if [[ -n "${CMAKE_PREFIX_PATH:-}" ]]; then
    cmake_args+=("-DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH")
elif [[ -n "${QT_ROOT_DIR:-}" ]]; then
    cmake_args+=("-DCMAKE_PREFIX_PATH=$QT_ROOT_DIR")
elif [[ -n "${Qt6_DIR:-}" ]]; then
    cmake_args+=("-DQt6_DIR=$Qt6_DIR")
elif [[ -n "${Qt5_DIR:-}" ]]; then
    cmake_args+=("-DQt5_DIR=$Qt5_DIR")
else
    for qt_major in 6 5; do
        for qt_root in "${HOME:-}/Qt" /opt/Qt; do
            if [[ -d "$qt_root" ]]; then
                qt_config="$(find "$qt_root" -type f -path "*/lib/cmake/Qt${qt_major}/Qt${qt_major}Config.cmake" -print -quit 2>/dev/null || true)"
                if [[ -n "$qt_config" ]]; then
                    qt_prefix="${qt_config%/lib/cmake/Qt${qt_major}/Qt${qt_major}Config.cmake}"
                    cmake_args+=("-DCMAKE_PREFIX_PATH=$qt_prefix")
                    echo "Using Qt from: $qt_prefix"
                    break 2
                fi
            fi
        done
    done
fi

echo 'Configuring Photo Snap for Release...'
cmake "${cmake_args[@]}"

echo 'Building Photo Snap...'
cmake --build "$build_dir" --config Release --parallel

echo 'Installing and deploying Qt runtime files...'
cmake --install "$build_dir" --config Release --prefix "$output_dir"

application="$output_dir/photo-snap.app"
if [[ ! -d "$application" ]]; then
    echo "Build completed, but the application bundle was not found at $application." >&2
    exit 1
fi

deploy_tool=""
for qt_prefix in "${CMAKE_PREFIX_PATH:-}" "${QT_ROOT_DIR:-}" "${Qt6_DIR:-}" "${Qt5_DIR:-}"; do
    if [[ -n "$qt_prefix" ]]; then
        if [[ "$qt_prefix" == */lib/cmake/Qt[56] ]]; then
            qt_prefix="${qt_prefix%/lib/cmake/Qt[56]}"
        fi
        if [[ -x "$qt_prefix/bin/macdeployqt" ]]; then
            deploy_tool="$qt_prefix/bin/macdeployqt"
            break
        fi
    fi
done

if [[ -z "$deploy_tool" ]]; then
    qt_dir="$(sed -n -E 's/^Qt(6|5)_DIR:PATH=(.*)$/\2/p' "$build_dir/CMakeCache.txt" | head -n 1)"
    if [[ -n "$qt_dir" ]]; then
        qt_prefix="${qt_dir%/lib/cmake/Qt[56]}"
        if [[ -x "$qt_prefix/bin/macdeployqt" ]]; then
            deploy_tool="$qt_prefix/bin/macdeployqt"
        fi
    fi
fi

if [[ -z "$deploy_tool" ]]; then
    deploy_tool="$(command -v macdeployqt || true)"
fi

if [[ -z "$deploy_tool" ]]; then
    echo 'macdeployqt was not found. Install or select a Qt macOS kit containing macdeployqt.' >&2
    exit 1
fi

echo "Deploying Qt frameworks with: $deploy_tool"
"$deploy_tool" "$application" -always-overwrite -verbose=1

framework_dir="$application/Contents/Frameworks"
required_frameworks=(QtCore QtGui QtWidgets QtNetwork QtSql)
for framework in "${required_frameworks[@]}"; do
    framework_path="$framework_dir/$framework.framework/Versions/A/$framework"
    if [[ ! -f "$framework_path" ]]; then
        echo "Qt deployment verification failed: $framework_path is missing." >&2
        exit 1
    fi
done

if command -v codesign >/dev/null 2>&1; then
    echo 'Ad-hoc signing the application bundle...'
    codesign --force --deep --sign - "$application"
    codesign --verify --deep --strict --verbose=2 "$application"
else
    echo 'Warning: codesign was not found; the application bundle will remain unsigned.' >&2
fi

echo "Deployable application bundle created: $application"
