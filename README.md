# Gaboze Express
> Software Branch

This branch contains the specific modification for the Odroid Go hardware to use the 320x240 ILI9342 2.6" display

- [x] Odroid Go - [Firmware](https://github.com/OtherCrashOverride/odroid-go-firmware)
- [x] Odroid Go - [Go Play Emulators](https://github.com/OtherCrashOverride/go-play)
- [x] Launcher based on [GoGo](https://bitbucket.org/odroid_go_stuff/gogo/src/master/) Launcher 

## Usage
> How To

Clone the Software Branch of the [Official Gaboze Express](https://github.com/gaboze-express/GabozeExpress)

```shell
$ git clone -b Software --single-branch git@github.com:gaboze-express/GabozeExpress.git
```

### firmware.sh
> inside scripts folder

Navigate to the /scripts folder of your newly cloned repository and set permission for ```firmware``` shell file

##### permissions for  firmware file

```shell
$ chmod 777 firmware
```

##### edit the firmware file

```shell
#---------------------------------
# EDIT PATHS FOR ESP AND IDF
#---------------------------------
export PATH="$PATH:$HOME/esp/xtensa-esp32-elf/bin"
export IDF_PATH=~/esp/esp-idf

#---------------------------------
# EDIT PATHS FOR REPOSITORY
#---------------------------------
# REPLACE path_to_repository with the actual path to your local repository
#---------------------------------
GOGO_PATH="[path_to_repository]/GabozeExpressOfficial/gogo/"
GOPLAY_PATH="[path_to_repository]/GabozeExpressOfficial/go-play/"
MKFW_PATH="[path_to_repository]/GabozeExpressOfficial/odroid-go-firmware/tools/mkfw"
```
##### execute the firmware file
> follow the prompts on screen
> if this is your first time running the executable, answer **Y** to the options
```
$ ./firmware
```

You will now have a file called **GabozeExpress.fw** in the scripts folder. You can copy this onto your *SD Card* into the ***odroid/firmware*** folder

## Enabling new firmware
> Insert the sd card into your Gaboze Express (Odroid Go) and follow the prompts

| Step     | Image                                     |
| -------- | ----------------------------------------- |
| Power On | ![Power on your hardware](assets/001.jpg) |
| Firmware List | ![Firmware List](assets/002.jpg) |
| Firmware Selection | ![Firmware Selection](assets/003.jpg) |
| Firmware Verification | ![Firmware Verification](assets/004.jpg) |
| Firmware Write | ![Firmware Write](assets/006.gif) |
| Reboot into new Firmware | ![Reboot into new Firmware](assets/007.jpg) |


