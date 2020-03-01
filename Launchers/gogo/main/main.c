// GO-GO - alternative launcher for ODROID-GO.

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_heap_caps.h"

#include "../components/odroid/odroid_settings.h"
#include "../components/odroid/odroid_input.h"
#include "../components/odroid/odroid_display.h"
#include "../components/odroid/odroid_audio.h"
#include "../components/odroid/odroid_system.h"
#include "../components/odroid/odroid_sdcard.h"

#include "font.h"
#include "icons.h"

// console icons
#include "icons/a26.c"
#include "icons/a78.c"
#include "icons/c64.c"
#include "icons/col.c"
#include "icons/gb.c"
#include "icons/gbc.c"
#include "icons/gg.c"
#include "icons/nes.c"
#include "icons/sms.c"
#include "icons/zx.c"

#include <string.h>
#include <dirent.h>


odroid_gamepad_state joystick;

odroid_battery_state battery_state;

int BatteryPercent = 100;

unsigned short buffer[40000];
int colour = 65535; // white text mostly.

#define ANIMATE true
#define STEPS 235
#define W 150
#define H 56
#define X ((320/2) - (W/2))
#define Y ((240/2) - (H/2))
#define MAX 9

uint16_t * ICONS[10] = {
  (uint16_t *)icon_nes.pixel_data,
  (uint16_t *)icon_gb.pixel_data,
  (uint16_t *)icon_gbc.pixel_data,
  (uint16_t *)icon_sms.pixel_data,
  (uint16_t *)icon_gg.pixel_data,   
  (uint16_t *)icon_col.pixel_data,  
  (uint16_t *)icon_zx.pixel_data,                               
  (uint16_t *)icon_a26.pixel_data,
  (uint16_t *)icon_a78.pixel_data,
  (uint16_t *)icon_c64.pixel_data
};

#define W 150
#define H 56
#define X ((320/2) - (W/2))
#define Y ((240/2) - (H/2))
#define MAX 9

#define STEPS 47
int PX[47] = { -145, -140, -135, -130, -125, -120, -115, -110, -105, -100, -95, -90, -85, -80, -75, -70, -65, -60, -55, -50, -45, -40, -35, -30, -25, -20, -15, -10, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85};
int NX[47] = {235, 230, 225, 220, 215, 210, 205, 200, 195, 190, 185, 180, 175, 170, 165, 160, 155, 150, 145, 140, 135, 130, 125, 120, 115, 110, 105, 100, 95, 90, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85, 85};

int num_emulators = 7;
char emulator[10][32] = {
  "Nintendo",
  "GAMEBOY",
  "GAMEBOY COLOR",
  "SEGA Master System",
  "GAME GEAR",
  "Colecovision",
  "ZX Spectrum 48K"
};
char emu_dir[10][20] = {
  "nes",
  "gb",
  "gbc",
  "sms",
  "gg",
  "col",
  "spectrum"
};
int emu_slot[10] = {1, 2, 2, 3, 3, 3, 4};

char target[256] = "";
int e = 0, last_e = 100, x, y = 0, count = 0;

//----------------------------------------------------------------
int read_config()
{
  FILE *fp;
  int v;

  if ((fp = fopen("/sd/odroid/data/gogo_conf.txt", "r"))) {
    while (!feof(fp)) {
      fscanf(fp, "%s %s %i\n", &emulator[num_emulators][0],
             &emu_dir[num_emulators][0],
             &emu_slot[num_emulators]);
      num_emulators++;
    }
  }
  return (0);
}

//----------------------------------------------------------------
void debounce(int key)
{
  while (joystick.values[key])
    odroid_input_gamepad_read(&joystick);
}

//----------------------------------------------------------------
// print a character string to screen...
void print(short x, short y, char *s)
{
  int i, n, k, a, len, idx;
  unsigned char c;
  //extern unsigned short  *buffer; // screen buffer, 16 bit pixels

  len = strlen(s);
  for (k = 0; k < len; k++) {
    c = s[k];
    for (i = 0; i < 8; i++) {
      a = font_8x8[c][i];
      for (n = 7, idx = i * len * 8 + k * 8; n >= 0; n--, idx++) {
        if (a % 2 == 1) buffer[idx] = colour; else buffer[idx] = 0;
        a = a >> 1;
      }
    }
  }
  ili9341_write_frame_rectangleLE(x * 8, y * 8, len * 8, 8, buffer);
}

