#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_partition.h"
#include "driver/i2s.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "esp_ota_ops.h"

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned long int uint32;

#include "../components/lynx/handy.h"
#include "../components/lynx/myadd.h"

#include "../components/odroid/odroid_settings.h"
#include "../components/odroid/odroid_audio.h"
#include "../components/odroid/odroid_input.h"
#include "../components/odroid/odroid_system.h"
#include "../components/odroid/odroid_display.h"
#include "../components/odroid/odroid_sdcard.h"
#include "../components/odroid/odroid_ui.h"

#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CONFIG_IN_GAME_MENU_YES
    #include <dirent.h>
    #include "../components/odroid/odroid_hud.h"
    int ACTION;
#endif

#define NOINLINE  __attribute__ ((noinline))

#define MY_LYNX_INCLUDE_SAVE_TO_SD

#define PALETTE_SIZE 256

const char* SD_BASE_PATH = "/sd";

//#define AUDIO_SAMPLE_RATE (32000)
#define AUDIO_SAMPLE_RATE (22050)

#define AUDIO_BUFFER_SIZE 2756

struct audio_meta {
    //uint8_t *buffer;
    short *buffer;
    int length;
};

uint32_t *lynx_mColourMap;
//uint16 palette[PALETTE_SIZE];
uint16 *palette = NULL;
uint16_t* framebuffer[2]; // uint8_t*
int currentFramebuffer = 0;

//extern unsigned char *gAudioBuffer;
//extern unsigned char *gAudioBufferPointer2;
extern short *gAudioBuffer;
extern short *gAudioBufferPointer2;

struct audio_meta audio_update1 = {0,};
struct audio_meta audio_update2 = {0,};
struct audio_meta *audio_update = &audio_update1;

spi_flash_mmap_handle_t hrom;

QueueHandle_t vidQueue;
TaskHandle_t videoTaskHandle;
QueueHandle_t audioQueue;
TaskHandle_t audioTaskHandle;

odroid_volume_level Volume;
odroid_battery_state battery;

bool scaling_enabled = true;
bool config_ui_stats = false;
int8_t filtering = 0;
bool previous_scaling_enabled = true;
uint8_t previous_rotate = 0;
uint8_t previous_filtering = 0;

volatile bool videoTaskIsRunning = false;

const char *odroidgo_rom_path = "/sd/roms/lynx";
retro_pixel_format odroidgo_fmt = RETRO_PIXEL_FORMAT_RGB565;
retro_game_info odroid_game;
retro_system_info retro_info;
int16_t retrolib_input_state_t = 0;

uint startTime;
uint stopTime;
uint totalElapsedTime;
int frame;
volatile float fps_ui = 0;
uint8_t rotate = 0;

typedef void (ODROID_UI_CALLCONV *odroid_display_func_def)(uint8_t* buffer, uint32_t* myPalette);

bool display_func_change = false;
odroid_display_func_def odroid_display_func;

// functions
extern void SaveState();
extern void LoadState();


void update_display_func() {
  previous_scaling_enabled = scaling_enabled;
  previous_filtering = filtering;
  previous_rotate = rotate;

  if (!scaling_enabled) {
     odroid_display_func = ili9341_write_frame_lynx_v2_original;
  } else {
    switch (filtering) {
    case 0:
        odroid_display_func = ili9341_write_frame_lynx_v2_mode0;
    break;
    case 1:
        odroid_display_func = ili9341_write_frame_lynx_v2_mode1;
    break;
    case 2:
        odroid_display_func = ili9341_write_frame_lynx_v2_mode2;
    break;
    case 3:
        odroid_display_func = ili9341_write_frame_lynx_v2_mode3;
    break;
    }
  }
  display_func_change = false;
}

#define TASK_BREAK (void*)1

#ifdef MY_VIDEO_MODE_V1

#define VID_TASK(func) \
    uint8_t* param; \
    videoTaskIsRunning = true; \
    printf("%s: STARTED\n", __func__); \
     \
    while(1) \
    { \
        xQueuePeek(vidQueue, &param, portMAX_DELAY); \
 \
        if (param == TASK_BREAK) \
            break; \
 \
        odroid_display_lock(); \
        func(param, lynx_mColourMap); \
        odroid_display_unlock(); \
        /* odroid_input_battery_level_read(&battery);*/ \
        xQueueReceive(vidQueue, &param, portMAX_DELAY); \
    } \
    xQueueReceive(vidQueue, &param, portMAX_DELAY); \
    /*odroid_display_lock();*/ \
    /*odroid_display_show_hourglass();*/ \
    /*odroid_display_unlock();*/ \
    videoTaskIsRunning = false; \
    printf("%s: FINISHED\n", __func__); \
    vTaskDelete(NULL); \
    while (1) {}


