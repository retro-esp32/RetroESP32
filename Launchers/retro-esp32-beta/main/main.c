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
  bool SPLASH = false;
  bool SETTINGS = false;

  int8_t STEP = 0;
  int OPTION = 0;
  int PREVIOUS = 0;
  int32_t VOLUME = 0;
  int32_t BRIGHTNESS = 0;
  int32_t BRIGHTNESS_COUNT = 10;
  int32_t BRIGHTNESS_LEVELS[10] = {10,20,30,40,50,60,70,80,90,100};
  int8_t USER;
  int8_t SETTING;
  int8_t COLOR;
  uint32_t currentDuty;

  char** FILES;
  char folder_path[256] = "";

  DIR *directory;
  struct dirent *file;
//}#pragma endregion Global

//{#pragma region Emulator and Directories
  char EMULATORS[COUNT][30] = {
    "SETTINGS",
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
    xTaskCreate(launcher, "launcher", 8192, NULL, 5, NULL);
  }
//}#pragma endregion Main

/*
  METHODS
*/
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

  void draw_text(short x, short y, char *string, bool ext, bool current) {
    int length = !ext ? strlen(string) : strlen(string)-(strlen(EXTENSIONS[STEP])+1);
    int rows = 7;
    int cols = 5;
    for(int n = 0; n < length; n++) {
      int dx = get_letter(string[n]);
      int i = 0;
      for(int r = 0; r < (rows); r++) {
        if(string[n] != ' ') {
          for(int c = dx; c < (dx+cols); c++) {
            //buffer[i] = FONT_5x5[r][c] == 0 ? GUI.bg : current ? WHITE : GUI.fg;
            buffer[i] = FONT_5x7[r][c] == 0 ? GUI.bg : current ? WHITE : GUI.fg;
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
    draw_text(x,y,"THEMES",false, SETTING == 0 ? true : false);

    y+=20;
    draw_mask(x,y-1,100,17);
    draw_text(x,y,"COLORED ICONS",false, SETTING == 1 ? true : false);
    draw_toggle();

    y+=20;
    draw_mask(x,y-1,100,17);
    draw_text(x,y,"VOLUME",false, SETTING == 2 ? true : false);

    draw_volume();

    y+=20;
    draw_mask(x,y-1,100,17);
    draw_text(x,y,"BRIGHTNESS",false, SETTING == 3 ? true : false);

    draw_brightness();

    /*
      BUILD
    */
    char message[100] = BUILD;
    int width = strlen(message)*5;
    int center = ceil((320)-(width))-48;
    y = 225;
    draw_text(center,y,message,false,false);
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
        draw_text(x,y,THEMES[n].name,false, n == USER ? true : false);
        y+=20;
        filled++;
      }
    }
    int slots = (ROMS.limit - filled);
    for(int n = 0; n < slots; n++) {
      draw_mask(x,y-1,100,17);
      draw_text(x,y,THEMES[n].name,false,false);
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
        USER = 0;
        break;
      default :
        USER = 0;
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
    draw_text(16,16,EMULATORS[STEP], false, true);
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

  void draw_media(int x, int y, bool current) {
    int offset = (STEP-1) * 16;
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
    int h = 7;
    int w = 0;
    char count[10];
    sprintf(count, "(%d/%d)", (ROMS.offset+1), ROMS.total);
    w = strlen(count)*5;
    x -= w;
    draw_mask(x-5,y,w+5,h);
    draw_text(x,y,count,false,false);
  }

  void draw_launcher() {
    draw_background();
    draw_text(16,16,EMULATORS[STEP], false, true);
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
    draw_media(x,y-6,true);
    draw_launcher_options();
  }

  void draw_launcher_options() {
    has_save_file(ROM.name);

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
      draw_text(x+10,y,"Resume",false,OPTION == 0?true:false);
      // restart
      i = 0;
      y+=20;
      offset = 10;
      for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
        buffer[i] = icons[r+offset][c] == WHITE ? OPTION == 1 ? WHITE : GUI.fg : GUI.bg;i++;
      }}
      ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
      draw_text(x+10,y,"Restart",false,OPTION == 1?true:false);
      // restart
      i = 0;
      y+=20;
      offset = 20;
      for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
        buffer[i] = icons[r+offset][c] == WHITE ? OPTION == 2 ? WHITE : GUI.fg : GUI.bg;i++;
      }}
      ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
      draw_text(x+10,y,"Delete Save",false,OPTION == 2?true:false);
    } else {
      // run
      i = 0;
      offset = 0;
      for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
        buffer[i] = icons[r+offset][c] == WHITE ? WHITE : GUI.bg;i++;
      }}
      ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
      draw_text(x+10,y,"Run",false,true);
    }
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
    char message[100] = "loading";
    int center = ceil((320/2)-((strlen(message)*5)/2));
    draw_text(center,134,message,false,false);

    ROMS.total = 0;

    char path[256] = "/sd/roms/";
    strcat(&path[strlen(path) - 1], DIRECTORIES[STEP]);
    strcat(&path[strlen(path) - 1],folder_path);
    strcpy(ROM.path, path);

    DIR *directory = opendir(path);  
    
    if(directory == NULL) {
      draw_mask(0,132,320,10);
      char message[100] = "rom folder not found";
      int center = ceil((320/2)-((strlen(message)*5)/2));
      draw_text(center,134,message,false,false);
    } else {              
      struct dirent *file;
      while ((file = readdir(directory)) != NULL) {
        int rom_length = strlen(file->d_name);
        int ext_lext = strlen(EXTENSIONS[STEP]);
        bool extenstion = strcmp(&file->d_name[rom_length - ext_lext], EXTENSIONS[STEP]) == 0 && file->d_name[0] != '.';
        if(extenstion || (file->d_type == 2)) {
          ROMS.total++;
        }
      }
      ROMS.pages = ROMS.total/ROMS.limit;
      if(ROMS.offset > ROMS.total) { ROMS.offset = 0;}
      closedir(directory); 
    }

    draw_mask(0,132,320,10);
    if(ROMS.total != 0) {
      draw_files();
    } else {
      char message[100] = "no roms available";
      int center = ceil((320/2)-((strlen(message)*5)/2));
      draw_text(center,134,message,false,false);
    }
  }

  void seek_files() {
    free(FILES);
    FILES = (char**)malloc(ROMS.limit * sizeof(void*)); 

    ///*
    printf("\n-----------");
    printf("\nROMS.offset:%d", ROMS.offset);
    printf("\nROMS.limit:%d", ROMS.limit);
    printf("\nROMS.total:%d", ROMS.total);
    printf("\nROMS.page:%d", ROMS.page);
    printf("\nROMS.pages:%d", ROMS.pages);
    printf("\n-----------");
    printf("\n");
    //*/                          

    char path[256] = "/sd/roms/";
    strcat(&path[strlen(path) - 1], DIRECTORIES[STEP]);
    strcat(&path[strlen(path) - 1],folder_path);
    strcpy(ROM.path, path); 
    
    DIR *directory = opendir(path); 

    if(directory == NULL) {
      char message[100] = "rom folder not found";
      int center = ceil((320/2)-((strlen(message)*5)/2));
      draw_text(center,134,message,false,false);
    } else {              
      rewinddir(directory);
      seekdir(directory, ROMS.offset);
      struct dirent *file;

      int n =0;
      while ((file = readdir(directory)) != NULL) {
        if(n >= ROMS.limit){break;}
        int rom_length = strlen(file->d_name);
        int ext_lext = strlen(EXTENSIONS[STEP]);
        bool extenstion = strcmp(&file->d_name[rom_length - ext_lext], EXTENSIONS[STEP]) == 0 && file->d_name[0] != '.';
        if(extenstion || (file->d_type == 2)) {
          size_t len = strlen(file->d_name);
          FILES[n] = (file->d_type == 2) ? (char*)malloc(len + 5) : (char*)malloc(len + 1);
          if((file->d_type == 2)) {
            char dir[256];
            strcpy(dir, file->d_name);
            char dd[8];
            sprintf(dd, "%s", ext_lext == 2 ? "dir" : ".dir");
            strcat(&dir[strlen(dir) - 1], dd);
            strcpy(FILES[n], dir);
          } else {
            strcpy(FILES[n], file->d_name);
          }          
          n++;
        }
      }
      closedir(directory); 
    }
  }

  void draw_files() {
    //printf("\n");
    int x = ORIGIN.x;
    int y = POS.y + 48;
    ROMS.page = ROMS.offset/ROMS.limit;

    seek_files();
    
    for (int i = 0; i < 4; i++) draw_mask(0, y+(i*40)-6, 320, 40);

    int limit = ROMS.total < ROMS.limit ? ROMS.total : ROMS.limit;                                                 
    for(int n = 0; n < limit; n++) {                                   
      //printf("\n%d:%s", n, FILES[n]);
      draw_text(x+24,y,FILES[n],true,n == 0 ? true : false);
      bool directory = strcmp(&FILES[n][strlen(FILES[n]) - 3], "dir") == 0;
      directory ? draw_folder(x,y-6,n == 0 ? true : false) : draw_media(x,y-6,n == 0 ? true : false);
      if(n == ROMS.offset) {
        strcpy(ROM.name, FILES[n]);
        ROM.ready = true;
      }
      y+=20;
    }


    draw_numbers();
  }

  void has_save_file(char *name) {
    SAVED = false;
    DIR *directory;
    struct dirent *file;
    char path[256] = "/sd/odroid/data/";
    strcat(&path[strlen(path) - 1], DIRECTORIES[STEP]);
    directory = opendir(path);
    gets(name);
    while ((file = readdir(directory)) != NULL) {
      char tmp[256] = "";
      strcat(tmp, file->d_name);
      tmp[strlen(tmp)-4] = '\0';
      gets(tmp);
      if(strcmp(name, tmp) == 0){
        SAVED = true;
      }
    }
    closedir(directory);
  }
