// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "driver/rtc_io.h"

//Nes stuff wants to define this as well...
#undef false
#undef true
#undef bool


#include <math.h>
#include <string.h>
#include <noftypes.h>
#include <bitmap.h>
#include <nofconfig.h>
#include <event.h>
#include <gui.h>
#include <log.h>
#include <nes.h>
#include <nes_pal.h>
#include <nesinput.h>
#include <osd.h>
#include <stdint.h>
#include "driver/i2s.h"
#include "sdkconfig.h"
#include "../nofrendo/nes/nesstate.h"

#include "hourglass_empty_black_48dp.h"
#include "../odroid/odroid_settings.h"
#include "../odroid/odroid_audio.h"
#include "../odroid/odroid_system.h"
#include "../odroid/odroid_display.h"
#include "../odroid/odroid_input.h"


#define DEFAULT_SAMPLERATE   32000
#define  DEFAULT_FRAGSIZE     512

#define  DEFAULT_WIDTH        256
#define  DEFAULT_HEIGHT       NES_VISIBLE_HEIGHT

odroid_volume_level Volume;
odroid_battery_state battery;
int scaling_enabled = 1;
int previous_scaling_enabled = 1;

#ifdef CONFIG_IN_GAME_MENU_YES
    #include <dirent.h>
    #include "../odroid/odroid_hud.h"
    int ACTION;
#endif

//Seemingly, this will be called only once. Should call func with a freq of frequency,
int osd_installtimer(int frequency, void *func, int funcsize, void *counter, int countersize)
{
   return 0;
}


/*
** Audio
*/
static void (*audio_callback)(void *buffer, int length) = NULL;
#if CONFIG_SOUND_ENA
        QueueHandle_t queue;
        static int16_t *audio_frame;
#endif

void do_audio_frame() {
#if CONFIG_SOUND_ENA
        int remaining = DEFAULT_SAMPLERATE / NES_REFRESH_RATE;
        while(remaining)
        {
            int n=DEFAULT_FRAGSIZE;
            if (n>remaining) n=remaining;

            audio_callback(audio_frame, n); //get more data

            //16 bit mono -> 32-bit (16 bit r+l)
            for (int i=n-1; i>=0; i--)
            {
                int sample = (int)audio_frame[i];

                audio_frame[i*2]= (short)sample;
                audio_frame[i*2+1] = (short)sample;
            }

            odroid_audio_submit(audio_frame, n);

            remaining -= n;
        }
#endif
}

void osd_setsound(void (*playfunc)(void *buffer, int length))
{
   //Indicates we should call playfunc() to get more data.
   audio_callback = playfunc;
}

static void osd_stopsound(void)
{
   audio_callback = NULL;
}


static int osd_init_sound(void)
{
#if CONFIG_SOUND_ENA

    audio_frame=malloc(4*DEFAULT_FRAGSIZE);

    odroid_audio_init(odroid_settings_AudioSink_get(), DEFAULT_SAMPLERATE);

#endif

    audio_callback = NULL;

    return 0;
}

void osd_getsoundinfo(sndinfo_t *info)
{
   info->sample_rate = DEFAULT_SAMPLERATE;
   info->bps = 16;
}

/*
** Video
*/

static int init(int width, int height);
static void shutdown(void);
static int set_mode(int width, int height);
static void set_palette(rgb_t *pal);
static void clear(uint8 color);
static bitmap_t *lock_write(void);
static void free_write(int num_dirties, rect_t *dirty_rects);
static void custom_blit(bitmap_t *bmp, int num_dirties, rect_t *dirty_rects);
static char fb[1]; //dummy

QueueHandle_t vidQueue;

viddriver_t sdlDriver =
{
   "Simple DirectMedia Layer",         /* name */
   init,          /* init */
   shutdown,      /* shutdown */
   set_mode,      /* set_mode */
   set_palette,   /* set_palette */
   clear,         /* clear */
   lock_write,    /* lock_write */
   free_write,    /* free_write */
   custom_blit,   /* custom_blit */
   false          /* invalidate flag */
};


bitmap_t *myBitmap;

void osd_getvideoinfo(vidinfo_t *info)
{
   info->default_width = DEFAULT_WIDTH;
   info->default_height = DEFAULT_HEIGHT;
   info->driver = &sdlDriver;
}

/* flip between full screen and windowed */
void osd_togglefullscreen(int code)
{
}

/* initialise video */
static int init(int width, int height)
{
    return 0;
}

static void shutdown(void)
{
}

/* set a video mode */
static int set_mode(int width, int height)
{
   return 0;
}

