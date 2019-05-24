# ![Gaboze Express](Assets/GabozeExpress.png)
> Software Branch

This branch contains the specific modification for the Odroid Go hardware to use the 320x240 ILI9342 2.6" display

- [x] Odroid Go - [Firmware](https://github.com/OtherCrashOverride/odroid-go-firmware)
- [x] Odroid Go - [Go Play Emulators](https://github.com/OtherCrashOverride/go-play)
- [x] Launcher based on [GoGo](https://bitbucket.org/odroid_go_stuff/gogo/src/master/) Launcher 

## Usage
> How To

Clone the Software Branch of the [Official Gaboze Express](https://github.com/gaboze-express/GabozeExpress)

```shell
$ git clone -b Software --single-branch git@github.com:gaboze-express/GabozeExpress.git --recursive
```
It will take a while since we run a few ```git submodule``` inside the repository

## Bash Scripts
> Some tools to help you along your way

Navigate to you newly cloned repository and execute the following
```
chmod -R 777 Scripts
cd Scripts
```

### Submodule Branches
> Make sure all the submodules are on the right branches

```
./branches
```

### Replace files for ILI9342 drivers
> Copy relevant files for ILI9342 TFT LCD

```
./replace
```

### Build Firmware
> Copy relevant files for ILI9342 TFT LCD

```
./replace
```

##### Follow Onscreen Prompts
> if this is your first time running the executable, answer **Y** to the options

```
$ ./firmware
```

You will now have a file called **GabozeExpress.fw** in the Firmware/Release folder. 

You can copy this onto your *SD Card* into the ***odroid/firmware*** folder

##### Enabling new firmware
> Insert the sd card into your Gaboze Express (Odroid Go) and follow the prompts

| Power On | List                    | Selection     |    Verification  |   Write   |   Reboot   |
| ----------------------------------------- | -------------------------------- | ---- | ---- | ---- | ---- |
| ![Power on your hardware](Assets/001.jpg) | ![Firmware List](Assets/002.jpg) |   ![Firmware Selection](Assets/003.jpg)   |  ![Firmware Verification](Assets/004.jpg)    |   ![Firmware Write](Assets/006.gif)    |   ![Reboot into new Firmware](Assets/007.jpg)   |


### OTA
> Flash directly to ESP32

```
./ota
```

### Erase
> Erase flash and storage from ESP32

```
./erase
```

### Arduino
> Create Firmware from Arduino ```.ino.bin``` file

```
./arduino
```

You will now have a ```.fw``` file in Arduino/firmware folder. 

You can copy this onto your *SD Card* into the ***odroid/firmware*** folder