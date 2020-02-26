//{#pragma region Includes
  #include "includes/core.h"
  #include "includes/definitions.h"
  #include "includes/structures.h"
  #include "includes/declarations.h"
//}#pragma endregion Includes

//{#pragma region Odroid
  static odroid_gamepad_state gamepad;
  odroid_battery_state battery_state;
//}#pragma endregion Odroid

//{#pragma region Global
  bool SAVED = false;
  bool RESTART = false;
  bool LAUNCHER = false;
  bool FOLDER = false;
  bool SPLASH = true;
  bool SETTINGS = false;

  int8_t STEP = 0;
  int16_t SEEK[MAX_FILES];
  int OPTION = 0;
  int PREVIOUS = 0;
  int32_t VOLUME = 0;
  int32_t BRIGHTNESS = 0;
  const int32_t BRIGHTNESS_COUNT = 10;
  const int32_t BRIGHTNESS_LEVELS[10] = {10,20,30,40,50,60,70,80,90,100};
  int8_t USER;
  int8_t SETTING;
  int8_t COLOR;
  int8_t COVER;
  uint32_t currentDuty;

  char** FILES;
  char** FAVORITES;
  char FAVORITE[256] = "";

  char folder_path[256] = "";

  DIR *directory;
  struct dirent *file;
//}#pragma endregion Global

//{#pragma region Emulator and Directories
  char EMULATORS[COUNT][30] = {
    "SETTINGS",
    "FAVORITES",
    "NINTENDO ENTERTAINMENT SYSTEM",
    "NINTENDO GAME BOY",
    "NINTENDO GAME BOY COLOR",
    "SEGA MASTER SYSTEM",
    "SEGA GAME GEAR",
    "COLECOVISION",
    "SINCLAIR ZX SPECTRUM 48K",
    "ATARI 2600",
    "ATARI 7800",
    "ATARI LYNX",
    "PC ENGINE"
  };

  char DIRECTORIES[COUNT][10] = {
    "",
    "",
    "nes",      // 1
    "gb",       // 2
    "gbc",      // 2
    "sms",      // 3
    "gg",       // 3
    "col",      // 3
    "spectrum", // 4
    "a26",      // 5
    "a78",      // 6
    "lynx",       // 7
    "pce"       // 8
  };

  char EXTENSIONS[COUNT][10] = {
    "",
    "",
    "nes",      // 1
    "gb",       // 2
    "gbc",      // 2
    "sms",      // 3
    "gg",       // 3
    "col",      // 3
    "z80",      // 4
    "a26",      // 5
    "a78",      // 6
    "lnx",      // 7
    "pce",      // 8
  };

  int PROGRAMS[COUNT] = {1, 2, 2, 3, 3, 3, 4, 5, 6, 7, 8, 9};
  int LIMIT = 6;
//}#pragma endregion Emulator and Directories

//{#pragma region Buffer
  unsigned short buffer[40000];
//}#pragma endregion Buffer

/*
  APPLICATION
*/
//{#pragma region Main
  void app_main(void)
  {
    nvs_flash_init();
    odroid_system_init();

    // Audio
    odroid_audio_init(16000);
    /*
    #ifdef CONFIG_LCD_DRIVER_CHIP_RETRO_ESP32
        odroid_settings_Volume_set(4);
    #else
        odroid_settings_Volume_set(3);
    #endif
    */

    VOLUME = odroid_settings_Volume_get();
    odroid_settings_Volume_set(VOLUME);

    //odroid_settings_Backlight_set(BRIGHTNESS);

    // Display
    ili9341_init();
    BRIGHTNESS = get_brightness();
    apply_brightness();

    // Joystick
    odroid_input_gamepad_init();

    // Battery
    odroid_input_battery_level_init();

    // SD
    odroid_sdcard_open("/sd");
    create_favorites();

    // Theme
    get_theme();
    get_restore_states();

    // Toggle
    get_toggle();

    GUI = THEMES[USER];

    ili9341_prepare();
    ili9341_clear(0);

    //printf("==============\n%s\n==============\n", "RETRO ESP32");
    switch(esp_reset_reason()) {
      case ESP_RST_POWERON:
        RESTART = false;
        STEP = 1;
        ROMS.offset = 0;
      break;
      case ESP_RST_SW:
        RESTART = true;
      break;
      default:
        RESTART = false;
      break;
    }
    //STEP = 0;
    RESTART ? restart() : SPLASH ? splash() : NULL;
    draw_background();
    restore_layout();
    //xTaskCreate(launcher, "launcher", 8192, NULL, 5, NULL);
    launcher();
  }
//}#pragma endregion Main

/*
  METHODS
*/

//{#pragma region Helpers
  char *remove_ext (char* myStr, char extSep, char pathSep) {
      char *retStr, *lastExt, *lastPath;

      // Error checks and allocate string.

      if (myStr == NULL) return NULL;
      if ((retStr = malloc (strlen (myStr) + 1)) == NULL) return NULL;

      // Make a copy and find the relevant characters.

      strcpy (retStr, myStr);
      lastExt = strrchr (retStr, extSep);
      lastPath = (pathSep == 0) ? NULL : strrchr (retStr, pathSep);

      // If it has an extension separator.

      if (lastExt != NULL) {
          // and it's to the right of the path separator.

          if (lastPath != NULL) {
              if (lastPath < lastExt) {
                  // then remove it.

                  *lastExt = '\0';
              }
          } else {
              // Has extension separator with no path separator.

              *lastExt = '\0';
          }
      }

      // Return the modified string.

      return retStr;
  }

  char *get_filename (char* myStr) {
    int ext = '/';
    const char* extension = NULL;
    extension = strrchr(myStr, ext) + 1;

    return extension;
  }

  char *get_ext (char* myStr) {
    int ext = '.';
    const char* extension = NULL;
    extension = strrchr(myStr, ext) + 1;

    return extension;
  }
//}#pragma endregion Helpers

//{#pragma region Debounce
  void debounce(int key) {
    draw_battery();
    draw_speaker();
    draw_contrast();
    while (gamepad.values[key]) odroid_input_gamepad_read(&gamepad);
  }
//}#pragma endregion Debounce

//{#pragma region States
  void get_step_state() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    nvs_handle handle;
    err = nvs_open("storage", NVS_READWRITE, &handle);

    err = nvs_get_i8(handle, "STEP", &STEP);
    switch (err) {
      case ESP_OK:
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        STEP = 0;
        break;
      default :
        STEP = 0;
    }
    nvs_close(handle);
    //printf("\nGet nvs_get_i8:%d\n", STEP);
  }

  void set_step_state() {
    //printf("\nGet nvs_set_i8:%d\n", STEP);
    nvs_handle handle;
    nvs_open("storage", NVS_READWRITE, &handle);
    nvs_set_i8(handle, "STEP", STEP);
    nvs_commit(handle);
    nvs_close(handle);
  }

  void get_list_state() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    nvs_handle handle;
    err = nvs_open("storage", NVS_READWRITE, &handle);
    err = nvs_get_i16(handle, "LAST", &ROMS.offset);
    switch (err) {
      case ESP_OK:
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        ROMS.offset = 0;
        break;
      default :
        ROMS.offset = 0;
    }
    nvs_close(handle);
    //printf("\nGet nvs_get_i16:%d\n", ROMS.offset);
  }

  void set_list_state() {
    //printf("\nSet nvs_set_i16:%d", ROMS.offset);
    nvs_handle handle;
    nvs_open("storage", NVS_READWRITE, &handle);
    nvs_set_i16(handle, "LAST", ROMS.offset);
    nvs_commit(handle);
    nvs_close(handle);
    get_list_state();
  }

  void set_restore_states() {
    set_step_state();
    set_list_state();
  }

  void get_restore_states() {
    get_step_state();
    get_list_state();
  }
//}#pragma endregion States

