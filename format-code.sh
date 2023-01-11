#!/bin/bash
set -ex

# cd to dir containing this script
MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
echo "My dir: $MY_DIR"
cd $MY_DIR

EXE=clang-format-12

if [[ $OSTYPE == 'darwin'* ]]; then
    EXE=clang-format
fi

find src -name "*.cpp" -print | xargs $EXE -i
find src -name "*.h" -print | xargs $EXE -i
find tests -name "*.cpp" -print | xargs $EXE -i
find tests -name "*.h" -print | xargs $EXE -i
