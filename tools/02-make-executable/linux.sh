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

qt_cmake_args=()
if [[ -n "${CMAKE_PREFIX_PATH:-}" ]]; then
    qt_cmake_args+=("-DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH")
elif [[ -n "${Qt6_DIR:-}" ]]; then
    qt_cmake_args+=("-DQt6_DIR=$Qt6_DIR")
elif [[ -n "${Qt5_DIR:-}" ]]; then
    qt_cmake_args+=("-DQt5_DIR=$Qt5_DIR")
else
    for qt_major in 6 5; do
        for qt_root in "${HOME:-}/Qt" /opt/Qt; do
            if [[ -d "$qt_root" ]]; then
                qt_config="$(find "$qt_root" -type f -path "*/lib/cmake/Qt${qt_major}/Qt${qt_major}Config.cmake" -print -quit 2>/dev/null || true)"
                if [[ -n "$qt_config" ]]; then
                    qt_prefix="${qt_config%/lib/cmake/Qt${qt_major}/Qt${qt_major}Config.cmake}"
                    qt_cmake_args+=("-DCMAKE_PREFIX_PATH=$qt_prefix")
                    echo "Using Qt from: $qt_prefix"
                    break 2
                fi
            fi
        done
    done
fi

echo 'Configuring Photo Snap for Release...'
cmake -S "$project_root" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_BINDIR=bin \
    -DQT_DEPLOY_USE_PATCHELF=ON \
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$build_dir/runtime" \
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=$build_dir/runtime" \
    "${qt_cmake_args[@]}"

echo 'Building Photo Snap...'
cmake --build "$build_dir" --config Release --parallel

echo 'Installing and deploying Qt runtime files...'
cmake --install "$build_dir" --config Release --prefix "$output_dir"

application="$output_dir/bin/photo-snap"
if [[ ! -x "$application" ]]; then
    echo "Build completed, but the executable was not found at $application." >&2
    exit 1
fi

if ! grep -q '^PHOTOSNAP_QT_DEPLOYMENT_API:INTERNAL=ON$' "$build_dir/CMakeCache.txt"; then
    echo 'Standalone Linux deployment requires Qt 6.3 or newer with CMake deployment support.' >&2
    exit 1
fi

echo "Deployable application created: $application"
