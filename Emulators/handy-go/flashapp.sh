. ${IDF_PATH}/add_path.sh
#export PORT=/dev/ttyUSB0
export PORT=/dev/cu.SLAB_USBtoUART

#export OFFSET=0x600000
export OFFSET=0x100000

esptool.py --chip esp32 --port $PORT --baud 921600 write_flash -fs detect --flash_freq 80m --flash_mode qio $OFFSET build/handy-go.bin