//{#pragma region Text
  int get_letter(char letter) {
    int dx = 0;
    char upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!-'&?.,/()[] ";
    char lower[] = "abcdefghijklmnopqrstuvwxyz0123456789!-'&?.,/()[] ";
    for(int n = 0; n < strlen(upper); n++) {
      if(letter == upper[n] || letter == lower[n]) {
        return letter != ' ' ? n * 5 : 0;
        break;
      }
    }
    return dx;
  }

  void draw_text(short x, short y, char *string, bool ext, bool current, bool remove) {
    int length = !ext ? strlen(string) : strlen(string)-(strlen(EXTENSIONS[STEP])+1);
    if(length > 64){length = 64;}
    int rows = 7;
    int cols = 5;
    for(int n = 0; n < length; n++) {
      int dx = get_letter(string[n]);
      int i = 0;
      for(int r = 0; r < (rows); r++) {
        if(string[n] != ' ') {
          for(int c = dx; c < (dx+cols); c++) {
            buffer[i] = FONT_5x7[r][c] == 0 ? GUI.bg : current ? WHITE : GUI.fg;
            if(remove) {buffer[i] = GUI.bg;}
            i++;
          }
        }
      }
      if(string[n] != ' ') {
        ili9341_write_frame_rectangleLE(x, y-1, cols, rows, buffer);
      }
      x+= string[n] != ' ' ? 7 : 3;
    }
  }
//}#pragma endregion Text

//{#pragma region Mask
  void draw_mask(int x, int y, int w, int h){
    for (int i = 0; i < w * h; i++) buffer[i] = GUI.bg;
    ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
  }

  void draw_background() {
    int w = 320;
    int h = 60;
    for (int i = 0; i < 4; i++) draw_mask(0, i*h, w, h);
    draw_battery();
    draw_speaker();
    draw_contrast();
  }
//}#pragma endregion Mask

//{#pragma region Settings
  void draw_settings() {
    int x = ORIGIN.x;
    int y = POS.y + 46;

    draw_mask(x,y-1,100,17);
    draw_text(x,y,"THEMES",false, SETTING == 0 ? true : false, false);

    y+=20;
    draw_mask(x,y-1,100,17);
    draw_text(x,y,"COLORED ICONS",false, SETTING == 1 ? true : false, false);
    draw_toggle();

    y+=20;
    draw_mask(x,y-1,100,17);
    draw_text(x,y,"VOLUME",false, SETTING == 2 ? true : false, false);

    draw_volume();

    y+=20;
    draw_mask(x,y-1,100,17);
    draw_text(x,y,"BRIGHTNESS",false, SETTING == 3 ? true : false, false);

    draw_brightness();

    y+=20;
    draw_mask(x,y-1,100,17);
    draw_text(x,y,"COVER ART",false, SETTING == 4 ? true : false, false);

    draw_cover_toggle();

    /*
      BUILD
    */
    char message[100] = BUILD;
    int width = strlen(message)*5;
    int center = ceil((320)-(width))-48;
    y = 225;
    draw_text(center,y,message,false,false, false);
  }
//}#pragma endregion Settings

//{#pragma region Toggle
  void draw_toggle() {
    get_toggle();
    int x = SCREEN.w - 38;
    int y = POS.y + 66;
    int w, h;

    int i = 0;
    for(h = 0; h < 9; h++) {
      for(w = 0; w < 18; w++) {
        buffer[i] = toggle[h + (COLOR*9)][w] == 0 ? GUI.bg : toggle[h + (COLOR*9)][w];
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, 18, 9, buffer);
  }

  void set_toggle() {
    COLOR = COLOR == 0 ? 1 : 0;
    nvs_handle handle;
    nvs_open("storage", NVS_READWRITE, &handle);
    nvs_set_i8(handle, "COLOR", COLOR);
    nvs_commit(handle);
    nvs_close(handle);
  }

  void get_toggle() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    nvs_handle handle;
    err = nvs_open("storage", NVS_READWRITE, &handle);

    err = nvs_get_i8(handle, "COLOR", &COLOR);
    switch (err) {
      case ESP_OK:
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        COLOR = false;
        break;
      default :
        COLOR = false;
    }
    nvs_close(handle);
  }

  void draw_cover_toggle() {
    get_cover_toggle();
    int x = SCREEN.w - 38;
    int y = POS.y + 126;
    int w, h;

    int i = 0;
    for(h = 0; h < 9; h++) {
      for(w = 0; w < 18; w++) {
        buffer[i] = toggle[h + (COVER*9)][w] == 0 ? GUI.bg : toggle[h + (COVER*9)][w];
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, 18, 9, buffer);
  }

  void set_cover_toggle() {
    COVER = COVER == 0 ? 1 : 0;
    nvs_handle handle;
    nvs_open("storage", NVS_READWRITE, &handle);
    nvs_set_i8(handle, "COVER", COVER);
    nvs_commit(handle);
    nvs_close(handle);
  }

  void get_cover_toggle() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    nvs_handle handle;
    err = nvs_open("storage", NVS_READWRITE, &handle);

    err = nvs_get_i8(handle, "COVER", &COVER);
    switch (err) {
      case ESP_OK:
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        COVER = false;
        break;
      default :
        COVER = false;
    }
    nvs_close(handle);
  }
//}#pragma endregion Toggle

//{#pragma region Volume
  void draw_volume() {
    int32_t volume = get_volume();
    int x = SCREEN.w - 120;
    int y = POS.y + 86;
    //int w = 25 * volume;
    int w, h;

    int i = 0;
    for(h = 0; h < 7; h++) {
      for(w = 0; w < 100; w++) {
        buffer[i] = (w+h)%2 == 0 ? GUI.fg : GUI.bg;
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, 100, 7, buffer);

    if(volume > 0) {
      i = 0;
      for(h = 0; h < 7; h++) {
        for(w = 0; w < (25 * volume); w++) {
          if(SETTING == 2) {
            buffer[i] = WHITE;
          } else {
            buffer[i] = GUI.fg;
          }
          i++;
        }
      }
      ili9341_write_frame_rectangleLE(x, y, (25 * volume), 7, buffer);
    }

    draw_speaker();
  }
  int32_t get_volume() {
    return odroid_settings_Volume_get();
  }
  void set_volume() {
    odroid_settings_Volume_set(VOLUME);
    draw_volume();
  }
//}#pragma endregion Volume

//{#pragma region Brightness
  void draw_brightness() {
    int x = SCREEN.w - 120;
    int y = POS.y + 106;
    int w, h;

    int i = 0;
    for(h = 0; h < 7; h++) {
      for(w = 0; w < 100; w++) {
        buffer[i] = (w+h)%2 == 0 ? GUI.fg : GUI.bg;
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, 100, 7, buffer);

    //if(BRIGHTNESS > 0) {
      i = 0;
      for(h = 0; h < 7; h++) {
        for(w = 0; w < (BRIGHTNESS_COUNT * BRIGHTNESS)+BRIGHTNESS+1; w++) {
          if(SETTING == 3) {
            buffer[i] = WHITE;
          } else {
            buffer[i] = GUI.fg;
          }
          i++;
        }
      }
      ili9341_write_frame_rectangleLE(x, y, (BRIGHTNESS_COUNT * BRIGHTNESS)+BRIGHTNESS+1, 7, buffer);
    //}
    draw_contrast();
  }
  int32_t get_brightness() {
    return odroid_settings_Backlight_get();
  }
  void set_brightness() {
    odroid_settings_Backlight_set(BRIGHTNESS);
    draw_brightness();
    apply_brightness();
  }
  void apply_brightness() {
    const int DUTY_MAX = 0x1fff;
    //BRIGHTNESS = get_brightness();
    int duty = DUTY_MAX * (BRIGHTNESS_LEVELS[BRIGHTNESS] * 0.01f);

    if(is_backlight_initialized()) {
      currentDuty = ledc_get_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
      if (currentDuty != duty) {
        //ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, currentDuty);
        //ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        //ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty, 1000);
        //ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_WAIT_DONE /*LEDC_FADE_NO_WAIT|LEDC_FADE_WAIT_DONE|LEDC_FADE_MAX*/);
        //ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
        //ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        ledc_set_fade_time_and_start(
          LEDC_LOW_SPEED_MODE,
          LEDC_CHANNEL_0,
          duty,
            25,
          LEDC_FADE_WAIT_DONE
        );
      }
    }
  }
//}#pragma endregion Brightness

//{#pragma region Theme
  void draw_themes() {
    int x = ORIGIN.x;
    int y = POS.y + 46;
    int filled = 0;
    int count = 22;
    for(int n = USER; n < count; n++){
      if(filled < ROMS.limit) {
        draw_mask(x,y-1,100,17);
        draw_text(x,y,THEMES[n].name,false, n == USER ? true : false, false);
        y+=20;
        filled++;
      }
    }
    int slots = (ROMS.limit - filled);
    for(int n = 0; n < slots; n++) {
      draw_mask(x,y-1,100,17);
      draw_text(x,y,THEMES[n].name,false,false, false);
      y+=20;
    }
  }

  void get_theme() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    nvs_handle handle;
    err = nvs_open("storage", NVS_READWRITE, &handle);

    err = nvs_get_i8(handle, "USER", &USER);
    switch (err) {
      case ESP_OK:
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        USER = 20;
        set_theme(USER);
        break;
      default :
        USER = 20;
        set_theme(USER);
    }
    nvs_close(handle);
  }

  void set_theme(int8_t i) {
    nvs_handle handle;
    nvs_open("storage", NVS_READWRITE, &handle);
    nvs_set_i8(handle, "USER", i);
    nvs_commit(handle);
    nvs_close(handle);
    get_theme();
  }

  void update_theme() {
    GUI = THEMES[USER];
    set_theme(USER);
    draw_background();
    draw_mask(0,0,320,64);
    draw_systems();
    draw_text(16,16,EMULATORS[STEP], false, true, false);
    draw_themes();
  }
