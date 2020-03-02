#!/bin/bash

# converts gif files to their corresponding RGB565 raw files
# also creates a helper C header file for resources embedding

for i in */*.gif
do
    FN="${i%.gif}"
    BN="`basename \"${FN}\"`"
    W=`ffprobe -v error -select_streams v:0 -show_entries stream=width -of default=nw=1:nk=1 "${i}"`
    H=`ffprobe -v error -select_streams v:0 -show_entries stream=height -of default=nw=1:nk=1 "${i}"`
    ffmpeg -i "${i}" -f rawvideo -pix_fmt rgb565 "${FN}.raw" -y
    cat >"${FN}.h" <<DONE

//extern const uint8_t* ${BN}_start asm("_binary_${BN}_raw_start");
//extern const uint8_t* ${BN}_end   asm("_binary_${BN}_raw_end");

extern const uint16_t ${BN}[${H}][${W}] asm("_binary_${BN}_raw_start");
DONE
done
