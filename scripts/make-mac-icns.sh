#!/usr/bin/env bash
# thanks to: https://stackoverflow.com/questions/12306223/how-to-manually-create-icns-files-using-iconutil/20703594#20703594
set -ex

# cd to the icons dir
MY_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
echo "My dir: $MY_DIR"
pushd $MY_DIR/../icons

ICON_PNG="wxiv-icon-256.png"
WORK_DIR="wxiv.iconset"

if [[ ! -d "$WORK_DIR" ]]; then
    mkdir -p $WORK_DIR
fi

sips -z 16 16     $ICON_PNG --out "${WORK_DIR}/icon_16x16.png"
sips -z 32 32     $ICON_PNG --out "${WORK_DIR}/icon_16x16@2x.png"
sips -z 32 32     $ICON_PNG --out "${WORK_DIR}/icon_32x32.png"
sips -z 64 64     $ICON_PNG --out "${WORK_DIR}/icon_32x32@2x.png"
sips -z 128 128   $ICON_PNG --out "${WORK_DIR}/icon_128x128.png"
sips -z 256 256   $ICON_PNG --out "${WORK_DIR}/icon_128x128@2x.png"
sips -z 256 256   $ICON_PNG --out "${WORK_DIR}/icon_256x256.png"
sips -z 512 512   $ICON_PNG --out "${WORK_DIR}/icon_256x256@2x.png"
sips -z 512 512   $ICON_PNG --out "${WORK_DIR}/icon_512x512.png"
sips -z 1024 1024 $ICON_PNG --out "${WORK_DIR}/icon_512x512@2x.png"

iconutil -c icns $WORK_DIR

rm -rf $WORK_DIR

popd