uint16 myPalette[256];

/* copy nes palette over to hardware */
static void set_palette(rgb_t *pal)
{
    uint16 c;

   int i;

   for (i = 0; i < 256; i++)
   {
      c=(pal[i].b>>3)+((pal[i].g>>2)<<5)+((pal[i].r>>3)<<11);
      myPalette[i]=(c>>8)|((c&0xff)<<8);
      //myPalette[i]= c;
   }

}

/* clear all frames to a particular color */
static void clear(uint8 color)
{
//   SDL_FillRect(mySurface, 0, color);
}



/* acquire the directbuffer for writing */
static bitmap_t *lock_write(void)
{
//   SDL_LockSurface(mySurface);
   myBitmap = bmp_createhw((uint8*)fb, DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_WIDTH*2);
   return myBitmap;
}

/* release the resource */
static void free_write(int num_dirties, rect_t *dirty_rects)
{
   bmp_destroy(&myBitmap);
}

static uint8_t lcdfb[256 * 224];
static void custom_blit(bitmap_t *bmp, int num_dirties, rect_t *dirty_rects) {
    if (bmp->line[0] != NULL)
    {
        memcpy(lcdfb, bmp->line[0], 256 * 224);

        void* arg = (void*)lcdfb;
        xQueueSend(vidQueue, &arg, portMAX_DELAY);
    }
}


//This runs on core 1.
volatile bool exitVideoTaskFlag = false;
static void videoTask(void *arg) {
    uint8_t* bmp = NULL;

    while(1)
    {
        xQueuePeek(vidQueue, &bmp, portMAX_DELAY);

          if (bmp == 1) break;

          if (previous_scaling_enabled != scaling_enabled)
          {
              // Clear display
              ili9341_write_frame_nes(NULL, NULL, true);
              previous_scaling_enabled = scaling_enabled;
          }

          ili9341_write_frame_nes(bmp, myPalette, scaling_enabled);

          odroid_input_battery_level_read(&battery);

        xQueueReceive(vidQueue, &bmp, portMAX_DELAY);
    }

    #ifdef CONFIG_IN_GAME_MENU_NO
        odroid_display_lock_nes_display();
        odroid_display_show_hourglass();
    #endif

    odroid_display_unlock_nes_display();
    //odroid_display_drain_spi();

    exitVideoTaskFlag = true;

    vTaskDelete(NULL);

    while(1){}
}


/*
** Input
*/

static void osd_initinput()
{
}


static void SaveState()
{
    printf("Saving state.\n");

    odroid_input_battery_monitor_enabled_set(0);
    odroid_system_led_set(1);

    save_sram();

    odroid_system_led_set(0);
    odroid_input_battery_monitor_enabled_set(1);

    printf("Saving state OK.\n");
}

static void PowerDown()
{
    uint16_t* param = 1;

    // Clear audio to prevent studdering
    odroid_audio_terminate();

    // Stop tasks
    printf("PowerDown: stopping tasks.\n");

    xQueueSend(vidQueue, &param, portMAX_DELAY);
    while (!exitVideoTaskFlag) { vTaskDelay(1); }


    // state
    printf("PowerDown: Saving state.\n");
    SaveState();

    // LCD
    printf("PowerDown: Powerdown LCD panel.\n");
    ili9341_poweroff();

    printf("PowerDown: Entering deep sleep.\n");
    odroid_system_sleep();

    // Should never reach here
    abort();
}


static odroid_gamepad_state previousJoystickState;
static bool ignoreMenuButton;
static ushort powerFrameCount;