void videoTask_mode0(void *arg) { VID_TASK(ili9341_write_frame_lynx_v2_mode0) }
void videoTask_mode1(void *arg) { VID_TASK(ili9341_write_frame_lynx_v2_mode1) }
void videoTask_mode2(void *arg) { VID_TASK(ili9341_write_frame_lynx_v2_mode2) }
void videoTask_mode3(void *arg) { VID_TASK(ili9341_write_frame_lynx_v2_mode3) }
void videoTask_mode_original(void *arg) { VID_TASK(ili9341_write_frame_lynx_v2_original) }
void videoTask_mode_original_rotate_R(void *arg) { VID_TASK(ili9341_write_frame_lynx_v2_original_rotate_R) }
void videoTask_mode_original_rotate_L(void *arg) { VID_TASK(ili9341_write_frame_lynx_v2_original_rotate_L) }
void videoTask_mode_original_mode0_rotate_R(void *arg) { VID_TASK(ili9341_write_frame_lynx_v2_mode0_rotate_R) }
void videoTask_mode_original_mode0_rotate_L(void *arg) { VID_TASK(ili9341_write_frame_lynx_v2_mode0_rotate_L) }

#endif

NOINLINE void update_display_task()
{
    printf("%s: Step #001\n", __func__);
    if (videoTaskIsRunning)
    {
        printf("VIDEO: Task: Stop\n");
        uint16_t* param = TASK_BREAK;
        xQueueSend(vidQueue, &param, portMAX_DELAY);
        while (videoTaskIsRunning) { vTaskDelay(1); }
        printf("VIDEO: Task: Stop done\n");

        printf("VIDEO: Clear display\n");
        //odroid_display_lock();
        ili9341_write_frame_lynx(NULL, NULL, false);
        //odroid_display_unlock();
    }
    printf("%s: Step #002\n", __func__);
    TaskFunction_t taskFunc = &videoTask_mode0;

      previous_scaling_enabled = scaling_enabled;
      previous_filtering = filtering;
      previous_rotate = rotate;

      my_setbutton_mapping(rotate);

      if (!scaling_enabled) {
         switch (rotate) {
         case 0: taskFunc = &videoTask_mode_original; break;
         case 1: taskFunc = &videoTask_mode_original_rotate_R; break;
         case 2: taskFunc = &videoTask_mode_original_rotate_L; break;
         }
      } else {
        if (rotate) {
              switch (rotate) {
                case 0: taskFunc = &videoTask_mode_original; break;
                case 1: taskFunc = &videoTask_mode_original_mode0_rotate_R; break;
                case 2: taskFunc = &videoTask_mode_original_mode0_rotate_L; break;
             }
        } else {
          // filtering;
          switch (filtering) {
          case 0:
            taskFunc = &videoTask_mode0;
            break;
          case 1:
            taskFunc = &videoTask_mode1;
            break;
          case 2:
            taskFunc = &videoTask_mode2;
            break;
          case 3:
            taskFunc = &videoTask_mode3;
            break;
          default:
            taskFunc = &videoTask_mode0;
            break;
          }
        }
      }
    printf("%s: Step #003\n", __func__);

    printf("\n************\n");
    printf("\nVIDEO: Task: Start\n");

#ifdef CONFIG_LCD_DRIVER_CHIP_RETRO_ESP32
    //taskFunc = &videoTask_mode_original_mode0_rotate_R;
#endif
    xTaskCreatePinnedToCore(taskFunc, "videoTask", 1024 * 4, NULL, 5, &videoTaskHandle, 1);
    while (!videoTaskIsRunning) { vTaskDelay(1); }
    printf("\nVIDEO: Task: Start done\n");
    printf("\n************\n");
}


volatile bool AudioTaskIsRunning = false;
void audioTask(void* arg)
{
  // sound
  struct audio_meta* param;

  AudioTaskIsRunning = true;
  while(1)
  {
    xQueuePeek(audioQueue, &param, portMAX_DELAY);

    if (param == NULL)
    {
        break;
    }
    else
    {
       odroid_audio_submit((short*)param->buffer, param->length - 1);
    }
    xQueueReceive(audioQueue, &param, portMAX_DELAY);
  }
  printf("audioTask: exiting.\n");
  odroid_audio_terminate();
  AudioTaskIsRunning = false;
  vTaskDelete(NULL);

  while (1) {}
}

//Read an unaligned byte.
char unalChar(const unsigned char *adr) {
	//See if the byte is in memory that can be read unaligned anyway.
	if (!(((int)adr)&0x40000000)) return *adr;
	//Nope: grab a word and distill the byte.
	int *p=(int *)((int)adr&0xfffffffc);
	int v=*p;
	int w=((int)adr&3);
	if (w==0) return ((v>>0)&0xff);
	if (w==1) return ((v>>8)&0xff);
	if (w==2) return ((v>>16)&0xff);
	if (w==3) return ((v>>24)&0xff);

    abort();
    return 0;
}

NOINLINE void PowerDown()
{
    uint16_t* param = TASK_BREAK;
    void *exitAudioTask = NULL;

    // Clear audio to prevent studdering
    printf("PowerDown: stopping audio.\n");
    // odroid_audio_terminate();
    xQueueSend(audioQueue, &exitAudioTask, portMAX_DELAY);
    while (AudioTaskIsRunning) {}

    // Stop tasks
    printf("PowerDown: stopping tasks.\n");

    xQueueSend(vidQueue, &param, portMAX_DELAY);
    while (videoTaskIsRunning) { vTaskDelay(1); }


    // state
    printf("PowerDown: Saving state.\n");
    SaveState();

    // LCD
    printf("PowerDown: Powerdown LCD panel.\n");
    ili9341_poweroff();

    odroid_system_sleep();

    // Should never reach here
    abort();
}

