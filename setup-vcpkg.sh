#!/usr/bin/env bash
# vcpkg uses VCPKG_ROOT instead of cwd
set -ex

# check env vars
if [[ -z "$VCPKG_ROOT" ]]; then
	echo "VCPKG_ROOT must be set to point to the vcpkg repo top dir."
    exit 1
fi

if [[ ! -d "$VCPKG_ROOT" ]]; then
    mkdir -p "$VCPKG_ROOT"
fi

# clone
cd $VCPKG_ROOT

if [[ ! -d ".git" ]]; then
    cd ..
    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg
else
    # float for now, might decide to pin at some point if problems
    git pull
fi

# bootstrap
if [[ ! -f "vcpkg" ]]; then
    ./bootstrap-vcpkg.sh
fi

$VCPKG_ROOT/vcpkg update
$VCPKG_ROOT/vcpkg remove --outdated --recurse

# No longer call install like this since now using manifest mode.
#$VCPKG_ROOT/vcpkg install --recurse fmt opencv4[core,jpeg,png,tiff] wxWidgets arrow[core,csv,filesystem,json,parquet] gtest
