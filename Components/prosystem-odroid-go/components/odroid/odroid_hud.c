//{#pragma region Includes
  /*
    C
  */
  #include <stdio.h>
  #include <string.h>
  #include <math.h>
  #include <dirent.h>
  #include <unistd.h>
  #include <ctype.h>
  #include <errno.h>

  /*
    General
  */
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
  #include "nvs_flash.h"
  #include "sdmmc_cmd.h"

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
  #include "driver/gpio.h"
  #include "driver/sdmmc_host.h"
  #include "driver/sdspi_host.h"

  /*
    Odroid
  */
  #include "odroid_settings.h"
  #include "odroid_audio.h"
  #include "odroid_system.h"
  #include "odroid_sdcard.h"
  #include "odroid_display.h"
  #include "odroid_input.h"
//}#pragma endregion Includes

//{#pragma region Globals
  #define WHITE 65535
  #define BLACK 0
  int OPTION = 0;
  int OPTIONS = 3;
  extern int ACTION = 0;
  int8_t USER;
  int8_t DELETE_SAVE;
  int x, y, w, h, i, size;
  uint16_t *buffer;
  bool INIT = false;
  bool SAVED = false;
  bool forceConsoleReset;

  int32_t VOLUME = 0;
  int32_t BRIGHTNESS = 0;
  const int32_t BRIGHTNESS_COUNT = 10;
  const int32_t BRIGHTNESS_LEVELS[10] = {10,20,30,40,50,60,70,80,90,100};        
//}#pragma endregion Globals

//{#pragma region Structs
  typedef struct{
    int x;
    int y;
    int w;
    int h;
  } SCR;
  SCR SCREEN = {0,0,320,240};

  typedef struct{
    char label[20];
    int action;
    int offset;
  } STATES;
  STATES WITHSAVE[5] = {
    {"Resume Game",0, 0},
    {"Restart Game",1, 10},
    {"Reload Game",2, 5},
    {"Overwrite Game",4, 25},
    {"Delete Save",5, 15}
  };
  STATES WITHOUTSAVE[2] = {
    {"Resume Game",0, 0},
    {"Restart Game",1, 10}
  };
  STATES EXTRAS[3] = {
    {"Volume",8, 45},
    {"Brightness",9, 50},
    {"Quit Game",6, 20}
  };     
  STATES STATE;

// THEME
typedef struct{
  int bg;
  int fg;
  int hl;
  char name[10];
} THEME;
THEME THEMES[22] = {
	{63877,35043,20610,"grapefruit"},
	{64420,35362,20770,"caramel"},
	{64963,35650,20961,"beer"},
	{65504,35937,21121,"mustard"},
	{49120,27745,14977,"slime"},
	{30689,17505,8833,"algae"},
	{8162,7266,4737,"jade"},
	{8174,5224,2692,"frog"},
	{8183,5229,2695,"seafoam"},
	{8191,5233,2697,"celeste"},
	{9727,6993,4586,"denim"},
	{11231,6706,4394,"sky"},
	{12543,6257,4202,"cobalt"},
	{28959,16561,8298,"indigo"},
	{47359,26801,14442,"plum"},
	{63743,34961,20586,"orchid"},
	{63799,34989,20583,"cupcake"},
	{63855,35016,20613,"lemonade"},
	{0,38034,48631,"night"},
	{27437,46518,55002,"carbon"},
	{44373,55002,65535,"smoke"},
	{65535,33840,16904,"cloud"},
};
THEME GUI;
//}#pragma endregion Structs