NOINLINE void DoMenuHome(bool save)
{
    uint16_t* param = TASK_BREAK;
    void *exitAudioTask = NULL;

    #ifdef CONFIG_IN_GAME_MENU_YES
        odroid_display_lock();
        hud_menu();
        printf("\nACTION:%d\n", ACTION);
        switch(ACTION) {
            case 3:
            case 4:
                hud_progress("Saving...", true);
                hud_deinit();
                odroid_display_unlock();
                xQueueSend(audioQueue, &exitAudioTask, portMAX_DELAY);
                while (AudioTaskIsRunning) {}
                xQueueSend(vidQueue, &param, portMAX_DELAY);
                while (videoTaskIsRunning) { vTaskDelay(1); }
                SaveState();
                ili9341_clear(0);
                esp_restart();
            break;
            case 5:
                printf("\nDELETE ROM\n");
            break;
        }
        ili9341_clear(0);
        odroid_display_unlock();
    #else
        // Clear audio to prevent studdering
        printf("PowerDown: stopping audio.\n");
        //odroid_audio_terminate();
        xQueueSend(audioQueue, &exitAudioTask, portMAX_DELAY);
        while (AudioTaskIsRunning) {}

        // Stop tasks
        printf("PowerDown: stopping tasks.\n");

        xQueueSend(vidQueue, &param, portMAX_DELAY);
        while (videoTaskIsRunning) { vTaskDelay(1); }

        odroid_gamepad_state joystick;
        odroid_input_gamepad_read(&joystick);
        if (!joystick.values[ODROID_INPUT_START] && !save)
        {
           save = odroid_ui_ask(" Save current state? Yes=A ; No=B ");
        }
        odroid_display_show_hourglass();

        if (save) gpio_set_level(GPIO_NUM_2, 1);

        // state
        printf("PowerDown: Saving state.\n");
        if (!joystick.values[ODROID_INPUT_START] && save) {
           SaveState();
        }
        #ifdef MY_LYNX_INTERNAL_GAME_SELECT
          odroid_settings_ForceInternalGameSelect_set(1);
        #endif

        gpio_set_level(GPIO_NUM_2, 0);
        // Set menu application
        odroid_system_application_set(0);
        // Reset
        esp_restart();
    #endif
}
/*
void system_manage_sram(uint8 *sram, int slot, int mode)
{
    printf("system_manage_sram\n");
    //sram_load();
}
*/

extern uint32    gAudioEnabled;

void menu_lynx_audio_update(odroid_ui_entry *entry) {
    if (gAudioEnabled) {
        sprintf(entry->text, "%-9s: %s", "audio", "on");
    } else {
        sprintf(entry->text, "%-9s: %s", "audio", "off");
    }
}

odroid_ui_func_toggle_rc menu_lynx_audio_toggle(odroid_ui_entry *entry, odroid_gamepad_state *joystick) {
    gAudioEnabled = !gAudioEnabled;
    return ODROID_UI_FUNC_TOGGLE_RC_CHANGED;
}

void menu_lynx_rotate_update(odroid_ui_entry *entry) {
    switch (rotate)
    {
    case 0: sprintf(entry->text, "%-9s: %s", "rotate", "off"); break;
    case 1: sprintf(entry->text, "%-9s: %s", "rotate", "right"); break;
    case 2: sprintf(entry->text, "%-9s: %s", "rotate", "left"); break;
    }
}

odroid_ui_func_toggle_rc menu_lynx_rotate_toggle(odroid_ui_entry *entry, odroid_gamepad_state *joystick) {
    rotate = (rotate+1)%3;
    return ODROID_UI_FUNC_TOGGLE_RC_CHANGED;
}

void menu_lynx_filtering_update(odroid_ui_entry *entry) {
    switch(filtering) {
    case 0:
        sprintf(entry->text, "%-9s: %s", "filter", "none");
        break;
    case 1:
        sprintf(entry->text, "%-9s: %s", "filter", "H");
        break;
    case 2:
        sprintf(entry->text, "%-9s: %s", "filter", "V");
        break;
    case 3:
        sprintf(entry->text, "%-9s: %s", "filter", "HV");
        break;
    }
}

odroid_ui_func_toggle_rc menu_lynx_filtering_toggle(odroid_ui_entry *entry, odroid_gamepad_state *joystick) {
    filtering++;
    if (filtering>3) filtering = 0;
    return ODROID_UI_FUNC_TOGGLE_RC_CHANGED;
}

void menu_lynx_init(odroid_ui_window *window) {
    odroid_ui_create_entry(window, &menu_lynx_audio_update, &menu_lynx_audio_toggle);
    odroid_ui_create_entry(window, &menu_lynx_rotate_update, &menu_lynx_rotate_toggle);
    odroid_ui_create_entry(window, &menu_lynx_filtering_update, &menu_lynx_filtering_toggle);
}

