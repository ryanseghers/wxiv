#!/bin/bash
# Clean (this source, not vcpkg) and build all, including vcpkg dependencies (except for the cleaning part), and test.
set -ex

# cd to dir containing this script
MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
echo "My dir: $MY_DIR"
cd $MY_DIR

# check env vars
if [[ -z "$VCPKG_ROOT" ]]; then
	echo "VCPKG_ROOT must be set to point to the vcpkg repo top dir."
    exit 1
fi

# build dependencies
./setup-vcpkg.sh

# argument: release type
BUILD_TYPE="${1:-Release}"

if [[ "$BUILD_TYPE" != "Release" ]] && [[ "$BUILD_TYPE" != "Debug" ]]; then
	echo "BUILD_TYPE must be Release or Debug"
    exit 1
fi

# check cmake will be able to use git to fetch without prompting
set +e
ssh -oBatchMode=yes -T git@github.com

if [ $? == 255 ]; then
    echo "ERROR: ssh cannot access github without prompting for key passphrase."
    exit 1
fi

set -e

# C++ build
./clean.sh
./build.sh $BUILD_TYPE
./test.sh $BUILD_TYPE
