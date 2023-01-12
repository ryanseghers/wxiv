#!/bin/bash
# Build installer using cpack, only after successful build.
set -ex

# cd to dir containing this script
MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
echo "My dir: $MY_DIR"
cd $MY_DIR

# always Release build
BUILD_TYPE="Release"

./build.sh $BUILD_TYPE

BUILD_DIR=./build/$BUILD_TYPE
cd $BUILD_DIR
cpack

if [[ $OSTYPE == 'darwin'* ]]; then
    echo "The dmg is something like build/Release/wxiv-0.0.1-Darwin.dmg"
    echo "On mac you probably double-click the dmg to run or install it."
else
    echo "If successful, you can install something like this:"
    echo "    sudo dpkg -i $HOME/src/wxiv/build/Release/wxiv-0.0.1-Linux.deb"
    echo "And if there are dependency problems, it will probably have already installed, but then run this to install dependencies:"
    echo "    sudo apt install -f"
    echo "And can uninstall something like this:"
    echo "    sudo apt remove -y wxiv"
fi