//}#pragma endregion Theme

//{#pragma region GUI
  void draw_systems() {
    for(int e = 0; e < COUNT; e++) {
      int i = 0;
      int x = SYSTEMS[e].x;
      int y = POS.y;
      int w = 32;
      int h = 32;
      if(x > 0 && x < 288) {
        for(int r = 0; r < 32; r++) {
          for(int c = 0; c < 32; c++) {
            switch(COLOR) {
              case 0:
                buffer[i] = SYSTEMS[e].system[r][c] == WHITE ? WHITE : GUI.bg;
              break;
              case 1:
                //buffer[i] = SYSTEMS[e].system[r][c] == WHITE ? WHITE : GUI.bg;
                buffer[i] = SYSTEMS[e].color[r][c] == 0 ? GUI.bg : SYSTEMS[e].color[r][c];
              break;
            }
            i++;
          }
        }
        ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
      }
    }
  }

  void draw_folder(int x, int y, bool current) {
    int i = 0;
    for(int h = 0; h < 16; h++) {
      for(int w = 0; w < 16; w++) {
        buffer[i] = folder[h][w] == WHITE ? current ? WHITE : GUI.fg : GUI.bg;
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, 16, 16, buffer);
  }

  void draw_media(int x, int y, bool current, int offset) {
    if(offset == -1) {offset = (STEP-2) * 16;}
    int i = 0;
    for(int h = 0; h < 16; h++) {
      for(int w = offset; w < (offset+16); w++) {
        switch(COLOR) {
          case 0:
            buffer[i] = media[h][w] == WHITE ? current ? WHITE : GUI.fg : GUI.bg;
          break;
          case 1:
            buffer[i] = media_color[h][w] == 0 ? GUI.bg : media_color[h][w];
            if(current) {
              buffer[i] = media_color[h+16][w] == 0 ? GUI.bg : media_color[h+16][w];
            }
          break;
        }
        /*

        */
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, 16, 16, buffer);
  }

  void draw_battery() {
    #ifdef BATTERY
      odroid_input_battery_level_read(&battery_state);

      int i = 0;
      int x = SCREEN.w - 32;
      int y = 8;
      int h = 0;
      int w = 0;

      draw_mask(x,y,16,16);
      for(h = 0; h < 16; h++) {
        for(w = 0; w < 16; w++) {
          buffer[i] = battery[h][w] == WHITE ? WHITE : GUI.bg;
          i++;
        }
      }
      ili9341_write_frame_rectangleLE(x, y, 16, 16, buffer);

      int percentage = battery_state.percentage/10;
      x += 2;
      y += 6;
      w = percentage > 0 ? percentage > 10 ? 10 : percentage : 10;
      h = 4;
      i = 0;

      //printf("\nbattery_state.percentage:%d\n(percentage):%d\n(millivolts)%d\n", battery_state.percentage, percentage, battery_state.millivolts);

      int color[11] = {24576,24576,64288,64288,65504,65504,65504,26592,26592,26592,26592};

      int fill = color[w];
      for(int c = 0; c < h; c++) {
        for(int n = 0; n <= w; n++) {
          buffer[i] = fill;
          i++;
        }
      }
      ili9341_write_frame_rectangleLE(x, y, w, h, buffer);

      /*
      if(battery_state.millivolts > 4200) {
        i = 0;
        for(h = 0; h < 5; h++) {
          for(w = 0; w < 3; w++) {
            buffer[i] = charge[h][w] == WHITE ? WHITE : fill;
            i++;
          }
        }
        ili9341_write_frame_rectangleLE(x+4, y, 3, 5, buffer);
      }
      */
    #endif
  }

  void draw_speaker() {
    int32_t volume = get_volume();

    int i = 0;
    int x = SCREEN.w - 52;
    int y = 8;
    int h = 16;
    int w = 16;

    draw_mask(x,y,16,16);

    int dh = 64 - (volume*16);
    for(h = 0; h < 16; h++) {
      for(w = 0; w < 16; w++) {
        buffer[i] = speaker[dh+h][w] == WHITE ? WHITE : GUI.bg;
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
  }

  void draw_contrast() {
    int32_t dy = 0;
    switch(BRIGHTNESS) {
      case 10:
      case 9:
      case 8:
        dy = 0;
      break;
      case 7:
      case 6:
      case 5:
        dy = 16;
      break;
      case 4:
      case 3:
      case 2:
        dy = 32;
      break;
      case 1:
      case 0:
        dy = 48;
      break;
    }
    int i = 0;
    int x = SCREEN.w - 72;
    int y = 8;
    int h = 16;
    int w = 16;

    draw_mask(x,y,16,16);

    for(h = 0; h < 16; h++) {
      for(w = 0; w < 16; w++) {
        buffer[i] = brightness[dy+h][w] == WHITE ? WHITE : GUI.bg;
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
  }

  void draw_numbers() {
    int x = 296;
    int y = POS.y + 48;
    int w = 0;
    char count[10];
    sprintf(count, "(%d/%d)", (ROMS.offset+1), ROMS.total);
    w = strlen(count)*5;
    x -= w;
    draw_text(x,y,count,false,false, false);
  }

  void delete_numbers() {
    int x = 296;
    int y = POS.y + 48;
    int w = 0;
    char count[10];
    sprintf(count, "(%d/%d)", (ROMS.offset+1), ROMS.total);
    w = strlen(count)*5;
    x -= w;
    draw_text(x,y,count,false,false, true);
  }

  void draw_launcher() {
    draw_background();
    draw_text(16,16,EMULATORS[STEP], false, true, false);
    int i = 0;
    int x = GAP/3;
    int y = POS.y;
    int w = 32;
    int h = 32;
    for(int r = 0; r < 32; r++) {
      for(int c = 0; c < 32; c++) {
        switch(COLOR) {
          case 0:
            buffer[i] = SYSTEMS[STEP].system[r][c] == WHITE ? WHITE : GUI.bg;
          break;
          case 1:
            //buffer[i] = SYSTEMS[e].system[r][c] == WHITE ? WHITE : GUI.bg;
            buffer[i] = SYSTEMS[STEP].color[r][c] == 0 ? GUI.bg : SYSTEMS[STEP].color[r][c];
          break;
        }
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, w, h, buffer);

    y += 48;
    draw_media(x,y-6,true,-1);
    draw_launcher_options();
    get_cover_toggle();
    if(COVER == 1){get_cover();}
  }

  void draw_launcher_options() {
    has_save_file(ROM.name);

    char favorite[256] = "";
    sprintf(favorite, "%s/%s", ROM.path, ROM.name);
    is_favorite(favorite);
    int x = GAP/3 + 32;
    int y = POS.y + 48;
    int w = 5;
    int h = 5;
    int i = 0;
    int offset = 0;
    if(SAVED) {
      // resume
      i = 0;
      offset = 5;
      for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
        buffer[i] = icons[r+offset][c] == WHITE ? OPTION == 0 ? WHITE : GUI.fg : GUI.bg;i++;
      }}
      ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
      draw_text(x+10,y,"Resume",false,OPTION == 0?true:false, false);
      // restart
      i = 0;
      y+=20;
      offset = 10;
      for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
        buffer[i] = icons[r+offset][c] == WHITE ? OPTION == 1 ? WHITE : GUI.fg : GUI.bg;i++;
      }}
      ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
      draw_text(x+10,y,"Restart",false,OPTION == 1?true:false, false);
      // delete
      i = 0;
      y+=20;
      offset = 20;
      for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
        buffer[i] = icons[r+offset][c] == WHITE ? OPTION == 2 ? WHITE : GUI.fg : GUI.bg;i++;
      }}
      ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
      draw_text(x+10,y,"Delete Save",false,OPTION == 2?true:false, false);
    } else {
      // run
      i = 0;
      offset = 0;
      for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
        buffer[i] = icons[r+offset][c] == WHITE ? OPTION == 0 ? WHITE : GUI.fg : GUI.bg;i++;
      }}
      ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
      draw_text(x+10,y,"Run",false,OPTION == 0?true:false, false);
    }

    // favorites
    y+=20;
    i = 0;
    offset = ROM.favorite?40:35;
    int option = SAVED ? 3 : 1;
    draw_mask(x,y-1,80,9);
    for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
      buffer[i] = icons[r+offset][c] == WHITE ? OPTION == option ? WHITE : GUI.fg : GUI.bg;i++;
    }}
    ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
    draw_text(x+10,y,ROM.favorite?"Unfavorite":"Favorite",false,OPTION == option?true:false, false);
  }