//{#pragma region Sprites
const uint16_t FONT_5x7[7][250] = {
  {0,65535,65535,65535,0,65535,65535,65535,65535,0,0,65535,65535,65535,0,65535,65535,65535,65535,0,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,0,65535,65535,65535,0,65535,0,0,0,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,0,0,0,65535,65535,0,0,0,0,65535,0,0,0,65535,65535,0,0,0,65535,0,65535,65535,65535,0,65535,65535,65535,65535,0,0,65535,65535,65535,0,65535,65535,65535,65535,0,0,65535,65535,65535,0,65535,65535,65535,65535,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,65535,65535,65535,65535,0,65535,65535,65535,0,0,0,65535,0,0,0,65535,65535,65535,0,0,65535,65535,65535,0,0,0,0,65535,65535,65535,65535,65535,65535,65535,0,65535,65535,65535,0,65535,65535,65535,65535,65535,0,65535,65535,65535,0,0,65535,65535,65535,0,0,0,65535,0,0,0,0,0,0,0,0,0,0,65535,0,0,0,65535,0,0,0,65535,65535,65535,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,65535,0,0,65535,0,0,0,0,65535,65535,65535,0,0,65535,65535,65535,0,0,0,0,0,0,0,0,0,0,0},
  {65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,0,65535,0,0,0,0,65535,0,0,0,65535,65535,0,0,0,65535,0,0,65535,0,0,0,0,65535,0,0,65535,0,0,65535,0,65535,0,0,0,0,65535,65535,0,65535,65535,65535,65535,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,0,0,65535,0,0,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,0,65535,0,65535,0,0,0,0,0,65535,65535,0,0,0,65535,0,65535,65535,0,0,65535,0,0,0,65535,65535,0,0,0,65535,0,0,65535,0,65535,65535,0,0,0,0,65535,0,0,0,65535,0,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,0,0,65535,0,0,0,0,0,0,0,0,0,0,65535,0,0,65535,0,65535,0,65535,0,0,0,65535,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,65535,0,0,0,0,65535,0,0,0,65535,0,0,0,0,0,0,65535,0,0,0,0,0,0,0,0,0,0,0},
  {65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,0,65535,0,0,0,65535,65535,0,0,0,0,65535,0,0,0,0,65535,0,0,0,0,65535,0,0,0,65535,0,0,65535,0,0,0,0,65535,0,0,65535,0,65535,0,0,65535,0,0,0,0,65535,0,65535,0,65535,65535,0,65535,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,0,0,0,65535,0,0,65535,0,0,0,65535,0,65535,0,65535,0,65535,0,0,0,65535,0,65535,0,65535,0,0,0,65535,0,0,0,0,0,65535,0,65535,0,0,0,65535,65535,0,65535,0,0,0,0,0,0,65535,0,0,0,0,65535,0,65535,0,0,65535,65535,0,0,0,0,65535,0,0,0,0,0,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,0,0,65535,0,0,0,0,0,0,0,0,0,0,0,0,0,0,65535,0,0,0,0,0,0,65535,0,0,0,0,0,0,0,0,0,0,0,0,0,0,65535,0,65535,0,0,0,0,0,0,65535,0,0,65535,0,0,0,0,0,0,65535,0,0,0,0,0,0,0,0,0,0,0},
  {65535,65535,65535,65535,65535,65535,65535,65535,65535,0,65535,0,0,0,0,65535,0,0,0,65535,65535,65535,65535,65535,0,65535,65535,65535,65535,0,65535,0,0,0,0,65535,65535,65535,65535,65535,0,0,65535,0,0,0,0,65535,0,0,65535,65535,0,0,0,65535,0,0,0,0,65535,0,0,0,65535,65535,0,0,65535,65535,65535,0,0,0,65535,65535,65535,65535,65535,0,65535,0,0,0,65535,65535,65535,65535,65535,0,0,65535,65535,65535,0,0,0,65535,0,0,65535,0,0,0,65535,0,65535,0,65535,0,65535,0,65535,0,65535,0,0,65535,0,0,0,0,65535,0,0,0,0,65535,0,0,65535,0,0,0,65535,0,0,65535,0,0,0,0,0,65535,0,0,0,65535,65535,0,65535,0,0,0,65535,0,65535,65535,65535,0,65535,65535,65535,65535,0,0,0,0,65535,0,0,65535,65535,65535,0,0,65535,65535,65535,65535,0,0,65535,0,0,0,0,0,0,0,0,0,0,0,0,0,65535,0,0,0,0,0,0,65535,0,0,0,0,0,0,0,0,0,0,0,0,0,0,65535,0,0,65535,0,0,0,0,0,0,65535,0,0,65535,0,0,0,0,0,0,65535,0,65535,0,65535,0,65535,0,0,0,0,0},
  {65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,0,65535,0,0,0,65535,65535,0,0,0,0,65535,0,0,0,0,65535,0,65535,65535,65535,65535,0,0,0,65535,0,0,65535,0,0,65535,0,65535,0,0,65535,0,65535,0,0,65535,0,0,0,0,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,0,65535,0,0,0,65535,65535,0,65535,0,0,0,0,0,0,65535,0,0,65535,0,0,65535,0,0,0,65535,0,65535,0,65535,0,65535,0,65535,0,65535,0,65535,0,65535,0,0,0,65535,0,0,0,65535,0,0,0,65535,0,0,0,65535,0,0,65535,0,0,0,0,65535,0,0,0,0,0,0,65535,65535,65535,65535,65535,65535,0,0,0,0,65535,65535,0,0,0,65535,0,0,65535,0,0,65535,0,0,0,65535,0,0,0,0,65535,0,0,65535,0,0,0,65535,65535,65535,0,0,0,0,0,0,65535,0,65535,0,65535,0,0,65535,0,0,0,0,0,0,0,0,0,0,0,0,0,0,65535,0,0,0,65535,0,0,0,0,0,0,65535,0,0,65535,0,0,0,0,0,0,65535,0,0,65535,0,65535,0,0,0,0,0,0},
  {65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,0,65535,0,0,0,0,65535,0,0,0,65535,65535,0,0,0,65535,0,0,65535,0,0,65535,0,65535,0,0,65535,0,0,65535,0,65535,0,0,0,0,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,0,0,65535,65535,65535,0,65535,0,0,65535,0,65535,0,0,0,65535,0,0,65535,0,0,65535,0,0,0,65535,0,0,65535,0,0,65535,0,65535,0,65535,65535,0,0,0,65535,0,0,65535,0,0,65535,0,0,0,0,65535,0,0,0,65535,0,0,65535,0,0,0,65535,0,0,0,65535,0,0,0,65535,0,0,0,0,65535,65535,0,0,0,65535,65535,0,0,0,65535,0,65535,0,0,0,65535,0,0,0,65535,65535,0,0,0,65535,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,65535,0,0,65535,65535,0,0,0,0,0,0,0,0,0,0,0,0,65535,0,0,0,65535,0,0,0,0,0,65535,0,0,0,0,65535,0,0,0,65535,0,0,0,0,0,0,65535,0,0,0,0,0,0,0,0,0,0,0},
  {65535,0,0,0,65535,65535,65535,65535,65535,0,0,65535,65535,65535,0,65535,65535,65535,65535,0,65535,65535,65535,65535,65535,65535,0,0,0,0,0,65535,65535,65535,0,65535,0,0,0,65535,65535,65535,65535,65535,65535,0,65535,65535,0,0,65535,0,0,0,65535,65535,65535,65535,65535,65535,65535,0,0,0,65535,65535,0,0,0,65535,0,65535,65535,65535,0,65535,0,0,0,0,0,0,0,0,65535,65535,0,0,0,65535,0,65535,65535,65535,0,0,0,65535,0,0,0,65535,65535,65535,0,0,0,65535,0,0,0,65535,0,65535,0,65535,0,0,0,65535,0,0,65535,0,0,65535,65535,65535,65535,65535,0,65535,65535,65535,0,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,0,65535,65535,65535,0,0,0,0,0,65535,0,65535,65535,65535,0,0,65535,65535,65535,0,65535,0,0,0,0,0,65535,65535,65535,0,0,65535,65535,65535,0,0,0,65535,0,0,0,0,0,0,0,0,0,0,0,0,0,65535,65535,65535,0,0,0,65535,0,0,0,0,65535,0,0,0,0,65535,0,0,65535,0,0,0,0,0,0,0,65535,0,0,65535,0,0,0,0,65535,65535,65535,0,0,65535,65535,65535,0,0,0,0,0,0,0,0,0,0,0},
};