static int ConvertJoystickInput()
{
    if (ignoreMenuButton)
    {
        ignoreMenuButton = previousJoystickState.values[ODROID_INPUT_MENU];
    }


    odroid_gamepad_state state;
    odroid_input_gamepad_read(&state);

    int result = 0;


    // A
    if (!state.values[ODROID_INPUT_A])
    {
        result |= (1<<13);
    }

    // B
    if (!state.values[ODROID_INPUT_B])
    {
        result |= (1 << 14);
    }

    // select
    if (!state.values[ODROID_INPUT_SELECT])
        result |= (1 << 0);

    // start
    if (!state.values[ODROID_INPUT_START])
        result |= (1 << 3);

    // right
    if (!state.values[ODROID_INPUT_RIGHT])
            result |= (1 << 5);

    // left
    if (!state.values[ODROID_INPUT_LEFT])
            result |= (1 << 7);

    // up
    if (!state.values[ODROID_INPUT_UP])
            result |= (1 << 4);

    // down
    if (!state.values[ODROID_INPUT_DOWN])
            result |= (1 << 6);


    if (!previousJoystickState.values[ODROID_INPUT_VOLUME] && state.values[ODROID_INPUT_VOLUME])
    {
        odroid_audio_volume_change();
    }

    if (!ignoreMenuButton && previousJoystickState.values[ODROID_INPUT_MENU] && state.values[ODROID_INPUT_MENU])
    {
        ++powerFrameCount;
    }
    else
    {
        powerFrameCount = 0;
    }

    // Note: this will cause an exception on 2nd Core in Debug mode
    if (powerFrameCount > /*60*/ 30 * 2)
    {
        // Turn Blue LED on. Power state change turns it off
        gpio_set_level(GPIO_NUM_2, 1);

        PowerDown();
    }

    if (!ignoreMenuButton && previousJoystickState.values[ODROID_INPUT_MENU] && !state.values[ODROID_INPUT_MENU])
    {
        #ifdef CONFIG_IN_GAME_MENU_YES
            odroid_display_lock_nes_display();
            hud_menu();
            printf("\nACTION:%d", ACTION);
            void* arg = 1;
            switch(ACTION) {
                case 3:
                case 4:
                    hud_progress("Saving...", true);
                    hud_deinit();
                    odroid_audio_terminate();
                    xQueueSend(vidQueue, &arg, portMAX_DELAY);
                    while(exitVideoTaskFlag) {vTaskDelay(10);}
                    SaveState();
                    ili9341_clear(0);
                    esp_restart();
                break;
                case 5:
                    printf("\nDELETE ROM\n");
                break;
            }
            ili9341_clear(0);
            odroid_display_unlock_nes_display();
        #else
          odroid_audio_terminate();

          printf("Stopping video queue.\n");

          void* arg = 1;
          xQueueSend(vidQueue, &arg, portMAX_DELAY);
          while(exitVideoTaskFlag)
          {
               vTaskDelay(10);
          }

          //odroid_display_drain_spi();

          SaveState();


          // Set menu application
          odroid_system_application_set(0);


          // Reset
          esp_restart();
        #endif
    }


    // Scaling
    if (state.values[ODROID_INPUT_START] && !previousJoystickState.values[ODROID_INPUT_RIGHT] && state.values[ODROID_INPUT_RIGHT])
    {
        scaling_enabled = !scaling_enabled;
        odroid_settings_ScaleDisabled_set(ODROID_SCALE_DISABLE_NES, scaling_enabled ? 0 : 1);
    }


    previousJoystickState = state;

    return result;
}


extern nes_t* console_nes;
extern nes6502_context cpu;

void osd_getinput(void)
{
    const int ev[16]={
            event_joypad1_select,0,0,event_joypad1_start,event_joypad1_up,event_joypad1_right,event_joypad1_down,event_joypad1_left,
            0,0,0,0,event_soft_reset,event_joypad1_a,event_joypad1_b,event_hard_reset
        };
    static int oldb=0xffff;
    int b=ConvertJoystickInput();
    int chg=b^oldb;
    int x;
    oldb=b;
    event_t evh;
//  printf("Input: %x\n", b);
    for (x=0; x<16; x++) {
        if (chg&1) {
            evh=event_get(ev[x]);
            if (evh) evh((b&1)?INP_STATE_BREAK:INP_STATE_MAKE);
        }
        chg>>=1;
        b>>=1;
    }
}

static void osd_freeinput(void)
{
}

void osd_getmouse(int *x, int *y, int *button)
{
}

/*
** Shutdown
*/

/* this is at the bottom, to eliminate warnings */
void osd_shutdown()
{
    osd_stopsound();
    osd_freeinput();
}

static int logprint(const char *string)
{
   return printf("%s", string);
}

/*
** Startup
*/
// Boot state overrides
bool forceConsoleReset = false;

int osd_init()
{
    log_chain_logfunc(logprint);

    if (osd_init_sound())
    {
        abort();
    }


    Volume = odroid_settings_Volume_get();

    scaling_enabled = odroid_settings_ScaleDisabled_get(ODROID_SCALE_DISABLE_NES) ? false : true;

    previousJoystickState = odroid_input_read_raw();
    ignoreMenuButton = previousJoystickState.values[ODROID_INPUT_MENU];


    ili9341_write_frame_nes(NULL, NULL, true);


    vidQueue=xQueueCreate(1, sizeof(bitmap_t *));
    xTaskCreatePinnedToCore(&videoTask, "videoTask", 2048, NULL, 5, NULL, 1);

    osd_initinput();

    return 0;
}