//--------------------------------------------------------------
void print_y(short x, short y, char *s) // print, but yellow text...
{
  colour = 0b1111111111100000;
  print(x, y, s);
  colour = 65535;
}

//-------------------------------------------------------------
// display details for a particular emulator....
int print_emulator(int e, int y)
{
  int i, x, len, dotlen;
  DIR *dr;
  struct dirent *de;
  char path[256] = "/sd/roms/";
  char s[40];

#ifdef ANIMATE
  if (e != last_e)
  {
    for (i = 0; i < 320 * 56; i++) buffer[i] = 65535;
    ili9341_write_frame_rectangleLE(0, 50, 320, 56, buffer);
    ili9341_write_frame_rectangleLE(85, 50, 150, 56, ICONS[e]);
    last_e = e;
  }
#else
  if (e != last_e) {
    for (i = 0; i < 320 * 56; i++) buffer[i] = 65535;
    ili9341_write_frame_rectangleLE(0, 50, 320, 56, buffer);
    if (e < 7) for (i = 0; i < 150 * 56; i++) buffer[i] = icons[e * 150 * 56 + i];
    ili9341_write_frame_rectangleLE(85, 50, 150, 56, buffer);

    last_e = e;
  }
#endif

  for (i = 0; i < 40; i++) s[i] = ' '; s[i] = 0;
  len = strlen(emulator[e]);
  for (i = 0; i < len; i++) s[i + 19 - len / 2] = emulator[e][i];
  print(0, 18, s);

  strcat(&path[strlen(path) - 1], emu_dir[e]);
  count = 0; for (i = 0; i < 40; i++) s[i] = ' '; s[i] = 0;
  if (!(dr = opendir(path))) {for (i = 20; i < 30; i++) print(0, i, s); return (0);}
  while ((de = readdir(dr)) != NULL) {

    len = strlen(de->d_name); dotlen = strlen(emu_dir[e]);
    // only show files that have matching extension...
    if ( strcmp(&de->d_name[len - dotlen], emu_dir[e]) == 0 &&
         de->d_name[0] != '.')
    {
      //printf("file: %s\n",de->d_name);
      if (count == y) {
        strcpy(target, path);
        i = strlen(target); target[i] = '/'; target[i + 1] = 0;
        strcat(target, de->d_name);
        //printf("target=%s\n",target);
      }
      // strip extension from file...
      de->d_name[len - dotlen - 1] = 0;
      if (strlen(de->d_name) > 39) de->d_name[39] = 0;
      if (y / 10 == count / 10) { // right page?
        for (i = 0; i < 40; i++) s[i] = ' ';
        for (i = 0; i < strlen(de->d_name); i++) s[i] = de->d_name[i];
        if (count == y) print_y(0, (count % 10) + 20, s); // highlight
        else print(0, (count % 10) + 20, s);
      }
      count++;
    }
  }
  if (y / 10 == count / 10) for (i = count % 10; i < 10; i++) print(0, i + 20, "                                        ");
  closedir(dr);
  //printf("total=%i\n",count);
  return (0);
}

//----------------------------------------------------------------
// Return to last emulator if 'B' pressed....
int resume(void)
{

  int i;
  char *extension;
  char *romPath;


  //printf("trying to resume...\n");
  romPath = odroid_settings_RomFilePath_get();
  if (romPath)
  {
    extension = odroid_util_GetFileExtenstion(romPath);
    for (i = 0; i < strlen(extension); i++) extension[i] = extension[i + 1];
    printf("extension=%s\n", extension);

  } else {
    //printf("can't resume!\n");
    return (0);
  }
  for (i = 0; i < num_emulators; i++) if (strcmp(extension, &emu_dir[i][0]) == 0) {
      printf("resume - extension=%s, slot=%i\n", extension, i);
      odroid_system_application_set(emu_slot[i]); // set emulator slot
      print(14, 15, "RESUMING....");
      usleep(500000);
      esp_restart(); // reboot!
    }
  free(romPath); free(extension);
  return (0); // nope!
}