//}#pragma endregion GUI

//{#pragma region Files
  //{#pragma region Sort
    inline static void swap(char** a, char** b) {
        char* t = *a;
        *a = *b;
        *b = t;
    }

    static int strcicmp(char const *a, char const *b) {
        for (;; a++, b++)
        {
            int d = tolower((int)*a) - tolower((int)*b);
            if (d != 0 || !*a) return d;
        }
    }

    static int partition (char** arr, int low, int high) {
        char* pivot = arr[high];
        int i = (low - 1);

        for (int j = low; j <= high- 1; j++)
        {
            if (strcicmp(arr[j], pivot) < 0)
            {
                i++;
                swap(&arr[i], &arr[j]);
            }
        }
        swap(&arr[i + 1], &arr[high]);
        return (i + 1);
    }

    void quick_sort(char** arr, int low, int high) {
        if (low < high)
        {
            int pi = partition(arr, low, high);

            quick_sort(arr, low, pi - 1);
            quick_sort(arr, pi + 1, high);
        }
    }

    void sort_files(char** files)
    {
        if (ROMS.total > 1)
        {
            quick_sort(files, 0, ROMS.total - 1);
        }
    }
  //}#pragma endregion Sort

  void count_files() {
    delete_numbers();
    SEEK[0] = 0;

    printf("\n----- %s -----", __func__);

    ROMS.total = 0;
    char message[100];
    sprintf(message, "searching %s roms", DIRECTORIES[STEP]);
    int center = ceil((320/2)-((strlen(message)*5)/2));
    draw_text(center,134,message,false,false, false);

    char path[256] = "/sd/roms/";
    strcat(&path[strlen(path) - 1], DIRECTORIES[STEP]);
    strcat(&path[strlen(path) - 1],folder_path);
    strcpy(ROM.path, path);

    printf("\npath:%s", path);

    if(directory != NULL) {
     // printf("\npath:%s", path);
      free(directory);
      closedir(directory);
    }

    directory = opendir(path);
    if(!directory) {
      draw_mask(0,132,320,10);
      sprintf(message, "unable to open %s directory", DIRECTORIES[STEP]);
      int center = ceil((320/2)-((strlen(message)*5)/2));
      draw_text(center,134,message,false,false, false);
      return NULL;
    } else {
      if(directory == NULL) {
        draw_mask(0,132,320,10);
        sprintf(message, "%s directory not found", DIRECTORIES[STEP]);
        int center = ceil((320/2)-((strlen(message)*5)/2));
        draw_text(center,134,message,false,false, false);
      } else {
        rewinddir(directory);
        seekdir(directory, 0);
        SEEK[0] = 0;
        struct dirent *file;
       // printf("\n");
        while ((file = readdir(directory)) != NULL) {
          int rom_length = strlen(file->d_name);
          int ext_length = strlen(EXTENSIONS[STEP]);
          bool extenstion = strcmp(&file->d_name[rom_length - ext_length], EXTENSIONS[STEP]) == 0 && file->d_name[0] != '.';
          if(extenstion || (file->d_type == 2)) {
            SEEK[ROMS.total+1] = telldir(directory);
            ROMS.total++;
          }
        }
        free(file);
       // printf("\nnumber of files:\t%d", ROMS.total);
       // printf("\nfree space:0x%x (%#08x)", esp_get_free_heap_size(), heap_caps_get_free_size(MALLOC_CAP_DMA));
      }
      //free(directory);
      //closedir(directory);
     // printf("\n%s: freed & closed", path);
      for(int n = 0; n < ROMS.total; n++) {
       // printf("\nSEEK[%d]:%d ", n, SEEK[n]);
      }
    }

   // printf("\n---------------------\n");
  }

  void seek_files() {
    delete_numbers();
   // printf("\n----- %s -----", __func__);
   // printf("\nROMS.offset:%d", ROMS.offset);
   // printf("\nROMS.limit:%d", ROMS.limit);
   // printf("\nROMS.total:%d", ROMS.total);
   // printf("\nROMS.page:%d", ROMS.page);
   // printf("\nROMS.pages:%d", ROMS.pages);

    char message[100];

    char path[256] = "/sd/roms/";
    strcat(&path[strlen(path) - 1], DIRECTORIES[STEP]);
    strcat(&path[strlen(path) - 1],folder_path);
    strcpy(ROM.path, path);

    free(FILES);
    FILES = (char**)malloc(ROMS.limit * sizeof(void*));

    //DIR *directory = opendir(path);
    if(!directory) {
      draw_mask(0,132,320,10);
      sprintf(message, "unable to open %s directory", DIRECTORIES[STEP]);
      int center = ceil((320/2)-((strlen(message)*5)/2));
      draw_text(center,134,message,false,false, false);
      return NULL;
    } else {
      if(directory == NULL) {
        draw_mask(0,132,320,10);
        sprintf(message, "%s directory not found", DIRECTORIES[STEP]);
        int center = ceil((320/2)-((strlen(message)*5)/2));
        draw_text(center,134,message,false,false, false);
      } else {
       // printf("\nSEEK[%d]:%d", ROMS.offset, SEEK[ROMS.offset]);
        rewinddir(directory);
        seekdir(directory, SEEK[ROMS.offset]);
        struct dirent *file;
        int n =0;
        for(;;) {
          if(n == ROMS.limit || (ROMS.offset + n) == ROMS.total){break;}
          if(!(file = readdir(directory))) {
            break;
          };

          int rom_length = strlen(file->d_name);
          int ext_length = strlen(EXTENSIONS[STEP]);
          bool extenstion = strcmp(&file->d_name[rom_length - ext_length], EXTENSIONS[STEP]) == 0 && file->d_name[0] != '.';
          if(extenstion || (file->d_type == 2)) {
            size_t len = strlen(file->d_name);
            FILES[n] = (file->d_type == 2) ? (char*)malloc(len + 5) : (char*)malloc(len + 1);
            if((file->d_type == 2)) {
              char dir[256];
              strcpy(dir, file->d_name);
              char dd[8];
              sprintf(dd, "%s", ext_length == 2 ? "dir" : ".dir");
              strcat(&dir[strlen(dir) - 1], dd);
              strcpy(FILES[n], dir);
            } else {
              strcpy(FILES[n], file->d_name);
            }
            n++;
          }
        }
      }
    }
   // printf("\n---------------------\n");
    ROMS.pages = ROMS.total/ROMS.limit;
    if(ROMS.offset > ROMS.total) { ROMS.offset = 0;}
    if(ROMS.total != 0) {
      draw_files();
    } else {
      sprintf(message, "no %s roms available", DIRECTORIES[STEP]);
      int center = ceil((320/2)-((strlen(message)*5)/2));
      draw_mask(0,POS.y + 47,320,10);
      draw_mask(0,132,320,10);
      draw_text(center,134,message,false,false, false);
    }
  }

  void get_files() {
    delete_numbers();
    count_files();
    seek_files();
  }

  void draw_files() {
    //printf("\n----- %s -----", __func__);
    int x = ORIGIN.x;
    int y = POS.y + 48;
    ROMS.page = ROMS.offset/ROMS.limit;

    for (int i = 0; i < 4; i++) draw_mask(0, y+(i*40)-6, 320, 40);
    //int limit = ROMS.total < ROMS.limit ? ROMS.total : ROMS.limit;
    int limit = (ROMS.total - ROMS.offset) <  ROMS.limit ?
      (ROMS.total - ROMS.offset) :
      ROMS.limit;

    //printf("\nlimit:%d", limit);
    for(int n = 0; n < limit; n++) {
      //printf("\n%d:%s", n, FILES[n]);
      draw_text(x+24,y,FILES[n],true,n == 0 ? true : false, false);
      bool directory = strcmp(&FILES[n][strlen(FILES[n]) - 3], "dir") == 0;
      directory ?
        draw_folder(x,y-6,n == 0 ? true : false) :
        draw_media(x,y-6,n == 0 ? true : false,-1);
      if(n == 0) {
        strcpy(ROM.name, FILES[n]);
        strcpy(ROM.art, remove_ext(ROM.name, '.', '/'));
        ROM.ready = true;
      }
      y+=20;
    }
    draw_numbers();
    //printf("\n---------------------\n");
  }

  void has_save_file(char *save_name) {
    SAVED = false;

    //printf("\n----- %s -----", __func__);
    //printf("\nsave_name: %s", save_name);

    char save_dir[256] = "/sd/odroid/data/";
    strcat(&save_dir[strlen(save_dir) - 1], DIRECTORIES[STEP]);
    //printf("\nsave_dir: %s", save_dir);

    char save_file[256] = "";
    sprintf(save_file, "%s/%s", save_dir, save_name);
    strcat(&save_file[strlen(save_file) - 1], ".sav");
    //printf("\nsave_file: %s", save_file);

    DIR *directory = opendir(save_dir);
    if(directory == NULL) {
      perror("opendir() error");
    } else {
      gets(save_file);
      struct dirent *file;
      while ((file = readdir(directory)) != NULL) {
        char tmp[256] = "";
        strcat(tmp, file->d_name);
        tmp[strlen(tmp)-4] = '\0';
        gets(tmp);
        //printf("\ntmp:%s save_name:%s", tmp, save_name);
        if(strcmp(save_name, tmp) == 0) {
          SAVED = true;
        }
      }
    }
    //printf("\n---------------------\n");
  }
