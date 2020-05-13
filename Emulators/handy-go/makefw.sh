#!/bin/bash
. ${IDF_PATH}/add_path.sh

export TITLE="Atari Lynx (pelle7-`date +%Y%m%d`)"
export IMG_TITLE=atarilynx-title.raw
export FW_FILENAME=lynx.fw

export EXE_MKFW=mkfw
export BIN_HANDY=build/handy-go.bin

$EXE_MKFW "$TITLE" $IMG_TITLE 0 16 1048576 atarilynx $BIN_HANDY
if [ $? -ne 0 ]; then
    echo MKFW failed
    exit 1
fi

mv firmware.fw $FW_FILENAME
echo info:
pwd
ls -lah *.fw
exit 0

convert atarilynx-title.jpg -resize 86x48\! atarilynx-title.png
ffmpeg -i "atarilynx-title.png" -f rawvideo -pix_fmt rgb565 atarilynx-title.raw 
