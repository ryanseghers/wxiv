#!/bin/bash
set -ex

# cd to dir containing this script
MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
echo "My dir: $MY_DIR"
cd $MY_DIR

# lint test first
./test-format.sh

# argument: release type
BUILD_TYPE="${1:-Release}"

if [[ "$BUILD_TYPE" != "Release" ]] && [[ "$BUILD_TYPE" != "Debug" ]]; then
	echo "BUILD_TYPE must be Release or Debug"
    exit 1
fi

./unit-test.sh ${BUILD_TYPE}