const uint16_t icons[55][5] = {
	{0,65535,65535,0,0},
	{0,65535,65535,65535,0},
	{0,65535,65535,65535,65535},
	{0,65535,65535,65535,0},
	{0,65535,65535,0,0},
	{65535,0,65535,0,0},
	{65535,0,65535,65535,0},
	{65535,0,65535,65535,65535},
	{65535,0,65535,65535,0},
	{65535,0,65535,0,0},
	{0,65535,65535,0,65535},
	{65535,0,0,65535,65535},
	{65535,0,0,0,65535},
	{65535,65535,0,0,65535},
	{65535,0,65535,65535,0},
	{0,65535,65535,65535,0},
	{65535,0,65535,0,65535},
	{65535,65535,0,65535,65535},
	{65535,0,65535,0,65535},
	{0,65535,65535,65535,0},
	{65535,0,0,0,65535},
	{0,65535,0,65535,0},
	{0,0,65535,0,0},
	{0,65535,0,65535,0},
	{65535,0,0,0,65535},
	{0,0,0,0,0},
	{0,0,0,0,65535},
	{0,0,0,65535,0},
	{65535,0,65535,0,0},
	{0,65535,0,0,0},
	{0,65535,65535,65535,0},
	{65535,0,65535,0,65535},
	{65535,0,65535,65535,65535},
	{65535,0,0,0,65535},
	{0,65535,65535,65535,0},
	{0,65535,0,65535,0},
	{65535,65535,65535,65535,65535},
	{65535,65535,65535,65535,65535},
	{0,65535,65535,65535,0},
	{0,0,65535,0,0},
	{0,65535,0,65535,0},
	{65535,0,65535,0,65535},
	{65535,0,0,0,65535},
	{0,65535,0,65535,0},
	{0,0,65535,0,0},
	{0,0,0,65535,0},
	{0,65535,0,0,65535},
	{0,0,65535,0,65535},
	{0,65535,0,0,65535},
	{0,0,0,65535,0},
	{65535,0,0,0,65535},
	{0,0,65535,0,0},
	{0,65535,0,65535,0},
	{0,0,65535,0,0},
	{65535,0,0,0,65535},
};

  const uint16_t logo[12][64] = {
    {0,0,0,0,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,0,0,0,0},
    {0,0,65535,65535,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,65535,65535,0,0},
    {0,65535,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,0,0,0,65535,0},
    {0,65535,0,0,0,0,0,0,65535,65535,65535,65535,0,0,0,0,0,0,65535,0,0,0,0,0,0,0,0,0,0,0,0,0,65535,65535,0,0,0,0,65535,65535,0,0,0,65535,0,0,0,65535,65535,65535,0,0,65535,65535,65535,0,0,65535,65535,65535,65535,0,65535,0},
    {65535,0,0,0,0,0,0,0,65535,0,0,65535,0,0,0,0,0,0,65535,65535,0,0,0,0,0,0,0,0,0,0,0,65535,65535,65535,0,65535,65535,65535,65535,0,65535,65535,65535,65535,0,65535,65535,0,65535,0,65535,65535,0,65535,0,65535,65535,0,65535,65535,65535,0,0,65535},
    {65535,0,0,0,0,0,0,65535,0,0,0,65535,0,65535,65535,0,0,65535,0,0,65535,65535,0,0,65535,65535,0,0,0,0,0,65535,65535,65535,0,0,0,65535,65535,0,0,0,65535,65535,0,65535,65535,0,65535,65535,65535,0,65535,65535,65535,65535,65535,0,65535,65535,65535,65535,0,65535},
    {65535,0,0,0,0,0,0,65535,65535,65535,65535,0,65535,0,0,65535,0,65535,0,0,65535,0,65535,65535,0,0,65535,0,0,0,65535,65535,65535,65535,0,65535,65535,65535,65535,65535,65535,65535,0,65535,0,0,0,65535,65535,65535,65535,65535,0,65535,65535,65535,0,65535,65535,65535,65535,65535,0,65535},
    {65535,0,0,0,0,0,65535,0,0,0,65535,0,65535,65535,0,0,65535,0,0,65535,0,0,0,65535,0,0,65535,0,0,0,65535,65535,65535,65535,0,65535,65535,65535,65535,65535,65535,65535,0,65535,0,65535,65535,65535,65535,0,65535,65535,0,65535,65535,0,65535,65535,65535,65535,65535,0,0,65535},
    {0,65535,0,0,0,0,65535,0,0,0,65535,0,0,65535,65535,0,65535,0,0,65535,0,0,0,0,65535,65535,0,0,0,65535,65535,65535,65535,65535,0,0,0,0,65535,0,0,0,65535,65535,0,65535,65535,65535,65535,65535,0,0,65535,65535,0,0,0,0,65535,65535,65535,0,65535,0},
    {0,65535,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,0,0,0,65535,0},
    {0,0,65535,65535,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,65535,65535,0,0},
    {0,0,0,0,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,0,0,0,0},

  };
