cd esplay-launcher
make -j8
cd ../esplay-gnuboy
make -j8
cd ../esplay-nofrendo
make -j8
cd ../esplay-smsplusgx
make -j8
cd ..
ffmpeg -i Tile.png -f rawvideo -pix_fmt rgb565 tile.raw
/home/pebri/Projects/ESP32/esplay/esplay-base-firmware/tools/mkfw/mkfw Retro-Emulation tile.raw 0 16 655360 launcher esplay-launcher/build/esplay-launcher.bin 0 17 655360 esplay-nofrendo esplay-nofrendo/build/esplay-nofrendo.bin 0 18 655360 esplay-gnuboy esplay-gnuboy/build/esplay-gnuboy.bin 0 19 1310720 esplay-smsplusgx esplay-smsplusgx/build/esplay-smsplusgx.bin
rm esplay-retro-emu.fw
mv firmware.fw esplay-retro-emu.fw