//}#pragma endregion Files

//{#pragma region Favorites

  void create_favorites() {
    printf("\n----- %s START -----", __func__);
    char file[256] = "/sd/odroid/data";
    sprintf(file, "%s/%s", file, FAVFILE);

    //struct stat st; if (stat(file, &st) == 0) {unlink(file);}

    FILE *f;
    f = fopen(file, "rb");
    if(f == NULL) {
      f = fopen(file, "w+");
      printf("\nCREATING: %s", file);
    } else {
      read_favorites();
    }
    printf("\nCLOSING: %s", file);
    fclose(f);
    printf("\n----- %s END -----\n", __func__);
  }

  void read_favorites() {
    printf("\n----- %s START -----", __func__);

    int n = 0;
    ROMS.total = 0;

    free(FAVORITES);
    FAVORITES = (char**)malloc((50) * sizeof(void*));

    char file[256] = "/sd/odroid/data";
    sprintf(file, "%s/%s", file, FAVFILE);

    FILE *f;
    f = fopen(file, "rb");
    if(f) {
      printf("\nREADING: %s\n", file);
      char line[256];
      while (fgets(line, sizeof(line), f)) {
        char *ep = &line[strlen(line)-1];
        while (*ep == '\n' || *ep == '\r'){*ep-- = '\0';}
        printf("\n%s", line);
        size_t len = strlen(line);
        FAVORITES[n] = (char*)malloc(len + 1);
        strcpy(FAVORITES[n], line);
        n++;
        ROMS.total++;
      }
    }
    fclose(f);

    printf("\n----- %s END -----\n", __func__);
  }

  void get_favorites() {
    printf("\n----- %s START -----", __func__);
    char message[100];
    sprintf(message, "loading favorites");
    int center = ceil((320/2)-((strlen(message)*5)/2));
    draw_text(center,134,message,false,false, false);

    read_favorites();
    process_favorites();

    printf("\n----- %s END -----", __func__);
  }

  void process_favorites() {
    printf("\n----- %s START -----", __func__);

    char message[100];
    delete_numbers();

    ROMS.pages = ROMS.total/ROMS.limit;
    if(ROMS.offset > ROMS.total) { ROMS.offset = 0;}
    if(ROMS.total != 0) {
      draw_favorites();
    } else {
      sprintf(message, "no favorites available");
      int center = ceil((320/2)-((strlen(message)*5)/2));
      draw_mask(0,POS.y + 47,320,10);
      draw_mask(0,132,320,10);
      draw_text(center,134,message,false,false, false);
    }

    printf("\n----- %s END -----", __func__);
  }

  void draw_favorites() {
    printf("\n----- %s START -----", __func__);
    int x = ORIGIN.x;
    int y = POS.y + 48;
    ROMS.page = ROMS.offset/ROMS.limit;

    for (int i = 0; i < 4; i++) draw_mask(0, y+(i*40)-6, 320, 40);
    int limit = (ROMS.total - ROMS.offset) <  ROMS.limit ?
      (ROMS.total - ROMS.offset) :
      ROMS.limit;

    //printf("\nlimit:%d", limit);
    for(int n = 0; n < limit; n++) {
      char full[512];
      char trimmed[512];
      char favorite[256];
      char extension[10];
      char path[256];

      strcpy(full, FAVORITES[n]);
      strcpy(trimmed, remove_ext(full, '.', '/'));
      strcpy(favorite, get_filename(trimmed));
      strcpy(extension, get_ext(full));

      int length = (strlen(trimmed) - strlen(favorite)) - 1;
      memset(path, '\0', 256);
      strncpy(path, full, length);

      int offset = -1;
      if(strcmp(extension, "nes") == 0) {offset = 0*16;}
      if(strcmp(extension, "gb") == 0) {offset = 1*16;}
      if(strcmp(extension, "gbc") == 0) {offset = 2*16;}
      if(strcmp(extension, "sms") == 0) {offset = 3*16;}
      if(strcmp(extension, "gg") == 0) {offset = 4*16;}
      if(strcmp(extension, "col") == 0) {offset = 5*16;}
      if(strcmp(extension, "z80") == 0) {offset = 6*16;}
      if(strcmp(extension, "a26") == 0) {offset = 7*16;}
      if(strcmp(extension, "a78") == 0) {offset = 8*16;}
      if(strcmp(extension, "lnx") == 0) {offset = 9*16;}
      if(strcmp(extension, "pce") == 0) {offset = 10*16;}

      /*
      printf("\n\nentry %d:"
        "\n- full ->\t%s"
        "\n- trimmed ->\t%s"
        "\n- favorite ->\t%s"
        "\n- extension ->\t%s"
        "\n- path ->\t%s"
        "\n- length ->\t%d"
        "\n- offset ->\t%d",
        n, full, trimmed, favorite, extension, path, length, offset);
      */


      draw_text(x+24,y,favorite,false,n == 0 ? true : false, false);
      draw_media(x,y-6,n == 0 ? true : false, offset);
      if(n == 0) {
        sprintf(favorite, "%s.%s", favorite, extension);
        strcpy(ROM.name, favorite);
        strcpy(ROM.art, remove_ext(ROM.name, '.', '/'));
        strcpy(ROM.path, path);
        ROM.ready = true;
      }
      y+=20;
    }
    draw_numbers();


    printf("\n\n***********"
      "\nROM details"
      "\n- ROM.name ->\t%s"
      "\n- ROM.art ->\t%s"
      "\n- ROM.path ->\t%s"
      "\n- ROM.ready ->\t%d"
      "\n***********\n\n",
      ROM.name, ROM.art, ROM.path, ROM.ready);
    printf("\n----- %s END -----", __func__);
  }

  void add_favorite(char *favorite) {
    printf("\n----- %s START -----", __func__);
    char file[256] = "/sd/odroid/data";
    sprintf(file, "%s/%s", file, FAVFILE);
    FILE *f;
    f = fopen(file, "a+");
    if(f) {
      printf("\nADDING: %s to %s", favorite, file);
      fprintf(f, "%s\n", favorite);
    }
    fclose(f);
    printf("\n----- %s END -----\n", __func__);
  }

  void delete_favorite(char *favorite) {
    printf("\n----- %s START -----", __func__);

    int n = 0;
    int count = 0;

    free(FAVORITES);
    FAVORITES = (char**)malloc(50 * sizeof(void*));

    char file[256] = "/sd/odroid/data";
    sprintf(file, "%s/%s", file, FAVFILE);

    FILE *f;
    f = fopen(file, "rb");
    if(f) {
      printf("\nCHECKING: %s\n", favorite);
      char line[256];
      while (fgets(line, sizeof(line), f)) {
        char *ep = &line[strlen(line)-1];
        while (*ep == '\n' || *ep == '\r'){*ep-- = '\0';}
        if(strcmp(favorite, line) != 0) {
          size_t len = strlen(line);
          FAVORITES[n] = (char*)malloc(len + 1);
          strcpy(FAVORITES[n], line);
          n++;
          count++;
        }
      }
    }
    fclose(f);
    struct stat st;
    if (stat(file, &st) == 0) {
      unlink(file);
      create_favorites();
      for(n = 0; n < count; n++) {
        size_t len = strlen(FAVORITES[n]);
        if(len > 0) {
          add_favorite(FAVORITES[n]);
          printf("\n%s - %d" , FAVORITES[n], len);
        }
      }
    } else {
      printf("\nUNABLE TO UNLINK\n");
    }

    printf("\n----- %s END -----\n", __func__);
  }

  void is_favorite(char *favorite) {
    printf("\n----- %s START -----", __func__);
    ROM.favorite = false;

    char file[256] = "/sd/odroid/data";
    sprintf(file, "%s/%s", file, FAVFILE);


    FILE *f;
    f = fopen(file, "rb");
    if(f) {
      printf("\nCHECKING: %s\n", favorite);
      char line[256];
      while (fgets(line, sizeof(line), f)) {
        char *ep = &line[strlen(line)-1];
        while (*ep == '\n' || *ep == '\r'){*ep-- = '\0';}
        if(strcmp(favorite, line) == 0) {
          ROM.favorite = true;
        }
      }
    }
    fclose(f);
    printf("\n----- %s END -----\n", __func__);
  }
