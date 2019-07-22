# ![Retro ESP32](Assets/logo.jpg)
> Software Branch

This branch contains the specific modification for the Odroid Go hardware to use the 320x240 ILI9342 2.6" and 240x320 ILI9341 display

- [x] Odroid Go - [Firmware](https://github.com/OtherCrashOverride/odroid-go-firmware)
- [x] Odroid Go - [Go Play Emulators](https://github.com/OtherCrashOverride/go-play)
- [x] Launcher based on [GoGo](https://bitbucket.org/odroid_go_stuff/gogo/src/master/) Launcher
- [x] ROM Management
- [x] Config for both ILI9341/2 

# Usage
> How To

Clone the Software Branch of the [Official Retro ESP32](https://github.com/retro-esp32/RetroESP32/)

```shell
git clone -b Software --single-branch git@github.com:gaboze-express/GabozeExpress.git --recursive
git submodule update --init --recursive
git submodule foreach git pull origin master
```
It will take a while since we run a few ```git submodule``` inside the repository

# Setup Toolchain
> You will need the xtensa esp32 toolchain

### Guides

***Note:*** You only need to install the xtensa toolchain. The esp-idf is included in this repository.

- [Linux](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/linux-setup.html)
- [Mac OS](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/macos-setup.html)
- [Windows](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/windows-setup-scratch.html)


# Bash Scripts
> Some tools to help you along your way

Navigate to you newly cloned repository and execute the following
```
chmod -R 777 Scripts
cd Scripts
```

***Note:*** All bash scripts **MUST** be executed from the Scripts folder.


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

### ESP32 Environment Installer
> Install all thing necessary (NIX/OS X)  *Windows Coming Soon*

```
./installer
```

### Environmanet Variables
> Set all your paths
Be sure to run installer first

```
source paths
```

### Build Firmware

> Execute ```source paths``` before proceeding


##### Follow Onscreen Prompts
> if this is your first time running the executable, answer **Y** to the options

```
./firmware
```

You will now have a file called **Retro ESP32.fw** in the Firmware/Release folder.

You can copy this onto your *SD Card* into the ***odroid/firmware*** folder

##### Enabling new firmware
> Insert the sd card into your Retro ESP32 /  (Odroid Go) and follow the prompts

| Power On | List                    | Selection     |    Verification  |   Write   |   Reboot   |
| ----------------------------------------- | -------------------------------- | ---- | ---- | ---- | ---- |
| ![Power on your hardware](Assets/001.jpg) | ![Firmware List](Assets/002.jpg) |   ![Firmware Selection](Assets/003.jpg)   |  ![Firmware Verification](Assets/004.jpg)    |   ![Firmware Write](Assets/006.gif)    |   ![Reboot into new Firmware](Assets/007.jpg)   |


### OTA
> Flash directly to ESP32

This is the easiest option, simple follow the onscreen prompts!

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


### ![Retro ESP32](Assets/retro-esp32/logo.jpg)
# Retro ESP32
> Software Branch

Finally a new launcher for your Odroid-Go

```
make menuconfig inside Launchers/retro-esp32
```

1. Select ```LCD Screen Driver``` ![LCD Screen Driver](Assets/menuconfig.png)

2. Select your display type ![LCD Driver Chip](Assets/driver.png)
  - [x] Odroid Go Default (ILI9341) [2.4"] ***default***
  - [ ] Retro ESP32 (ILI9342) [2.6"]
  
3. Run ```./ota```  (see above)

|   Splash   |   Navigation   |   ROM State Management   |   Run   |
| ---- | ---- | ---- | ---- |
|  ![](Assets/launcher/splash.jpg)    | ![](Assets/launcher/games.jpg)    | ![](Assets/launcher/manager.jpg)    | ![](Assets/launcher/run.jpg)    |

- [ ] 