//}#pragma endregion Sprites

//{#pragma region Odroid
  static odroid_gamepad_state gamepad;
//}#pragma endregion Odroid

//{#pragma region Debug
  void hud_debug(char *string) {
    printf("\n**********\n%s\n**********\n", string);
  }
//}#pragma endregion Debug

//{#pragma region Debounce
  void debounce(int key) {
    while (gamepad.values[key]) odroid_input_gamepad_read(&gamepad);
  }
//}#pragma endregion Debounce

//{#pragma region Files
  void hud_prepare_delete(int del) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    nvs_handle handle;
    nvs_open("storage", NVS_READWRITE, &handle);
    nvs_set_i8(handle, "DELETE_SAVE", del);
    nvs_commit(handle);
    nvs_close(handle);
  }

  void hud_delete_save(char *file_to_delete) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    nvs_handle handle;
    err = nvs_open("storage", NVS_READWRITE, &handle);
    err = nvs_get_i8(handle, "DELETE_SAVE", &DELETE_SAVE);
    switch (err) {
      case ESP_OK:
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        DELETE_SAVE = 0;
        break;
      default :
        DELETE_SAVE = 0;
    }
    nvs_close(handle);
    printf("\nDELETE_SAVE:%d\n\n", DELETE_SAVE);
    if(DELETE_SAVE == 1) {
      hud_prepare_delete(0);
      struct stat st;
      if (stat(file_to_delete, &st) == 0) {
        unlink(file_to_delete);
        esp_restart();
      }
    }
  }

  void hud_check_saves(char *save_name) {
    hud_debug("hud_check_saves START");
    char * dir = "lynx";//strrchr(save_name, '.');
    char save_dir[256] = "/sd/odroid/data/";
    strcat(&save_dir[strlen(save_dir) - 1], dir);
    char file_to_delete[256] = "";
    sprintf(file_to_delete, "%s/%s", save_dir, save_name);
    strcat(&file_to_delete[strlen(file_to_delete) - 1], ".sav");
    printf("\nSEARCH\nfile_to_delete:%s\nsave_dir:%s\n", file_to_delete, save_dir);
    DIR *directory;
    directory = opendir(save_dir);
    if(directory == NULL) {
      perror("opendir() error");
    } else {
      gets(save_name);
      struct dirent *file;
      while ((file = readdir(directory)) != NULL) {
        char tmp[256] = "";
        strcat(tmp, file->d_name);
        tmp[strlen(tmp)-4] = '\0';
        gets(tmp);
        if(strcmp(save_name, tmp) == 0) {
          SAVED = true;
          printf("\nFOUND\nfile_to_delete:%s\nsave_name:%s\ntmp%s\n", file_to_delete, save_name, tmp);
          hud_delete_save(file_to_delete);
        }
      }
      closedir(directory);
    }
    hud_debug("hud_check_saves END");
  }