//}#pragma endregion Favorites

//{#pragma region Cover
  void get_cover() {
    preview_cover(false);
  }

  void preview_cover(bool error) {
    ROM.crc = 0;

    int bw = 112;
    int bh = 150;
    int i = 0;

    char file[256] = "/sd/romart";

    sprintf(file, "%s/%s/%s.art", file, DIRECTORIES[STEP], ROM.art);

    if(!error) {
      FILE *f = fopen(file, "rb");
      if(f) {
        uint16_t width, height;
        fread(&width, 2, 1, f);
        fread(&height, 2, 1, f);
        bw = width;
        bh = height;
        ROM.crc = 1;
        fclose(f);
      } else {
        error = true;
      }
    }

    printf("\n----- %s -----\n%s\n", __func__, file);
    for(int h = 0; h < bh; h++) {
      for(int w = 0; w < bw; w++) {
        buffer[i] = (h == 0) || (h == bh -1) ? WHITE : (w == 0) ||  (w == bw -1) ? WHITE : GUI.bg;
        i++;
      }
    }
    int x = SCREEN.w-24-bw;
    int y = POS.y+8;
    ili9341_write_frame_rectangleLE(x, y, bw, bh, buffer);

    int center = x + bw/2;
    center -= error ? 30 : 22;

    draw_text(center, y + (bh/2) - 3, error ? "NO PREVIEW" : "PREVIEW", false, false, false);

    if(ROM.crc == 1) {
      usleep(20000);
      draw_cover();
    }
  }

  void draw_cover() {
    printf("\n----- %s -----\n%s\n", __func__, "OPENNING");
    char file[256] = "/sd/romart";
    sprintf(file, "%s/%s/%s.art", file, DIRECTORIES[STEP], ROM.art);

    FILE *f = fopen(file, "rb");
    if(f) {
      printf("\n----- %s -----\n%s\n", __func__, "OPEN");
      uint16_t width, height;
      fread(&width, 2, 1, f);
      fread(&height, 2, 1, f);

      int x = SCREEN.w-24-width;
      int y = POS.y+8;

      if (width<=320 && height<=240) {
        uint16_t *img = (uint16_t*)heap_caps_malloc(width*height*2, MALLOC_CAP_SPIRAM);
        fread(img, 2, width*height, f);
        ili9341_write_frame_rectangleLE(x,y, width, height, img);
        heap_caps_free(img);
      } else {
        printf("\n----- %s -----\n%s\nwidth:%d height:%d\n", __func__, "ERROR", width, height);
        preview_cover(true);
      }

      fclose(f);
    }
  }
//}#pragma endregion Cover

//{#pragma region Animations
  void animate(int dir) {
    delete_numbers();
    draw_mask(0,0,SCREEN.w - 56,32);
    draw_text(16,16,EMULATORS[STEP], false, true, false);
    draw_contrast();

    int y = POS.y + 46;
    for (int i = 0; i < 4; i++) draw_mask(0, y+(i*40)-6, 320, 40);
    int sx[4][13] = {
      {8,8,4,4,4,3,3,3,3,2,2,2,2}, // 48
      {30,26,20,20,18,18,16,16,12,12,8,8,4} // 208 30+26+20+20+18+18+16+16+12+12+8+8+4
    };
    for(int i = 0; i < 13; i++) {
      if(dir == -1) {
        // LEFT
        for(int e = 0; e < COUNT; e++) {
          SYSTEMS[e].x += STEP != COUNT - 1 ?
            STEP == (e-1) ? sx[1][i] : sx[0][i] :
            e == 0 ? sx[1][i] : sx[0][i] ;
        }
      } else {
        // RIGHT
        for(int e = 0; e < COUNT; e++) {
          SYSTEMS[e].x -= STEP == e ? sx[1][i] : sx[0][i];
        }
      }
      draw_mask(0,32,320,32);
      draw_systems();
      usleep(20000);
    }
    STEP == 0 ? draw_settings() : STEP == 1 ? get_favorites() : get_files();
    clean_up();
  }

  void restore_layout() {

    SYSTEMS[0].x = GAP/3;
    for(int n = 1; n < COUNT; n++) {
      if(n == 1) {
        SYSTEMS[n].x = GAP/3+NEXT;
      } else if(n == 2) {
        SYSTEMS[n].x = GAP/3+NEXT+(GAP);
      } else {
        SYSTEMS[n].x = GAP/3+NEXT+(GAP*(n-1));
      }
    };

    draw_background();

    for(int n = 0; n < COUNT; n++) {
      int delta = (n-STEP);
      if(delta < 0) {
        SYSTEMS[n].x = (GAP/3) + (GAP * delta);
      } else if(delta == 0) {
        SYSTEMS[n].x = (GAP/3);
      } else if(delta == 1) {
        SYSTEMS[n].x = GAP/3+NEXT;
      } else if(delta == 2) {
        SYSTEMS[n].x = GAP/3+NEXT+(GAP);
      } else {
        SYSTEMS[n].x = GAP/3+NEXT+(GAP*(delta-1));
      }
    }

    clean_up();
    draw_systems();
    draw_text(16,16,EMULATORS[STEP],false,true, false);
    STEP == 0 ? draw_settings() : STEP == 1 ? get_favorites() : get_files();
  }

  void clean_up() {
    int MAX = 784;
    for(int n = 0; n < COUNT; n++) {
      if(SYSTEMS[n].x > 560) {
        SYSTEMS[n].x -= MAX;
      }
      if(SYSTEMS[n].x <= -320) {
        SYSTEMS[n].x += MAX;
      }
    }
  }