//----------------------------------------------------------------
void app_main(void)
{
  FILE *fp;
  char s[256];

  printf("odroid start.\n");

  nvs_flash_init();
  odroid_system_init();
  odroid_input_gamepad_init();

  // Boot state overrides
  bool forceConsoleReset = false;

  switch (esp_sleep_get_wakeup_cause())
  {
  case ESP_SLEEP_WAKEUP_EXT0:
  {
    printf("app_main: ESP_SLEEP_WAKEUP_EXT0 deep sleep wake\n");
    break;
  }

  case ESP_SLEEP_WAKEUP_EXT1:
  case ESP_SLEEP_WAKEUP_TIMER:
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
  case ESP_SLEEP_WAKEUP_ULP:
  case ESP_SLEEP_WAKEUP_UNDEFINED:
  {
    printf("app_main: Non deep sleep startup\n");

    odroid_gamepad_state bootState = odroid_input_read_raw();

    if (bootState.values[ODROID_INPUT_MENU])
    {
      // Force return to factory app to recover from
      // ROM loading crashes

      // Set factory app
      const esp_partition_t* partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
      if (partition == NULL)
      {
        abort();
      }

      esp_err_t err = esp_ota_set_boot_partition(partition);
      if (err != ESP_OK)
      {
        abort();
      }

      // Reset
      esp_restart();
    }

    if (bootState.values[ODROID_INPUT_START])
    {
      // Reset emulator if button held at startup to
      // override save state
      forceConsoleReset = true;
    }

    break;
  }
  default:
    printf("app_main: Not a deep sleep reset\n");
    break;
  }

  // Display
  ili9341_prepare();
  ili9341_init();

  odroid_input_battery_level_init();

  odroid_sdcard_open("/sd");   // map SD card.
  odroid_audio_init(16000);

  ili9341_clear(0); // clear screen
  sprintf(s, "Volume: %i%%   ", odroid_settings_Volume_get() * 25);
  print(0, 0, s);
  print(16, 0, "ODROID");
  odroid_input_battery_level_read(&battery_state);
  sprintf(s, "Battery: %i%%", battery_state.percentage);
  print(26, 0, s);

  read_config();

  print_emulator(e, y);
  while (1) {
    odroid_input_gamepad_read(&joystick);
    if (joystick.values[ODROID_INPUT_LEFT]) {
      y = 0; e--; if (e < 0) e = num_emulators - 1;
      print_emulator(e, y);
      debounce(ODROID_INPUT_LEFT);
    }
    if (joystick.values[ODROID_INPUT_RIGHT]) {
      y = 0; e++; if (e == num_emulators) e = 0;
      print_emulator(e, y);
      debounce(ODROID_INPUT_RIGHT);
    }
    if (joystick.values[ODROID_INPUT_UP]) {
      y--; if (y < 0) y = count - 1;
      print_emulator(e, y);
      debounce(ODROID_INPUT_UP);
    }
    if (joystick.values[ODROID_INPUT_DOWN]) {
      y++; if (y >= count) y = 0;
      print_emulator(e, y);
      debounce(ODROID_INPUT_DOWN);
    }
    if (joystick.values[ODROID_INPUT_SELECT] && y > 9) {
      y -= 10;
      print_emulator(e, y);
      debounce(ODROID_INPUT_SELECT);
    }
    if (joystick.values[ODROID_INPUT_START] && y < count - 10) {
      y += 10;
      print_emulator(e, y);
      debounce(ODROID_INPUT_START);
    }
    if (joystick.values[ODROID_INPUT_VOLUME]) {
      odroid_audio_volume_change();
      sprintf(s, "Volume: %i%%   ", odroid_settings_Volume_get() * 25);
      print(0, 0, s);
      debounce(ODROID_INPUT_VOLUME);
    }

    if (joystick.values[ODROID_INPUT_A]) {
      if (count != 0) { // not in an empty directory...
        odroid_settings_RomFilePath_set(target);
        odroid_system_application_set(emu_slot[e]); // set emulator slot
        esp_restart(); // reboot!
      }
      debounce(ODROID_INPUT_A);
    }
    if (joystick.values[ODROID_INPUT_B]) resume();
  }
}
