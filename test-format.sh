#!/bin/bash
set -ex

# cd to dir containing this script
MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
echo "My dir: $MY_DIR"
cd $MY_DIR

EXE=clang-format

if [[ $OSTYPE == 'darwin'* ]]; then
    EXE=clang-format
fi

find src -name "*.cpp" -print | xargs $EXE --Werror --dry-run --ferror-limit 1
find src -name "*.h" -print | xargs $EXE --Werror --dry-run --ferror-limit 1
find tests -name "*.cpp" -print | xargs $EXE --Werror --dry-run --ferror-limit 1
find tests -name "*.h" -print | xargs $EXE --Werror --dry-run --ferror-limit 1