//}#pragma endregion Animations

//{#pragma region Boot Screens
  void splash() {
    draw_background();
    int w = 128;
    int h = 18;
    int x = (SCREEN.w/2)-(w/2);
    int y = (SCREEN.h/2)-(h/2);
    int i = 0;
    for(int r = 0; r < h; r++) {
      for(int c = 0; c < w; c++) {
        buffer[i] = logo[r][c] == 0 ? GUI.bg : GUI.fg;
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, w, h, buffer);

    /*
      BUILD
    */
    char message[100] = BUILD;
    int width = strlen(message)*5;
    int center = ceil((320)-(width))-48;
    y = 225;
    draw_text(center,y,message,false,false, false);

    sleep(2);
    draw_background();
  }

  void boot() {
    draw_background();
    char message[100] = "retro esp32";
    int width = strlen(message)*5;
    int center = ceil((320/2)-(width/2));
    int y = 118;
    draw_text(center,y,message,false,false, false);

    y+=10;
    for(int n = 0; n < (width+10); n++) {
      for(int i = 0; i < 5; i++) {
        buffer[i] = GUI.fg;
      }
      ili9341_write_frame_rectangleLE(center+n, y, 1, 5, buffer);
      usleep(10000);
    }
  }

  void restart() {
    draw_background();

    char message[100] = "restarting";
    int h = 5;
    int w = strlen(message)*h;
    int x = (SCREEN.w/2)-(w/2);
    int y = (SCREEN.h/2)-(h/2);
    draw_text(x,y,message,false,false, false);

    y+=10;
    for(int n = 0; n < (w+10); n++) {
      for(int i = 0; i < 5; i++) {
        buffer[i] = GUI.fg;
      }
      ili9341_write_frame_rectangleLE(x+n, y, 1, 5, buffer);
      usleep(15000);
    }

    restore_layout();
  }
//}#pragma endregion Boot Screens

//{#pragma region ROM Options
  void rom_run(bool resume) {

    set_restore_states();

    draw_background();
    char *message = !resume ? "loading..." : "hold start";

    int h = 5;
    int w = strlen(message)*h;
    int x = (SCREEN.w/2)-(w/2);
    int y = (SCREEN.h/2)-(h/2);
    draw_text(x,y,message,false,false, false);
    y+=10;
    for(int n = 0; n < (w+10); n++) {
      for(int i = 0; i < 5; i++) {
        buffer[i] = GUI.fg;
      }
      ili9341_write_frame_rectangleLE(x+n, y, 1, 5, buffer);
      usleep(15000);
    }

    odroid_system_application_set(PROGRAMS[STEP-2]);
    usleep(10000);
    esp_restart();
  }

  void rom_resume() {

    set_restore_states();

    draw_background();
    char message[100] = "resuming...";
    int h = 5;
    int w = strlen(message)*h;
    int x = (SCREEN.w/2)-(w/2);
    int y = (SCREEN.h/2)-(h/2);
    draw_text(x,y,message,false,false, false);
    y+=10;
    for(int n = 0; n < (w+10); n++) {
      for(int i = 0; i < 5; i++) {
        buffer[i] = GUI.fg;
      }
      ili9341_write_frame_rectangleLE(x+n, y, 1, 5, buffer);
      usleep(15000);
    }

    odroid_system_application_set(PROGRAMS[STEP-2]);
    usleep(10000);
    esp_restart();
  }

  void rom_delete_save() {
    draw_background();
    char message[100] = "deleting...";
    int width = strlen(message)*5;
    int center = ceil((320/2)-(width/2));
    int y = 118;
    draw_text(center,y,message,false,false, false);

    y+=10;
    for(int n = 0; n < (width+10); n++) {
      for(int i = 0; i < 5; i++) {
        buffer[i] = GUI.fg;
      }
      ili9341_write_frame_rectangleLE(center+n, y, 1, 5, buffer);
      usleep(15000);
    }

    DIR *directory;
    struct dirent *file;
    char path[256] = "/sd/odroid/data/";
    strcat(&path[strlen(path) - 1], DIRECTORIES[STEP]);
    directory = opendir(path);
    gets(ROM.name);
    while ((file = readdir(directory)) != NULL) {
      char tmp[256] = "";
      char file_to_delete[256] = "";
      strcat(tmp, file->d_name);
      sprintf(file_to_delete, "%s/%s", path, file->d_name);
      tmp[strlen(tmp)-4] = '\0';
      gets(tmp);
      if(strcmp(ROM.name, tmp) == 0) {
        //printf("\nDIRECTORIES[STEP]:%s ROM.name:%s tmp:%s",DIRECTORIES[STEP], ROM.name, tmp);
        struct stat st;
        if (stat(file_to_delete, &st) == 0) {
          unlink(file_to_delete);
          LAUNCHER = false;
          draw_background();
          draw_systems();
          draw_text(16,16,EMULATORS[STEP],false,true, false);
          STEP == 0 ? draw_themes() : get_files();
        }
      }
    }
    //closedir(path);
  }
//}#pragma endregion ROM Options

//{#pragma region Launcher
  static void launcher() {
    draw_battery();
    draw_speaker();
    draw_contrast();

  //{#pragma region Gamepad
    while (true) {
      /*
        Get Gamepad State
      */
      odroid_input_gamepad_read(&gamepad);
      /*
        LEFT
      */
      if(gamepad.values[ODROID_INPUT_LEFT]) {
        if(!LAUNCHER && !FOLDER) {
          if(!SETTINGS && SETTING != 1 && SETTING != 2 && SETTING != 3 && SETTING != 4) {
            STEP--;
            if( STEP < 0 ) {
              STEP = COUNT - 1;
            }

            ROMS.offset = 0;
            animate(-1);
          } else {
            if(SETTING == 1) {
              nvs_handle handle;
              nvs_open("storage", NVS_READWRITE, &handle);
              nvs_set_i8(handle, "COLOR", 0);
              nvs_commit(handle);
              nvs_close(handle);
              draw_toggle();
              draw_systems();
            }
            if(SETTING == 2) {
              if(VOLUME > 0) {
                VOLUME--;
                set_volume();
                usleep(200000);
              }
            }
            if(SETTING == 3) {
              if(BRIGHTNESS > 0) {
                BRIGHTNESS--;
                set_brightness();
                usleep(200000);
              }
            }
            if(SETTING == 4) {
              nvs_handle handle;
              nvs_open("storage", NVS_READWRITE, &handle);
              nvs_set_i8(handle, "COVER", 0);
              nvs_commit(handle);
              nvs_close(handle);
              draw_cover_toggle();
            }
          }
        }
        usleep(100000);
        //debounce(ODROID_INPUT_LEFT);
      }
      /*
        RIGHT
      */
      if(gamepad.values[ODROID_INPUT_RIGHT]) {
        if(!LAUNCHER && !FOLDER) {
          if(!SETTINGS && SETTING != 1 && SETTING != 2 && SETTING != 3 && SETTING != 4) {
            STEP++;
            if( STEP > COUNT-1 ) {
              STEP = 0;
            }
            ROMS.offset = 0;
            animate(1);
          } else {
            if(SETTING == 1) {
              nvs_handle handle;
              nvs_open("storage", NVS_READWRITE, &handle);
              nvs_set_i8(handle, "COLOR", 1);
              nvs_commit(handle);
              nvs_close(handle);
              draw_toggle();
              draw_systems();
            }
            if(SETTING == 2) {
              if(VOLUME < 4) {
                VOLUME++;
                set_volume();
                usleep(200000);
              }
            }
            if(SETTING == 3) {
              if(BRIGHTNESS < (BRIGHTNESS_COUNT-1)) {
                BRIGHTNESS++;
                set_brightness();
                usleep(200000);
              }
            }
            if(SETTING == 4) {
              nvs_handle handle;
              nvs_open("storage", NVS_READWRITE, &handle);
              nvs_set_i8(handle, "COVER", 1);
              nvs_commit(handle);
              nvs_close(handle);
              draw_cover_toggle();
            }
          }
        }
        usleep(100000);
        //debounce(ODROID_INPUT_RIGHT);
      }
      /*
        UP
      */
      if (gamepad.values[ODROID_INPUT_UP]) {
        if(!LAUNCHER) {
          if(STEP == 0) {
            if(!SETTINGS) {
              SETTING--;
              if( SETTING < 0 ) { SETTING = 4; }
              draw_settings();
            } else {
              USER--;
              if( USER < 0 ) { USER = 21; }
              draw_themes();
            }
          }
          if(STEP != 0) {
            ROMS.offset--;
            if( ROMS.offset < 0 ) { ROMS.offset = ROMS.total - 1; }
            if(STEP != 1) {
              //draw_files();
              delete_numbers();
              seek_files();
            } else {
              process_favorites();
            }
          }
        } else {
          int min = SAVED ? 3 : 1;
          if(SAVED) {
            OPTION--;
            if( OPTION < 0 ) { OPTION = min; }
            draw_launcher_options();
          }
        }
        usleep(200000);
        //debounce(ODROID_INPUT_UP);
      }
      /*
        DOWN
      */
      if (gamepad.values[ODROID_INPUT_DOWN]) {
        if(!LAUNCHER) {
          if(STEP == 0) {
            if(!SETTINGS) {
              SETTING++;
              if( SETTING > 4 ) { SETTING = 0; }
              draw_settings();
            } else {
              USER++;
              if( USER > 21 ) { USER = 0; }
              draw_themes();
            }
          }
          if(STEP != 0) {
            ROMS.offset++;
            if( ROMS.offset > ROMS.total - 1 ) { ROMS.offset = 0; }
            if(STEP != 1) {
              //draw_files();
              delete_numbers();
              seek_files();
            } else {
              process_favorites();
            }
          }
        } else {
          int max = SAVED ? 3 : 1;
          OPTION++;
          if( OPTION > max ) { OPTION = 0; }
          draw_launcher_options();
        }

        usleep(200000);
        //debounce(ODROID_INPUT_DOWN);
      }

      /*
        START + SELECT
      */
      if (gamepad.values[ODROID_INPUT_START] || gamepad.values[ODROID_INPUT_SELECT]) {
        /*
          SELECT
        */
        if (gamepad.values[ODROID_INPUT_START] && !gamepad.values[ODROID_INPUT_SELECT]) {
          if(!LAUNCHER) {
            if(STEP != 0) {
              ROMS.page++;
              if( ROMS.page > ROMS.pages ) { ROMS.page = 0; }
              ROMS.offset =  ROMS.page * ROMS.limit;
              if(STEP != 1) {
                //draw_files();
                delete_numbers();
                seek_files();
              } else {
                process_favorites();
              }
            }
          }
          //debounce(ODROID_INPUT_START);
        }

        /*
          START
        */
        if (!gamepad.values[ODROID_INPUT_START] && gamepad.values[ODROID_INPUT_SELECT]) {
          if(!LAUNCHER) {
            if(STEP != 0) {
              ROMS.page--;
              if( ROMS.page < 0 ) { ROMS.page = ROMS.pages; };
              ROMS.offset =  ROMS.page * ROMS.limit;
              if(STEP != 1) {
                //draw_files();
                delete_numbers();
                seek_files();
              } else {
                process_favorites();
              }
            }
          }
          //debounce(ODROID_INPUT_SELECT);
        }

        usleep(200000);
      }

      /*
        BUTTON A
      */
      if (gamepad.values[ODROID_INPUT_A]) {
        if(STEP == 0) {
          if(!SETTINGS && SETTING == 0) {
            SETTINGS = true;
            draw_background();
            draw_systems();
            switch(SETTING) {
              case 0:
                draw_text(16,16,"THEMES",false,true, false);
                draw_themes();
              break;
            }
          } else {
           // printf("\nSETTING:%d COLOR:%d\n", SETTING, COLOR);
            switch(SETTING) {
              case 0:
                update_theme();
              break;
              case 1:
                set_toggle();
                draw_toggle();
                draw_systems();
              break;
              case 4:
                set_cover_toggle();
                draw_cover_toggle();
              break;
            }
          }
        } else if(STEP == 1) {
          printf("FAVORITES");
        } else {
          if (ROM.ready && !LAUNCHER && ROMS.total != 0) {
            OPTION = 0;
            char file_to_load[256] = "";
            sprintf(file_to_load, "%s/%s", ROM.path, ROM.name);
            bool is_directory = strcmp(&file_to_load[strlen(file_to_load) - 3], "dir") == 0;

            if(is_directory) {
              FOLDER = true;
              PREVIOUS = ROMS.offset;
              ROMS.offset = 0;

              sprintf(folder_path, "/%s", ROM.name);
              folder_path[strlen(folder_path)-(strlen(EXTENSIONS[STEP]) == 3 ? 4 : 3)] = 0;
              draw_background();
              draw_systems();
              draw_text(16,16,EMULATORS[STEP],false,true, false);
              get_files();
            } else {
              LAUNCHER = true;
              odroid_settings_RomFilePath_set(file_to_load);
              draw_launcher();
            }
          } else {
            if(ROMS.total != 0) {
              char favorite[256] = "";
              sprintf(favorite, "%s/%s", ROM.path, ROM.name);
              switch(OPTION) {
                case 0:
                  SAVED ? rom_resume() : rom_run(false);
                break;
                case 1:
                  SAVED ? rom_run(true) : ROM.favorite ? delete_favorite(favorite) : add_favorite(favorite);
                  if(!SAVED) {draw_launcher_options();}
                break;
                case 2:
                  rom_delete_save();
                break;
                case 3:
                  ROM.favorite ? delete_favorite(favorite) : add_favorite(favorite);
                  draw_launcher_options();
                break;
              }
            }
          }
        }
        debounce(ODROID_INPUT_A);
      }
      /*
        BUTTON B
      */
      if (gamepad.values[ODROID_INPUT_B]) {
        if (ROM.ready && LAUNCHER) {
          LAUNCHER = false;
          draw_background();
          draw_systems();
          draw_text(16,16,EMULATORS[STEP],false,true, false);
          if(FOLDER) {
           // printf("\n------\nfolder_path:%s\n-----\n", folder_path);
            get_files();
          } else {
            STEP == 0 ? draw_settings() : STEP == 1 ? get_favorites() : get_files();
          }
        } else {
          if(FOLDER) {
            FOLDER = false;
            ROMS.offset = PREVIOUS;
            PREVIOUS = 0;
            folder_path[0] = 0;
            get_files();
          }
          if(SETTINGS) {
            SETTINGS = false;
            draw_background();
            draw_systems();
            draw_text(16,16,EMULATORS[STEP],false,true, false);
            draw_settings();
          }
        }
        debounce(ODROID_INPUT_B);
      }

      /*
        START + SELECT (MENU)
      */
      if (gamepad.values[ODROID_INPUT_MENU]) {
        usleep(10000);
        esp_restart();
        debounce(ODROID_INPUT_MENU);
      }
    }
  //}#pragma endregion GamePad
  }
//}#pragma endregion Launcher
