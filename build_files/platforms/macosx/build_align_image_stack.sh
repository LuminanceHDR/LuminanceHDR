#!/usr/bin/env bash

set -euo pipefail

HUGIN_VERSION="2025.0.1"
HUGIN_SHA256="7cf8eb33a6a8848cc7f816faf4bc88389228883d5513136dccb5cb243912ab79"
HUGIN_URL="https://downloads.sourceforge.net/project/hugin/hugin/hugin-2025.0/hugin-${HUGIN_VERSION}.tar.bz2"
VIGRA_COMMIT="de98f930b66d461360a2d5dc8f9adfa84bb01058"
VIGRA_URL="https://github.com/ukoethe/vigra.git"

usage() {
    echo "Usage: $0 OUTPUT_PATH" >&2
    echo "Builds a native align_image_stack from pinned Hugin and VIGRA sources." >&2
}

if [[ $# -ne 1 ]]; then
    usage
    exit 2
fi

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "This script must run on macOS." >&2
    exit 1
fi

for tool in brew cmake ninja git curl shasum tar patch lipo; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Required tool not found: $tool" >&2
        exit 1
    fi
done

output_parent=$(dirname "$1")
mkdir -p "$output_parent"
output_parent=$(cd "$output_parent" && pwd -P)
output_path="${output_parent}/$(basename "$1")"
if [[ -e "$output_path" ]]; then
    echo "Refusing to overwrite existing output: $output_path" >&2
    exit 1
fi

architecture="${LHDR_ARCHITECTURES:-arm64}"
deployment_target="${MACOSX_DEPLOYMENT_TARGET:-14.0}"
brew_prefix=$(brew --prefix)
for formula in libomp libtiff jpeg-turbo libpng zlib sqlite; do
    if ! brew list --versions "$formula" >/dev/null 2>&1; then
        echo "Required Homebrew formula is not installed: $formula" >&2
        exit 1
    fi
done
libomp_prefix=$(brew --prefix libomp)
tiff_prefix=$(brew --prefix libtiff)
jpeg_prefix=$(brew --prefix jpeg-turbo)
png_prefix=$(brew --prefix libpng)
zlib_prefix=$(brew --prefix zlib)
sqlite_prefix=$(brew --prefix sqlite)

cleanup_work=0
if [[ -n "${LHDR_HUGIN_WORK_DIR:-}" ]]; then
    work_dir="$LHDR_HUGIN_WORK_DIR"
    if [[ -e "$work_dir" ]] && [[ -n "$(find "$work_dir" -mindepth 1 -maxdepth 1 -print -quit 2>/dev/null)" ]]; then
        echo "LHDR_HUGIN_WORK_DIR must be empty: $work_dir" >&2
        exit 1
    fi
    mkdir -p "$work_dir"
    work_dir=$(cd "$work_dir" && pwd -P)
else
    work_dir=$(mktemp -d "${TMPDIR:-/tmp}/lhdr-hugin.XXXXXX")
    cleanup_work=1
fi

cleanup() {
    if [[ "$cleanup_work" -eq 1 && "${LHDR_KEEP_HUGIN_WORK:-0}" != "1" ]]; then
        rm -rf -- "$work_dir"
    else
        echo "Hugin work directory: $work_dir"
    fi
}
trap cleanup EXIT

vigra_source="$work_dir/vigra-source"
vigra_build="$work_dir/vigra-build"
vigra_prefix="$work_dir/vigra-prefix"
hugin_archive="$work_dir/hugin-${HUGIN_VERSION}.tar.bz2"
hugin_source="$work_dir/hugin-${HUGIN_VERSION}"
hugin_build="$work_dir/hugin-build"

git init -q "$vigra_source"
git -C "$vigra_source" remote add origin "$VIGRA_URL"
git -C "$vigra_source" fetch -q --depth 1 origin "$VIGRA_COMMIT"
git -C "$vigra_source" checkout -q --detach FETCH_HEAD

cmake -S "$vigra_source" -B "$vigra_build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="$architecture" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$deployment_target" \
    -DCMAKE_INSTALL_PREFIX="$vigra_prefix" \
    -DVIGRA_STATIC_LIB=ON \
    -DWITH_VIGRANUMPY=OFF \
    -DWITH_HDF5=OFF \
    -DWITH_OPENEXR=OFF \
    -DWITH_VALGRIND=OFF
cmake --build "$vigra_build" --target install --parallel

curl --fail --location --retry 5 --retry-all-errors \
    --output "$hugin_archive" "$HUGIN_URL"
actual_sha=$(shasum -a 256 "$hugin_archive" | awk '{print $1}')
if [[ "$actual_sha" != "$HUGIN_SHA256" ]]; then
    echo "Hugin source checksum mismatch: $actual_sha" >&2
    exit 1
fi
tar -xjf "$hugin_archive" -C "$work_dir"

patch -d "$hugin_source" -p1 <<'PATCH'
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -19,7 +19,9 @@ if(POLICY CMP0127)
 endif()

 if(APPLE)
-  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
+  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
+    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
+  endif()
   if (MAC_SELF_CONTAINED_BUNDLE)
       set(CMAKE_LIBRARY_PATH ${CMAKE_SOURCE_DIR}/mac/ExternalPrograms/repository/lib)
       set(CMAKE_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/mac/ExternalPrograms/repository/include ${CMAKE_SOURCE_DIR}/mac/ExternalPrograms/repository/bin)
--- a/CMakeModules/FindVIGRA.cmake
+++ b/CMakeModules/FindVIGRA.cmake
@@ -52,8 +52,21 @@ IF (VIGRA_FOUND)
   IF(NOT VIGRA_CONFIG_VERSION_HXX)
     MESSAGE(FATAL_ERROR "Could not find vigra/configVersion.hxx or vigra/config_version.hxx. Your vigra installation seems to be corrupt.")
   ENDIF()
-  FILE(STRINGS "${VIGRA_CONFIG_VERSION_HXX}" VIGRA_VERSION_HXX REGEX ".*#define +VIGRA_VERSION +\"")
-  STRING(REGEX REPLACE ".*#define +VIGRA_VERSION +\"([.0-9]+).*" "\\1" VIGRA_VERSION "${VIGRA_VERSION_HXX}")
+  FILE(STRINGS "${VIGRA_CONFIG_VERSION_HXX}" VIGRA_VERSION_HXX REGEX ".*#define +VIGRA_VERSION +\"")
+  IF(VIGRA_VERSION_HXX)
+    STRING(REGEX REPLACE ".*#define +VIGRA_VERSION +\"([.0-9]+).*" "\\1" VIGRA_VERSION "${VIGRA_VERSION_HXX}")
+  ELSE()
+    FILE(STRINGS "${VIGRA_CONFIG_VERSION_HXX}" VIGRA_VERSION_MAJOR_HXX REGEX ".*#define +VIGRA_VERSION_MAJOR +[0-9]+")
+    FILE(STRINGS "${VIGRA_CONFIG_VERSION_HXX}" VIGRA_VERSION_MINOR_HXX REGEX ".*#define +VIGRA_VERSION_MINOR +[0-9]+")
+    FILE(STRINGS "${VIGRA_CONFIG_VERSION_HXX}" VIGRA_VERSION_PATCH_HXX REGEX ".*#define +VIGRA_VERSION_PATCH +[0-9]+")
+    STRING(REGEX REPLACE ".*#define +VIGRA_VERSION_MAJOR +([0-9]+).*" "\\1" VIGRA_VERSION_MAJOR "${VIGRA_VERSION_MAJOR_HXX}")
+    STRING(REGEX REPLACE ".*#define +VIGRA_VERSION_MINOR +([0-9]+).*" "\\1" VIGRA_VERSION_MINOR "${VIGRA_VERSION_MINOR_HXX}")
+    STRING(REGEX REPLACE ".*#define +VIGRA_VERSION_PATCH +([0-9]+).*" "\\1" VIGRA_VERSION_PATCH "${VIGRA_VERSION_PATCH_HXX}")
+    IF(NOT VIGRA_VERSION_MAJOR OR NOT VIGRA_VERSION_MINOR OR NOT VIGRA_VERSION_PATCH)
+      MESSAGE(FATAL_ERROR "Could not determine the installed VIGRA version.")
+    ENDIF()
+    SET(VIGRA_VERSION "${VIGRA_VERSION_MAJOR}.${VIGRA_VERSION_MINOR}.${VIGRA_VERSION_PATCH}")
+  ENDIF()
   IF(${VIGRA_VERSION} VERSION_EQUAL VIGRA_FIND_VERSION OR ${VIGRA_VERSION} VERSION_GREATER VIGRA_FIND_VERSION)
     SET(VIGRA_VERSION_CHECK TRUE)
     MESSAGE(STATUS "VIGRA version: ${VIGRA_VERSION}")
PATCH

# GitHub's macOS image includes a Mono framework with stale image-library
# headers. Hugin's custom find modules otherwise discover those before the
# native Homebrew libraries and produce incompatible typedefs.
cmake -S "$hugin_source" -B "$hugin_build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="$architecture" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$deployment_target" \
    -DCMAKE_PREFIX_PATH="${vigra_prefix};${brew_prefix}" \
    -DCMAKE_INCLUDE_PATH="${vigra_prefix}/include;${brew_prefix}/include" \
    -DCMAKE_LIBRARY_PATH="${vigra_prefix}/lib;${brew_prefix}/lib" \
    -DCMAKE_IGNORE_PREFIX_PATH=/Library/Frameworks/Mono.framework \
    -DTIFF_INCLUDE_DIR:PATH="${tiff_prefix}/include" \
    -DTIFF_LIBRARIES:FILEPATH="${tiff_prefix}/lib/libtiff.dylib" \
    -DJPEG_INCLUDE_DIR:PATH="${jpeg_prefix}/include" \
    -DJPEG_LIBRARIES:FILEPATH="${jpeg_prefix}/lib/libjpeg.dylib" \
    -DPNG_INCLUDE_DIR:PATH="${png_prefix}/include" \
    -DPNG_LIBRARIES:FILEPATH="${png_prefix}/lib/libpng.dylib" \
    -DZLIB_INCLUDE_DIR:PATH="${zlib_prefix}/include" \
    -DZLIB_LIBRARIES:FILEPATH="${zlib_prefix}/lib/libz.dylib" \
    -DSQLITE3_INCLUDE_DIR:PATH="${sqlite_prefix}/include" \
    -DSQLITE3_LIBRARIES:FILEPATH="${sqlite_prefix}/lib/libsqlite3.dylib" \
    -DVIGRA_INCLUDE_DIR="${vigra_prefix}/include" \
    -DVIGRA_LIBRARIES="${vigra_prefix}/lib/libvigraimpex.a" \
    -DDISABLE_DPKG=ON \
    -DBUILD_HSI=OFF \
    -DENABLE_LAPACK=OFF \
    -DHUGIN_SHARED=OFF \
    "-DCMAKE_C_FLAGS=-I${libomp_prefix}/include" \
    "-DCMAKE_CXX_FLAGS=-I${libomp_prefix}/include" \
    "-DCMAKE_EXE_LINKER_FLAGS=-L${libomp_prefix}/lib -lomp" \
    -DOpenMP_C_FLAGS:STRING=-Xclang=-fopenmp \
    -DOpenMP_CXX_FLAGS:STRING=-Xclang=-fopenmp
cmake --build "$hugin_build" --target align_image_stack --parallel

helper_path=$(find "$hugin_build" -type f -name align_image_stack -perm -111 -print -quit)
if [[ -z "$helper_path" ]]; then
    echo "The Hugin build did not produce align_image_stack." >&2
    exit 1
fi

actual_architectures=$(lipo -archs "$helper_path")
if [[ "$actual_architectures" != "$architecture" ]]; then
    echo "Unexpected helper architecture: $actual_architectures (expected $architecture)" >&2
    exit 1
fi

install -m 755 "$helper_path" "$output_path"
"$output_path" --help >/dev/null
echo "Built $output_path ($actual_architectures) from Hugin $HUGIN_VERSION."