inline void update_ui_fps() {
	stopTime = xthal_get_ccount();
    int elapsedTime;
    if (stopTime > startTime)
      elapsedTime = (stopTime - startTime);
    else
      elapsedTime = ((uint64_t)stopTime + (uint64_t)0xffffffff) - (startTime);

    totalElapsedTime += elapsedTime;
    ++frame;

    if (frame == 60)
    {
      float seconds = totalElapsedTime / (CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ * 1000000.0f);
      float fps = frame / seconds;
      fps_ui = fps;
      printf("FPS:%f, BATTERY:%d [%d]\n", fps, battery.millivolts, battery.percentage);

      //printf("HEAP:0x%x, FPS:%f, BATTERY:%d [%d]\n", esp_get_free_heap_size(), fps, battery.millivolts, battery.percentage);

      //vTaskGetRunTimeStats(pmem);
      //printf(pmem);

      frame = 0;
      totalElapsedTime = 0;
      /*if (config_ui_stats) {
        update_ui_fps_text(fps);
      }*/
    }
    startTime = stopTime;
    // usleep(20*1000UL);
}


void odroid_retro_log(retro_log_level level,
      const char *fmt, ...) {
	va_list(args);
	switch(level) {
	case RETRO_LOG_DEBUG:
	  printf("DEBUG - ");
      break;
    case RETRO_LOG_INFO:
      printf("INFO  - ");
      break;
    case RETRO_LOG_WARN:
      printf("WARN  - ");
      break;
    case RETRO_LOG_ERROR:
      printf("ERROR - ");
      break;
    default:
      printf("LOG%d - ", level);
    break;
	};
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	printf("\n");
}

//expected 'retro_environment_t {aka _Bool (*)(unsigned int,  void *)}'
//but argument is of type '_Bool (*)(unsigned int,  void **)' RETRO_API void retro_set_environment(retro_environment_t);

bool odroidgo_env(unsigned int cmd, void *_data) {
   void **data = (void**)_data;
   if (!data) {
     printf("CMD '%d'. Data is null!\n", cmd);
   }
   switch(cmd) {
   case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
   {
    retro_log_callback *log = (retro_log_callback *)data;
   	log->log = &odroid_retro_log;
   }
   break;
   case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
    *data = &odroidgo_fmt;
    break;
   case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    *data = (char*)odroidgo_rom_path;
   break;
   case RETRO_ENVIRONMENT_SET_VARIABLES:
     // VARs from lynx emu
   break;
   case RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS:
      // serialization_quirks
   break;
   case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
     //
   break;
   case RETRO_ENVIRONMENT_GET_VARIABLE:
     *data = NULL;
     return false;
   case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
     return false;
   default:
    data = NULL;
    printf("No data for '%d'!\n", cmd);
    abort();
   return false;
   };
   return true;
}

void odroid_retro_input_poll_t() {
   //
}

void odroid_retro_audio_sample_t(int16_t left, int16_t right) {
   printf("odroid_retro_audio_sample_t\n");
}

size_t odroid_retro_audio_sample_batch_t(const int16_t *data, size_t frames) {
   // Process audio
#ifdef MY_AUDIO_MODE_V1
    //printf("Audio: Frames: %u\n", frames);
    audio_update->length = frames;
    // xQueueSend(audioQueue, &audio_update, portMAX_DELAY);
    // *** odroid_audio_submit(audio_update->buffer, audio_update->length - 1);

    odroid_audio_submit(audio_update->buffer, audio_update->length - 1);

    audio_update = (audio_update==&audio_update1)?&audio_update2:&audio_update1;
    gAudioBuffer = audio_update->buffer;
#else
    odroid_audio_submit((short*)data, frames - 1);
#endif
   return 0;
}

    uint16_t menuButtonFrameCount;
    odroid_gamepad_state previousState;
    bool ignoreMenuButton, menu_restart;

