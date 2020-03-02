/*
  C
*/
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <math.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>

/*
  General
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sdmmc_cmd.h"

/*
  CRC
*/
#include <rom/crc.h>

/*
  ESP
*/
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_spiffs.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"

/*
  Drivers
*/
#include "driver/i2c.h"
#include "driver/gpio.h"
#include <driver/adc.h>
#include "esp_adc_cal.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "driver/ledc.h"

/*
  Odroid
*/
#include "../../components/odroid/odroid_settings.h"
#include "../../components/odroid/odroid_system.h"
#include "../../components/odroid/odroid_sdcard.h"
#include "../../components/odroid/odroid_display.h"
#include "../../components/odroid/odroid_input.h"
#include "../../components/odroid/odroid_audio.h"

/*
  Sprites
*/
#include "../sprites/5x7font.h"
#include "../sprites/battery.h"
#include "../sprites/brightness.h"
//#include "../sprites/characters.h"
#include "../sprites/folder.h"
#include "../sprites/icons.h"
#include "../sprites/logo.h"
#include "../sprites/media.h"
#include "../sprites/media_color.h"
#include "../sprites/speaker.h"
#include "../sprites/toggle.h"

/*
  Systems
*/
#include "../sprites/systems/a26.h"
#include "../sprites/systems/a26_color.h"
#include "../sprites/systems/a78.h"
#include "../sprites/systems/a78_color.h"
#include "../sprites/systems/col.h"
#include "../sprites/systems/col_color.h"
#include "../sprites/systems/fav.h"
#include "../sprites/systems/fav_color.h"
#include "../sprites/systems/gb.h"
#include "../sprites/systems/gb_color.h"
#include "../sprites/systems/gbc.h"
#include "../sprites/systems/gbc_color.h"
#include "../sprites/systems/gg.h"
#include "../sprites/systems/gg_color.h"
#include "../sprites/systems/lynx.h"
#include "../sprites/systems/lynx_color.h"
#include "../sprites/systems/nec.h"
#include "../sprites/systems/nec_color.h"
#include "../sprites/systems/nes.h"
#include "../sprites/systems/nes_color.h"
#include "../sprites/systems/settings.h"
#include "../sprites/systems/settings_color.h"
#include "../sprites/systems/sms.h"
#include "../sprites/systems/sms_color.h"
#include "../sprites/systems/zx.h"
#include "../sprites/systems/zx_color.h"
