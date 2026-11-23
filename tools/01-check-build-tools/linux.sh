#!/usr/bin/env bash
set -Eeuo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"

if ! command -v cmake >/dev/null 2>&1; then
    echo 'MISSING: CMake 3.16 or newer.' >&2
    echo 'Ubuntu/Debian: sudo apt update && sudo apt install cmake' >&2
    echo 'Fedora: sudo dnf install cmake' >&2
    echo 'Arch Linux: sudo pacman -S --needed cmake' >&2
    exit 1
fi
cmake_version="$(cmake --version | sed -n '1s/^cmake version //p')"
IFS=. read -r cmake_major cmake_minor _ <<<"$cmake_version"
if (( cmake_major < 3 || (cmake_major == 3 && cmake_minor < 16) )); then
    echo "MISSING: CMake $cmake_version is older than 3.16." >&2
    echo 'Ubuntu/Debian: sudo apt update && sudo apt install cmake (if still too old, install the latest Linux release from https://cmake.org/download/).' >&2
    echo 'Fedora: sudo dnf upgrade cmake' >&2
    echo 'Arch Linux: sudo pacman -Syu cmake' >&2
    exit 1
fi
echo "OK: CMake $cmake_version"

cxx_compiler="$(command -v c++ || command -v g++ || command -v clang++ || true)"
if [[ -z "$cxx_compiler" ]]; then
    echo 'MISSING: A C++17 compiler and standard development tools.' >&2
    echo 'Ubuntu/Debian: sudo apt update && sudo apt install build-essential' >&2
    echo 'Fedora: sudo dnf group install "Development Tools" && sudo dnf install gcc-c++' >&2
    echo 'Arch Linux: sudo pacman -S --needed base-devel' >&2
    exit 1
fi
echo "OK: C++ compiler ($($cxx_compiler --version | head -n 1))"

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
                    break 2
                fi
            fi
        done
    done
fi

build_dir="$(mktemp -d "${TMPDIR:-/tmp}/photo-snap-check.XXXXXX")"
build_succeeded=0
cleanup() {
    status=$?
    trap - EXIT
    if [[ "$build_succeeded" -eq 1 ]]; then
        rm -rf -- "$build_dir"
    else
        echo "Build files kept for inspection: $build_dir" >&2
    fi
    exit "$status"
}
trap cleanup EXIT

if ! cmake -S "$project_root" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release "${qt_cmake_args[@]}" >"$build_dir/configure.log" 2>&1; then
    cat "$build_dir/configure.log" >&2
    if grep -Eqi 'Could not find.*QT|Qt[56].*(NOTFOUND|not found)|Qt Widgets|Qt[56]Config|Qt[56]_DIR' "$build_dir/configure.log"; then
        echo 'MISSING: Qt Widgets development files.' >&2
        echo 'Ubuntu/Debian: sudo apt update && sudo apt install qt6-base-dev' >&2
        echo 'Fedora: sudo dnf install qt6-qtbase-devel' >&2
        echo 'Arch Linux: sudo pacman -S --needed qt6-base' >&2
        echo 'For Qt 5 instead, install qtbase5-dev (Debian/Ubuntu), qt5-qtbase-devel (Fedora), or qt5-base (Arch).' >&2
    else
        echo 'MISSING: A working C++ build configuration. Install or repair the compiler and development tools listed above.' >&2
    fi
    exit 1
fi

qt_entry="$(grep -E '^Qt(6|5)_DIR:PATH=' "$build_dir/CMakeCache.txt" | head -n 1 || true)"
if [[ "$qt_entry" =~ ^Qt([56])_DIR:PATH= ]]; then
    echo "OK: Qt ${BASH_REMATCH[1]} Widgets"
else
    echo 'MISSING: Qt Widgets was not selected by CMake.' >&2
    echo 'Ubuntu/Debian: sudo apt update && sudo apt install qt6-base-dev' >&2
    echo 'Fedora: sudo dnf install qt6-qtbase-devel' >&2
    echo 'Arch Linux: sudo pacman -S --needed qt6-base' >&2
    exit 1
fi

if ! cmake --build "$build_dir" --config Release --parallel >"$build_dir/build.log" 2>&1; then
    cat "$build_dir/build.log" >&2
    echo 'Build failed. Check the compiler output above; if a system library is missing, install its development package and rerun.' >&2
    exit 1
fi

build_succeeded=1
echo 'OK: Photo Snap Release build'