void process_keys(odroid_gamepad_state *joystick)
{
#ifndef MY_KEYS_IN_CALLBACK
        retrolib_input_state_t = 0;
        if (joystick->values[ODROID_INPUT_A]) {
                retrolib_input_state_t |= 1 << RETRO_DEVICE_ID_JOYPAD_A;
        }
        if (joystick->values[ODROID_INPUT_B]) {
                retrolib_input_state_t |= 1 << RETRO_DEVICE_ID_JOYPAD_B;
        }
        if (joystick->values[ODROID_INPUT_LEFT]) {
                retrolib_input_state_t |= 1 << RETRO_DEVICE_ID_JOYPAD_LEFT;
        }
        if (joystick->values[ODROID_INPUT_RIGHT]) {
                retrolib_input_state_t |= 1 << RETRO_DEVICE_ID_JOYPAD_RIGHT;
        }
        if (joystick->values[ODROID_INPUT_UP]) {
                retrolib_input_state_t |= 1 << RETRO_DEVICE_ID_JOYPAD_UP;
        }
        if (joystick->values[ODROID_INPUT_DOWN]) {
                retrolib_input_state_t |= 1 << RETRO_DEVICE_ID_JOYPAD_DOWN;
        }
        if (joystick->values[ODROID_INPUT_START]) {
                retrolib_input_state_t |= 1 << RETRO_DEVICE_ID_JOYPAD_START;
        }
        if (joystick->values[ODROID_INPUT_SELECT]) {
                retrolib_input_state_t |= 1 << RETRO_DEVICE_ID_JOYPAD_L;
        }
        // { RETRO_DEVICE_ID_JOYPAD_R, BUTTON_OPT2 },
#endif
        if (ignoreMenuButton)
        {
            ignoreMenuButton = previousState.values[ODROID_INPUT_MENU];
        }

        if (!ignoreMenuButton && previousState.values[ODROID_INPUT_MENU] && joystick->values[ODROID_INPUT_MENU])
        {
            ++menuButtonFrameCount;
        }
        else
        {
            menuButtonFrameCount = 0;
        }

        // Note: this will cause an exception on 2nd Core in Debug mode
        /*
        if (menuButtonFrameCount > 60 * 2)
        {
            // Turn Blue LED on. Power state change turns it off
            odroid_system_led_set(1);
            PowerDown();
        }
        */

        if (menuButtonFrameCount > 60 * 1)
        {
            DoMenuHome(true);
        }

        if (!ignoreMenuButton && previousState.values[ODROID_INPUT_MENU] && !joystick->values[ODROID_INPUT_MENU])
        {
            DoMenuHome(false);
        }

        if (joystick->values[ODROID_INPUT_VOLUME] || menu_restart)
        {
            menu_restart = odroid_ui_menu_ext(menu_restart, &menu_lynx_init);
            if (previous_scaling_enabled != scaling_enabled ||
                previous_filtering != filtering ||
                previous_rotate != rotate) {
              if (!menu_restart)
              {
                update_display_task();
              }

              // display_func_change = true;
              //odroid_display_lock();
              //ili9341_write_frame_lynx(NULL, NULL, false);
              //update_display_func();
              //odroid_display_unlock();
            }
        }

        // Scaling
        if (joystick->values[ODROID_INPUT_START] && !previousState.values[ODROID_INPUT_RIGHT] && joystick->values[ODROID_INPUT_RIGHT])
        {
            scaling_enabled = !scaling_enabled;
            odroid_settings_ScaleDisabled_set(ODROID_SCALE_DISABLE_SMS, scaling_enabled ? 0 : 1);
        }
}

#ifdef MY_KEYS_IN_CALLBACK
int16_t odroid_retro_input_state_t(unsigned port, unsigned device,
      unsigned index, unsigned id) {
      int16_t rc;
      switch(id) {
        case RETRO_DEVICE_ID_JOYPAD_A:
            rc = previousState.values[ODROID_INPUT_A];
            break;
        case RETRO_DEVICE_ID_JOYPAD_B:
            rc = previousState.values[ODROID_INPUT_B];
            break;
        case RETRO_DEVICE_ID_JOYPAD_LEFT:
            rc = previousState.values[ODROID_INPUT_LEFT];
            break;
        case RETRO_DEVICE_ID_JOYPAD_RIGHT:
            rc = previousState.values[ODROID_INPUT_RIGHT];
            break;
        case RETRO_DEVICE_ID_JOYPAD_UP:
            rc = previousState.values[ODROID_INPUT_UP];
            break;
        case RETRO_DEVICE_ID_JOYPAD_DOWN:
            rc = previousState.values[ODROID_INPUT_DOWN];
            break;
        case RETRO_DEVICE_ID_JOYPAD_START:
            rc = previousState.values[ODROID_INPUT_START];
            break;
        case RETRO_DEVICE_ID_JOYPAD_L:
            rc = previousState.values[ODROID_INPUT_SELECT];
            break;
        // { RETRO_DEVICE_ID_JOYPAD_R, BUTTON_OPT2 },
        default:
            rc = 0;
            break;
      }
      return rc;
}
#else
int16_t odroid_retro_input_state_t(unsigned port, unsigned device,
      unsigned index, unsigned id) {
      return (1<<id) & retrolib_input_state_t;
      //return retrolib_input_state_t;
      //return ((retrolib_input_state_t&0x00ff) << 8) | ((retrolib_input_state_t&0xff00) >> 8);
}
#endif



// Ok
#define FRAME_SKIP_PL1 3
// Max speed
//#define FRAME_SKIP_PL1 10
// No frameskip
//#undef FRAME_SKIP_PL1

#ifdef FRAME_SKIP_PL1
bool skipNextFrame = true;
void odroid_retro_video_refresh_t(const void *data, unsigned width,
      unsigned height, size_t pitch) {
	 if ((frame%FRAME_SKIP_PL1)==1) {
     	xQueueSend(vidQueue, &data, portMAX_DELAY);
     	skipNextFrame = true;
     } else if ((frame%FRAME_SKIP_PL1)==0) {
     	skipNextFrame = false;
     } else {
     	skipNextFrame = true;
     }
     update_ui_fps();

#ifdef MY_KEYS_IN_VIDEO
        odroid_gamepad_state joystick;
        odroid_input_gamepad_read(&joystick);

        process_keys(&joystick);
        previousState = joystick;
#endif
}
#else
bool skipNextFrame = false;
void odroid_retro_video_refresh_t(const void *data, unsigned width,
      unsigned height, size_t pitch) {
      xQueueSend(vidQueue, &data, portMAX_DELAY);
      update_ui_fps();
#ifdef MY_KEYS_IN_VIDEO
        odroid_gamepad_state joystick;
        odroid_input_gamepad_read(&joystick);

        process_keys(&joystick);
        previousState = joystick;
#endif
}
#endif