//}#pragma endregion Files

//{#pragma region Text
  int hud_letter(char letter) {
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

  void hud_text(short x, short y, char *string, bool ext, bool current) {
    int length = strlen(string);
    int size = 5;
    int rows = 7;
    int cols = 5;
    for(int n = 0; n < length; n++) {
      int dx = hud_letter(string[n]);
      int i = 0;
      for(int r = 0; r < (rows); r++) {
        if(string[n] != ' ') {
          for(int c = dx; c < (dx+cols); c++) {
            //buffer[i] = FONT_5x5[r][c] == 0 ? GUI.bg : current ? WHITE : GUI.fg;
            buffer[i] = FONT_5x7[r][c] == 0 ? GUI.bg : current ? GUI.hl : GUI.fg;
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
  void hud_mask(int x, int y, int w, int h){
    for (i = 0; i < w * h; i++) buffer[i] = GUI.bg;
    ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
  }

  void hud_background() {
    int w = 320;
    int h = 30;
    for (int i = 0; i < 240/h; i++) hud_mask(0, i*h, w, h);
  }
//}#pragma endregion Mask

//{#pragma region Theme
  void hud_theme() {
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
//}#pragma endregion Theme

//{#pragma region Display
  void hud_logo() {
    x = 16;
    y = 16;
    w = 64;
    h = 12;
    i = 0;
    for(int r = 0; r < h; r++) {
      for(int c = 0; c < w; c++) {
        buffer[i] = logo[r][c] == 0 ? GUI.bg : GUI.hl;
        i++;
      }
    }
    ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
    //hud_text(x,y+18,"In Game Menu",false,false);
  }

  void hud_progress(char *string, bool bar) {
    hud_background();
    hud_logo();
    w = strlen(string)*5;
    x = (SCREEN.w/2)-(w/2);
    y = (SCREEN.h/2)-(h/2);
    hud_text(x,y,string,false,false);
    if(bar) {
      y+=10;
      for(int n = 0; n < (w+10); n++) {
        for(int i = 0; i < 5; i++) {
          buffer[i] = GUI.fg;
        }
        ili9341_write_frame_rectangleLE(x+n, y, 1, 5, buffer);
        usleep(15000);
      }
    }
  }

void hud_bar(int x, int y, int percent, bool active) {
    int w, h;

    int i = 0;
    for(h = 0; h < 7; h++) {
      for(w = 0; w < 100; w++) {
        buffer[i] = (w+h)%2 == 0 ? GUI.fg : GUI.bg;
        i++;
      }
    }  
    ili9341_write_frame_rectangleLE(x, y, 100, 7, buffer);  

    if(percent > 0) {
      i = 0;
      for(h = 0; h < 7; h++) {
        for(w = 0; w < percent; w++) {
          buffer[i] = active ? GUI.hl : GUI.fg;
          i++;
        }
      }  
      ili9341_write_frame_rectangleLE(x, y, percent, 7, buffer);  
    }    
  }

  void hud_volume() {
    // y=176;
    int32_t VOLUME = odroid_settings_Volume_get();
    int LIMIT = OPTIONS - 3;
    bool active = OPTION == LIMIT ? true : false;
    hud_bar((SCREEN.w - 120), 176, VOLUME * 12.5, active);
  }

  void hud_brightness() {
    // y=176;
    int LIMIT = OPTIONS - 2;
    bool active = OPTION == LIMIT ? true : false;    
    BRIGHTNESS = odroid_settings_Backlight_get();

    hud_bar((SCREEN.w - 120), 196, (BRIGHTNESS_COUNT * BRIGHTNESS)+BRIGHTNESS+1, active);
  }   

  void hud_options() {
    x = 16;
    y = 28;
    w = 5;
    h = 5;
    i = 0;
    int n;
    if(OPTIONS == 7) {
      for(n = 0; n < OPTIONS; n++) {
        STATE = WITHSAVE[n];
        y+=20;
        for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
          buffer[i] = icons[r+STATE.offset][c] == WHITE ? OPTION == n ? GUI.hl : GUI.fg : GUI.bg;i++;
        }}
        if(n == OPTIONS-1) {y = 216;}
        ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
        hud_text(x+10,y,STATE.label,false,OPTION == n?true:false);
        i = 0;
      }
      STATE = WITHSAVE[OPTION];
    } else {
      for(n = 0; n < 2; n++) {
        STATE = WITHOUTSAVE[n];
        y+=20;
        for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
          buffer[i] = icons[r+STATE.offset][c] == WHITE ? OPTION == n ? GUI.hl : GUI.fg : GUI.bg;i++;
        }}
        if(n == OPTIONS-1) {y = 216;}
        ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
        hud_text(x+10,y,STATE.label,false,OPTION == n?true:false);
        i = 0;
      }
      STATE = WITHOUTSAVE[OPTION];
    }

    y=156;
    for(n = 0; n < 3; n++) {
      STATE = EXTRAS[n];
      y+=20;
      for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
        buffer[i] = icons[r+STATE.offset][c] == WHITE ? OPTION == (n+OPTIONS-3) ? GUI.hl : GUI.fg : GUI.bg;i++;
      }}
      ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
      hud_text(x+10,y,STATE.label,false,OPTION == (n+OPTIONS-3)?true:false);
      i = 0;
    } 

    int LIMIT = OPTIONS - 3;
    if(OPTION < LIMIT) {
      STATE = OPTIONS == 7 ? WITHSAVE[OPTION] : WITHOUTSAVE[OPTION];
    } else {
      STATE = EXTRAS[OPTION-LIMIT];
    }
    printf("\n**********\n%s - %d:%d\n**********\n", STATE.label, OPTION, LIMIT);

    hud_volume();
    hud_brightness();    
  }
//}#pragma endregion Display

//{#pragma region Init
  void hud_init() {
    if(!INIT) {
      size = 320 * 30 * sizeof(uint16_t);
      buffer = (uint16_t *)malloc(size);
      if (!buffer) abort();
      OPTION = 0;
      OPTIONS = SAVED ? 8 : 5;
      hud_theme();
      GUI = THEMES[USER];
      STATE = OPTIONS == 8 ? WITHSAVE[OPTION] : WITHOUTSAVE[OPTION];
      INIT = true;
      hud_debug("HUD - INIT");
    }
  }

  void hud_deinit() {
    size = 0;
    buffer = realloc(buffer, size);
    free(buffer);
    INIT = false;
    hud_debug("HUD - DEINIT");
  }
//}#pragma endregion Init

//{#pragma region Menu
  void hud_menu(void) {
    VOLUME = odroid_audio_volume_get();
    #ifdef CONFIG_LCD_DRIVER_CHIP_RETRO_ESP32
      //volume = 8;
    #endif
    odroid_audio_terminate();
    hud_init();
    hud_debug("HUD - MENU");
    //hud_background();
    //hud_logo();
    usleep(200000);
    hud_background();
    hud_logo();
    hud_options();
    while(1) {

      odroid_input_gamepad_read(&gamepad);
      /*
        UP
      */
      if (gamepad.values[ODROID_INPUT_UP]) {
        OPTION--;
        if( OPTION < 0 ) { OPTION = OPTIONS-1; }
        hud_options();
        usleep(200000);
        //debounce(ODROID_INPUT_UP);
      }
      /*
        DOWN
      */
      if (gamepad.values[ODROID_INPUT_DOWN]) {
        OPTION++;
        if( OPTION >= OPTIONS ) { OPTION = 0; }
        hud_options();
        usleep(200000);
        //debounce(ODROID_INPUT_DOWN);
      }
      /*
        LEFT
      */
      if(gamepad.values[ODROID_INPUT_LEFT]) {
        if(OPTION == OPTIONS - 3) {
          if(VOLUME > 0) {
            VOLUME--;
            odroid_settings_Volume_set(VOLUME);
            hud_options();
            usleep(200000);
          }
        }          
        if(OPTION == OPTIONS - 3 + 1) {
          if(BRIGHTNESS > 0) {
            BRIGHTNESS--;
            odroid_settings_Backlight_set(BRIGHTNESS);
            hud_options();
            usleep(200000);
          }
        }        
      }

      /*
        RIGHT
      */
      if(gamepad.values[ODROID_INPUT_RIGHT]) {
        if(OPTION == OPTIONS - 3) {
          if(VOLUME < 8) {
            VOLUME++;
            odroid_settings_Volume_set(VOLUME);
            hud_options();
            usleep(200000);
          }
        }
        if(OPTION == OPTIONS - 3 + 1) {
          if(BRIGHTNESS < (BRIGHTNESS_COUNT-1)) {
            BRIGHTNESS++;
            odroid_settings_Backlight_set(BRIGHTNESS);
            hud_options();
            usleep(200000);
          }
        }        
      }       
      /*
        BUTTON B
      */
      if (gamepad.values[ODROID_INPUT_B]) {
        STATE.action = 0;
        ili9341_clear(0);
        debounce(ODROID_INPUT_B);
        return 0;
      }
      /*
        BUTTON A
      */
      if (gamepad.values[ODROID_INPUT_A]) {
        odroid_audio_volume_set(VOLUME);
        hud_debug(STATE.label);
        ACTION = STATE.action;
        switch(ACTION) {
          case 0: // "Resume Game"
            ili9341_clear(0);
            return 0;
          break;
          case 1: // "Restart Game"
            //odroid_settings_StartAction_set(1);
            forceConsoleReset = true;
            esp_restart();
          break;
          case 2: // "Reload Game"
            esp_restart();
          break;
          case 3: // "Save Game"
          case 4: // "Overwrite Game"
            ili9341_clear(0);
            return 0;
          break;
          case 5: // "Delete Save"
            hud_progress("Deleting...", true);
            hud_prepare_delete(1);
            ili9341_clear(0);
            esp_restart();
            //return 0;
          break;
          case 6: // "Exit Game"
            odroid_system_application_set(0);
            esp_restart();
          break;
        }
        debounce(ODROID_INPUT_A);
      }
    }
  }
//}#pragma endregion Menu