//}#pragma endregion Files

//{#pragma region Animations
  void animate(int dir) {
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
    draw_mask(0,0,SCREEN.w - 56,32);
    draw_text(16,16,EMULATORS[STEP], false, true);
    STEP == 0 ? draw_settings() : count_files();
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
    draw_text(16,16,EMULATORS[STEP],false,true);
    STEP == 0 ? draw_settings() : count_files();
  }

  void clean_up() {
    int MAX = 736;
    for(int n = 0; n < COUNT; n++) {
      if(SYSTEMS[n].x > 512) {
        SYSTEMS[n].x -= MAX;
      }
      if(SYSTEMS[n].x <= -272) {
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
    draw_text(center,y,message,false,false);

    sleep(2);
    draw_background();
  }

  void boot() {
    draw_background();
    char message[100] = "retro esp32";
    int width = strlen(message)*5;
    int center = ceil((320/2)-(width/2));
    int y = 118;
    draw_text(center,y,message,false,false);

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
    draw_text(x,y,message,false,false);

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
    draw_text(x,y,message,false,false);
    y+=10;
    for(int n = 0; n < (w+10); n++) {
      for(int i = 0; i < 5; i++) {
        buffer[i] = GUI.fg;
      }
      ili9341_write_frame_rectangleLE(x+n, y, 1, 5, buffer);
      usleep(15000);
    }

    odroid_system_application_set(PROGRAMS[STEP-1]);
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
    draw_text(x,y,message,false,false);
    y+=10;
    for(int n = 0; n < (w+10); n++) {
      for(int i = 0; i < 5; i++) {
        buffer[i] = GUI.fg;
      }
      ili9341_write_frame_rectangleLE(x+n, y, 1, 5, buffer);
      usleep(15000);
    }

    odroid_system_application_set(PROGRAMS[STEP-1]);
    usleep(10000);
    esp_restart();
  }

  void rom_delete_save() {
    draw_background();
    char message[100] = "deleting...";
    int width = strlen(message)*5;
    int center = ceil((320/2)-(width/2));
    int y = 118;
    draw_text(center,y,message,false,false);

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
          draw_text(16,16,EMULATORS[STEP],false,true);
          STEP == 0 ? draw_themes() : count_files();
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
          if(SETTING != 2 && SETTING != 3) {
            STEP--;
            if( STEP < 0 ) {
              STEP = COUNT - 1;
            }

            ROMS.offset = 0;
            ROMS.total = 0;
            animate(-1);
          } else {
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
          if(SETTING != 2 && SETTING != 3) {
            STEP++;
            if( STEP > COUNT-1 ) {
              STEP = 0;
            }
            ROMS.offset = 0;
            ROMS.total = 0;
            animate(1);
          } else {
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
              if( SETTING < 0 ) { SETTING = 3; }
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
            draw_files();
          }
        } else {
          if(SAVED) {
            OPTION--;
            if( OPTION < 0 ) { OPTION = 2; }
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
              if( SETTING > 3 ) { SETTING = 0; }
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
            draw_files();
          }
        } else {
          if(SAVED) {
            OPTION++;
            if( OPTION > 2 ) { OPTION = 0; }
            draw_launcher_options();
          }
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
              draw_files();
            }
          }
          //debounce(ODROID_INPUT_START);
        }

        /*
          SELECT
        */
        if (!gamepad.values[ODROID_INPUT_START] && gamepad.values[ODROID_INPUT_SELECT]) {
          if(!LAUNCHER) {
            if(STEP != 0) {
              ROMS.page--;
              if( ROMS.page < 0 ) { ROMS.page = ROMS.pages; };
              ROMS.offset =  ROMS.page * ROMS.limit;
              draw_files();
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
                draw_text(16,16,"THEMES",false,true);
                draw_themes();
              break;
            }
          } else {
            printf("\nSETTING:%d COLOR:%d\n", SETTING, COLOR);
            switch(SETTING) {
              case 0:
                update_theme();
              break;
              case 1:
                set_toggle();
                draw_toggle();
                draw_systems();
              break;
            }
          }
        } else {
          if (ROM.ready && !LAUNCHER) {
            OPTION = 0;
            char file_to_load[256] = "";
            sprintf(file_to_load, "%s/%s", ROM.path, ROM.name);
            bool directory = strcmp(&file_to_load[strlen(file_to_load) - 3], "dir") == 0;

            if(directory) {
              FOLDER = true;
              PREVIOUS = ROMS.offset;
              ROMS.offset = 0;
              ROMS.total = 0;

              sprintf(folder_path, "/%s", ROM.name);
              folder_path[strlen(folder_path)-(strlen(EXTENSIONS[STEP]) == 3 ? 4 : 3)] = 0;
              draw_background();
              draw_systems();
              draw_text(16,16,EMULATORS[STEP],false,true);
              count_files();
            } else {
              LAUNCHER = true;
              odroid_settings_RomFilePath_set(file_to_load);
              draw_launcher();
            }
          } else {
            switch(OPTION) {
              case 0:
                SAVED ? rom_resume() : rom_run(false);
              break;
              case 1:
                rom_run(true);
              break;
              case 2:
                rom_delete_save();
              break;
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
          draw_text(16,16,EMULATORS[STEP],false,true);
          if(FOLDER) {
            printf("\n------\nfolder_path:%s\n-----\n", folder_path);
            count_files();
          } else {
            STEP == 0 ? draw_settings() : draw_files();
          }
        } else {
          if(FOLDER) {
            FOLDER = false;
            ROMS.offset = PREVIOUS;
            ROMS.total = 0;
            PREVIOUS = 0;
            folder_path[0] = 0;
            count_files();
          }
          if(SETTINGS) {
            SETTINGS = false;
            draw_background();
            draw_systems();
            draw_text(16,16,EMULATORS[STEP],false,true);
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
  }
//}#pragma endregion Launcher