void odroidgo_retro_init(void) {
	printf("odroidgo_init\n");
	odroid_display_unlock();
	char *rc = odroid_ui_choose_file("/sd/roms/lynx", "lnx");
	odroid_display_lock();
	if (rc) {
	   printf("File: %s\n", rc);
	   odroid_game.path = rc;
	} else {
	   printf("File: ---\n");
	   odroid_game.path = "/sd/roms/lynx/_debug.lnx";
	}

	odroid_game.data = NULL;
    odroid_game.size = 0;
    odroid_game.meta = "dummy";

	retro_set_environment(&odroidgo_env);
	retro_set_audio_sample(&odroid_retro_audio_sample_t);
	retro_set_video_refresh(&odroid_retro_video_refresh_t);
	retro_set_audio_sample_batch(&odroid_retro_audio_sample_batch_t);
	// retro_set_controller_port_device();
}

void odroidgo_retro_init_post() {
	retro_set_input_poll(&odroid_retro_input_poll_t);
	retro_set_input_state(&odroid_retro_input_state_t);
}

/*
#include "esp_heap_trace.h"
#define NUM_RECORDS 1000
static heap_trace_record_t trace_record[NUM_RECORDS]; // This buffer must be in internal RAM
*/

void dump_heap_info_short() {
    printf("LARGEST: 8BIT: %u\n", heap_caps_get_largest_free_block( MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT ));
    printf("LARGEST: 32BIT: %u\n", heap_caps_get_largest_free_block( MALLOC_CAP_INTERNAL | MALLOC_CAP_32BIT ));
    printf("LARGEST: MALLOC_CAP_INTERNAL: %u\n", heap_caps_get_largest_free_block( MALLOC_CAP_INTERNAL ));
    printf("LARGEST: MALLOC_CAP_DEFAULT: %u\n", heap_caps_get_largest_free_block( MALLOC_CAP_DEFAULT ));
}

void app_loop(void)
{
    menuButtonFrameCount = 0;
    menu_restart = false;
    odroid_input_gamepad_read(&previousState);
    ignoreMenuButton = previousState.values[ODROID_INPUT_MENU];

#ifdef MY_RETRO_LOOP
    retro_run_endless();
#else
    while (true)
    {
#ifndef MY_KEYS_IN_VIDEO
        odroid_gamepad_state joystick;
        odroid_input_gamepad_read(&joystick);
        process_keys(&joystick);
#endif
        retro_run();
#ifndef MY_KEYS_IN_VIDEO
        previousState = joystick;
#endif
    }
#endif
}

