#!/bin/bash
# Just wxiv build, not vcpkg, so that must already be good.
set -ex

# cd to dir containing this script
MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
echo "My dir: $MY_DIR"
cd $MY_DIR

# argument: release type
BUILD_TYPE="${1:-Release}"

if [[ "$BUILD_TYPE" != "Release" ]] && [[ "$BUILD_TYPE" != "Debug" ]]; then
	echo "BUILD_TYPE must be Release or Debug"
    exit 1
fi

# check env vars
if [[ -z "$VCPKG_ROOT" ]]; then
	echo "VCPKG_ROOT must be set to point to the vcpkg repo top dir."
    exit 1
fi

# cmake doesn't verify toolchain file exists
TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"

if [ ! -f "$TOOLCHAIN_FILE" ]
then
    echo ERROR: The cmake toolchain file does not exist: ${TOOLCHAIN_FILE}
    exit 1
fi

mkdir -p build/$BUILD_TYPE
pushd build/$BUILD_TYPE >/dev/null 2>&1

# cmake
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE} -GNinja ../..

# make
#make -j 8
cmake --build .

popd >/dev/null 2>&1

echo "The built binary is: ./build/$BUILD_TYPE/wxiv/wxiv"