NOINLINE void app_init(void)
{
printf("lynx-handy (%s-%s).\n", COMPILEDATE, GITREV);
    // ESP_ERROR_CHECK( heap_trace_init_standalone(trace_record, NUM_RECORDS) );

    framebuffer[0] = heap_caps_malloc(160 * 102 * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if (!framebuffer[0]) abort();
    printf("app_main: framebuffer[0]=%p\n", framebuffer[0]);

    framebuffer[1] = heap_caps_malloc(160 * 102 * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if (!framebuffer[1]) abort();
    printf("app_main: framebuffer[1]=%p\n", framebuffer[1]);

    //audio_update1.buffer = MY_MEM_ALLOC_FAST_EXT(unsigned short, AUDIO_BUFFER_SIZE, 1);
    //audio_update2.buffer = MY_MEM_ALLOC_FAST_EXT(unsigned short, AUDIO_BUFFER_SIZE, 1);
    audio_update1.buffer = MY_MEM_ALLOC_FAST_EXT(short, AUDIO_BUFFER_SIZE, 1);
    audio_update2.buffer = MY_MEM_ALLOC_FAST_EXT(short, AUDIO_BUFFER_SIZE, 1);

    gAudioBuffer = audio_update1.buffer;
#ifdef MY_AUDIO_MODE_V1
    gAudioBufferPointer2 = gAudioBuffer;
#endif

    QuickSaveSetBuffer( MY_MEM_ALLOC_SLOW(void, 512*1024) );

    // ESP_ERROR_CHECK( heap_trace_start(HEAP_TRACE_LEAKS) );

    nvs_flash_init();

    odroid_system_init();

    // Joystick.
    odroid_input_gamepad_init();
    odroid_input_battery_level_init();


    // Boot state overrides
    bool forceConsoleReset = false;


    ili9341_prepare();


    // Disable LCD CD to prevent garbage
    const gpio_num_t LCD_PIN_NUM_CS = GPIO_NUM_5;

    gpio_config_t io_conf = { 0 };
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LCD_PIN_NUM_CS);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;

    gpio_config(&io_conf);
    gpio_set_level(LCD_PIN_NUM_CS, 1);

    esp_sleep_source_t cause = esp_sleep_get_wakeup_cause();
    switch (cause)
    {
        case ESP_SLEEP_WAKEUP_EXT0:
        {
            printf("app_main: ESP_SLEEP_WAKEUP_EXT0 deep sleep reset\n");
            break;
        }

        case ESP_SLEEP_WAKEUP_EXT1:
        case ESP_SLEEP_WAKEUP_TIMER:
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
        case ESP_SLEEP_WAKEUP_ULP:
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        {
            printf("app_main: Unexpected deep sleep reset: %d\n", cause);

            odroid_gamepad_state bootState = odroid_input_read_raw();

            if (bootState.values[ODROID_INPUT_MENU])
            {
                // Force return to factory app to recover from
                // ROM loading crashes

                // Set menu application
                odroid_system_application_set(0);

                // Reset
                esp_restart();
            }

            if (bootState.values[ODROID_INPUT_START])
            {
                // Reset emulator if button held at startup to
                // override save state
                forceConsoleReset = true;
            }
        }
            break;

        default:
            printf("app_main: Not a deep sleep reset\n");
            break;
    }

    if (odroid_settings_StartAction_get() == ODROID_START_ACTION_RESTART)
    {
        forceConsoleReset = true;
        odroid_settings_StartAction_set(ODROID_START_ACTION_NORMAL);
    }


    ili9341_init();

    char* cartName = odroid_settings_RomFilePath_get();
    printf("%s: filename='%s'\n", __func__, cartName);

    ili9341_write_frame_lynx(NULL, NULL, false);

    odroid_audio_init(odroid_settings_AudioSink_get(), AUDIO_SAMPLE_RATE);

    vidQueue = xQueueCreate(1, sizeof(uint16_t*));
    //xTaskCreatePinnedToCore(&videoTask, "videoTask", 1024 * 4, NULL, 5, &videoTaskHandle, 1);
    audioQueue = xQueueCreate(1, sizeof(uint16_t*));
    xTaskCreatePinnedToCore(&audioTask, "audioTask", 2048, NULL, 5, NULL, 1); //768


    esp_err_t r = odroid_sdcard_open(SD_BASE_PATH);
    if (r != ESP_OK)
    {
        odroid_display_show_sderr(ODROID_SD_ERR_NOCARD);
        abort();
    }
    #ifdef CONFIG_IN_GAME_MENU_YES
        hud_debug(odroid_util_GetFileName(cartName));
        char* save_name = odroid_util_GetFileName(cartName);
        hud_check_saves(odroid_util_GetFileName(cartName));
    #endif

    odroid_display_lock();
    odroid_display_drain_spi();

    // ESP_ERROR_CHECK( heap_trace_start(HEAP_TRACE_LEAKS) );
    printf("LYNX-hande: 001\n");
    printf("Version: %d; %d\n", RETRO_API_VERSION, RETRO_MEMORY_VIDEO_RAM);
    odroidgo_retro_init();
    //retro_init();
    printf("LYNX-hande: 002\n");

    printf("Retro: API: %d\n", retro_api_version());
    retro_get_system_info(&retro_info);
    printf("Retro: Info.library_name      : %s\n", retro_info.library_name);
    printf("Retro: Info.library_version   : %s\n", retro_info.library_version);
    printf("Retro: Info.need_fullpath     : %d\n", retro_info.need_fullpath);
    printf("Retro: Info.valid_extensions  : %s\n", retro_info.valid_extensions);
    printf("Retro: Info.block_extract     : %d\n", retro_info.block_extract);

    odroid_display_unlock();

    if (!retro_load_game(&odroid_game)) {
        printf("LYNX-handy: 003 Gameload: Error\n");
    } else {
       printf("LYNX-handy: 003 Gameload: Ok\n");
    }

    printf("LYNX-handy: 003\n");

    // FIXME: Do some EMU stuff
    // system_reset();

    // Restore state
    LoadState(cartName);

    if (forceConsoleReset)
    {
        // Reset emulator if button held at startup to
        // override save state
        printf("%s: forceConsoleReset=true\n", __func__);
        // system_reset();
    }
    odroidgo_retro_init_post();

    // ESP_ERROR_CHECK( heap_trace_stop() );
    // heap_trace_dump();

    odroid_gamepad_state previousState;
    odroid_input_gamepad_read(&previousState);

    totalElapsedTime = 0;
    frame = 0;
    scaling_enabled = odroid_settings_ScaleDisabled_get(ODROID_SCALE_DISABLE_SMS) ? false : true;

    odroid_ui_debug_enter_loop();
    startTime = xthal_get_ccount();

    dump_heap_info_short();
    printf("unsigned char   : %u\n", sizeof(unsigned char));
    printf("unsigned int    : %u\n", sizeof(unsigned int));
    printf("unsigned long   : %u\n", sizeof(unsigned long));

    /*
    uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    for (int i = 0; i < 64; i++) {
    void *tmp = heap_caps_malloc(1024, caps);// MALLOC_CAP_SPIRAM
    if (!tmp) {
        printf("PRE-ALLOC failed #%d\n", i+1);
        odroid_ui_debug_enter_loop();
        printf("LARGEST: %u\n", heap_caps_get_largest_free_block( caps ));
        abort();
    }
        printf("PRE-ALLOC test #%d: OK: %p\n", i+1, tmp);
        printf("LARGEST: %u\n", heap_caps_get_largest_free_block( caps ));
    }
    */
    if (!forceConsoleReset)
    {
       printf("\n***********\nLoadState(%s)\n***********\n", cartName);
       LoadState(cartName);
    }

    update_display_task();
}

void app_main(void)
{
    app_init();
    app_loop();
}

void *my_special_alloc(unsigned char speed, unsigned char bytes, unsigned long size) {
    uint32_t caps = (speed?MALLOC_CAP_INTERNAL:MALLOC_CAP_SPIRAM) |
      ( bytes==1?MALLOC_CAP_8BIT:MALLOC_CAP_32BIT);
      /*
    if (speed) {
        uint32_t max = heap_caps_get_largest_free_block(caps);
        if (max < size) {
            printf("ALLOC: Size: %u; Max FREE for internal is '%u'. Allocating in SPI RAM\n", (unsigned int)size, max);
            caps = MALLOC_CAP_SPIRAM | ( size==1?MALLOC_CAP_8BIT:MALLOC_CAP_32BIT);
        }
    } else {
      caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT;
    }
    */
    if (!speed) caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT;
    //if (!speed || size!=65536) caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT; // only RAM
    void *rc = heap_caps_malloc(size, caps);
    printf("ALLOC: Size: %-10u; SPI: %u; 32BIT: %u; RC: %p\n", (unsigned int)size, (caps&MALLOC_CAP_SPIRAM)!=0, (caps&MALLOC_CAP_32BIT)!=0, rc);
    if (!rc) { dump_heap_info_short(); abort(); }
    return rc;
}

void my_special_alloc_free(void *p) {
    printf("FREE: Size: -; RC: %p\n", p);
    if (p) heap_caps_free(p);
}

void SaveState()
{
#ifdef MY_LYNX_INCLUDE_SAVE_TO_SD
    // Save sram
    odroid_input_battery_monitor_enabled_set(0);
    odroid_system_led_set(1);

    char* romPath = odroid_settings_RomFilePath_get();
    if (romPath)
    {
        char* fileName = odroid_util_GetFileName(romPath);
        if (!fileName) abort();


        char* pathName = odroid_sdcard_create_savefile_path(SD_BASE_PATH, fileName);
        if (!pathName) abort();

        FILE* f = fopen(pathName, "w");
        if (f == NULL)
        {
            const char *path = "/sd/odroid/data/lynx";
            DIR* dir = opendir(path);
            if (dir)
            {
                closedir(dir);
                printf("%s: fopen save failed: '%s' ; Folder exists\n", __func__, pathName);
                abort();
            }
            if (mkdir(path, 01666) != 0)
            {
                printf("%s: fopen save failed: '%s' ; Folder doesn't exists. Could not create\n", __func__, pathName);
                abort();
            }
            f = fopen(pathName, "w");
            if (f == NULL)
            {
               printf("%s: fopen save failed: '%s'\n", __func__, pathName);
               abort();
            }
        }
        {
            uint8_t buf[8];
            memset(buf, 0, 8);
            buf[0] = gAudioEnabled&0xff;
            buf[1] = scaling_enabled&0xff;
            buf[2] = filtering&0xff;
            buf[3] = rotate&0xff;
            fwrite(buf,sizeof(uint8_t),8,f);
        }
        QuickSaveState(f);
        fclose(f);

        printf("%s: savestate OK.\n", __func__);

        free(pathName);
        free(fileName);
        free(romPath);
    } else
    {
       printf("ERROR!\n");
    }
#endif
}

void LoadState(const char* cartName)
{
#ifdef MY_LYNX_INCLUDE_SAVE_TO_SD
    char* romName = odroid_settings_RomFilePath_get();
    if (romName)
    {
        char* fileName = odroid_util_GetFileName(romName);
        if (!fileName) abort();

        char* pathName = odroid_sdcard_create_savefile_path(SD_BASE_PATH, fileName);
        if (!pathName) abort();

        FILE* f = fopen(pathName, "r");
        if (f == NULL)
        {
            printf("LoadState: fopen load failed\n");
        }
        else
        {
            printf("LoadState: loadstate OK.\n");

            {
                uint8_t buf[8];
                memset(buf, 0, 8);
                fread(buf,sizeof(uint8_t),8,f);
                gAudioEnabled = buf[0];
                scaling_enabled = buf[1];
                filtering = buf[2];
                rotate = buf[3];
            }
            QuickLoadState(f);
            fclose(f);
        }

        free(pathName);
        free(fileName);
        free(romName);
    }
    else
    {
      printf("ERROR\n");
    }
#endif
}

bool DoSaveState(const char* pathName) {
    return true;
}

bool DoLoadState(const char* pathName) {
    return true;
}
