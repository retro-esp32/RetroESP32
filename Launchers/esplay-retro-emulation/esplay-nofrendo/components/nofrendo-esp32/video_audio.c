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
#include "nesstate.h"
#include <osd.h>
#include <stdint.h>
#include "driver/i2s.h"
#include "sdkconfig.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "display.h"
#include "gamepad.h"
#include "audio.h"
#include "settings.h"
#include "power.h"
#include "display_nes.h"

#define DEFAULT_SAMPLERATE      32000
#define DEFAULT_FRAGSIZE        512

#define DEFAULT_FRAME_WIDTH     256
#define DEFAULT_FRAME_HEIGHT    NES_VISIBLE_HEIGHT

TimerHandle_t timer;

//Seemingly, this will be called only once. Should call func with a freq of frequency,
int osd_installtimer(int frequency, void *func, int funcsize, void *counter, int countersize)
{
    printf("Timer install, freq=%d\n", frequency);
    timer=xTimerCreate("nes",configTICK_RATE_HZ/frequency, pdTRUE, NULL, func);
    xTimerStart(timer, 0);
    return 0;
}


/*
** Audio
*/
static void (*audio_callback)(void *buffer, int length) = NULL;
QueueHandle_t queue;
static short *audio_frame;

void do_audio_frame() {
    int left=DEFAULT_SAMPLERATE/NES_REFRESH_RATE;
    while(left) {
        int n=DEFAULT_FRAGSIZE;
        if (n>left) n=left;
        audio_callback(audio_frame, n);
        //16 bit mono -> 32-bit (16 bit r+l)
        for (int i=n-1; i>=0; i--)
        {
            int sample = (int)audio_frame[i];

            audio_frame[i*2]= (short)sample;
            audio_frame[i*2+1] = (short)sample;
        }
        audio_submit(audio_frame, n);
        left-=n;
    }
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
    audio_frame=malloc(4*DEFAULT_FRAGSIZE);
    audio_init(DEFAULT_SAMPLERATE);
    audio_callback = NULL;
    audio_volume_set(get_volume_settings());

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
    info->default_width = DEFAULT_FRAME_WIDTH;
    info->default_height = DEFAULT_FRAME_HEIGHT;
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

uint16_t myPalette[256];

/* copy nes palette over to hardware */
static void set_palette(rgb_t *pal)
{
    uint16 c;

    int i;

    for (i = 0; i < 256; i++)
    {
        c=(pal[i].b>>3)+((pal[i].g>>2)<<5)+((pal[i].r>>3)<<11);
        myPalette[i]=(c>>8)|((c&0xff)<<8);
        //myPalette[i]=c;
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
    myBitmap = bmp_createhw((uint8*)fb, DEFAULT_FRAME_WIDTH, DEFAULT_FRAME_HEIGHT, DEFAULT_FRAME_WIDTH*2);
    return myBitmap;
}

/* release the resource */
static void free_write(int num_dirties, rect_t *dirty_rects)
{
    bmp_destroy(&myBitmap);
}
/*
static void custom_blit(bitmap_t *bmp, int num_dirties, rect_t *dirty_rects) {
    xQueueSend(vidQueue, &bmp, 0);
    do_audio_frame();
}
*/

static uint8_t lcdfb[256 * 240];
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
esplay_scale_option opt;
static void videoTask(void *arg) {
    bitmap_t *bmp=NULL;
    opt = get_scale_option_settings();
    while(1)
    {
        xQueuePeek(vidQueue, &bmp, portMAX_DELAY);

        if (bmp == 1) break;

        write_nes_frame(bmp, opt);

        xQueueReceive(vidQueue, &bmp, portMAX_DELAY);
    }

    display_show_hourglass();

    exitVideoTaskFlag = true;

    vTaskDelete(NULL);

    while(1){}
}

static void SaveState()
{
    printf("Saving state.\n");

    save_sram();

    printf("Saving state OK.\n");
}

static void PowerDown()
{
    uint16_t* param = 1;

    // Stop tasks
    printf("PowerDown: stopping tasks.\n");

    xQueueSend(vidQueue, &param, portMAX_DELAY);
    while (!exitVideoTaskFlag) { vTaskDelay(1); }


    // state
    printf("PowerDown: Saving state.\n");
    SaveState();

    // LCD
    printf("PowerDown: Powerdown LCD panel.\n");
    display_poweroff();

    printf("PowerDown: Entering deep sleep.\n");
    system_sleep();

    // Should never reach here
    abort();
}


/*
** Input
*/

static void osd_initinput()
{
    
}


input_gamepad_state previous_state;
static bool ignoreMenuButton;
static ushort powerFrameCount;

static int ConvertGamepadInput()
{
    if (ignoreMenuButton)
    {
        ignoreMenuButton = previous_state.values[GAMEPAD_INPUT_MENU];
    }

    input_gamepad_state state;
    gamepad_read(&state);

    int result = 0;

    // A
    if (!state.values[GAMEPAD_INPUT_A])
    {
        result |= (1 << 13);
    }

    // B
    if (!state.values[GAMEPAD_INPUT_B])
    {
        result |= (1 << 14);
    }

    // select
    if (!state.values[GAMEPAD_INPUT_SELECT])
        result |= (1 << 0);

    // start
    if (!state.values[GAMEPAD_INPUT_START])
        result |= (1 << 3);

    // right
    if (!state.values[GAMEPAD_INPUT_RIGHT])
        result |= (1 << 5);

    // left
    if (!state.values[GAMEPAD_INPUT_LEFT])
        result |= (1 << 7);

    // up
    if (!state.values[GAMEPAD_INPUT_UP])
        result |= (1 << 4);

    // down
    if (!state.values[GAMEPAD_INPUT_DOWN])
        result |= (1 << 6);

    if (!ignoreMenuButton && previous_state.values[GAMEPAD_INPUT_MENU] && state.values[GAMEPAD_INPUT_MENU])
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
        PowerDown();
    }

    if (!ignoreMenuButton && previous_state.values[GAMEPAD_INPUT_MENU] && !state.values[GAMEPAD_INPUT_MENU])
    {
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
        system_application_set(0);


        // Reset
        esp_restart();
    }

    previous_state = state;

    return result;
}

void osd_getinput(void)
{
    const int ev[16]={
        event_joypad1_select,
        0,
        0,
        event_joypad1_start,
        event_joypad1_up,
        event_joypad1_right,
        event_joypad1_down,
        event_joypad1_left,
        0,
        0,
        0,
        0,
        event_soft_reset,
        event_joypad1_a,
        event_joypad1_b,
        event_hard_reset
    };
    static int oldb=0xffff;
    int b=ConvertGamepadInput();
    int chg=b^oldb;
    int x;
    oldb=b;
    event_t evh;
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
        return -1;

    write_nes_frame(NULL, SCALE_STRETCH);
    vidQueue=xQueueCreate(1, sizeof(bitmap_t *));
    xTaskCreatePinnedToCore(&videoTask, "videoTask", 2048, NULL, 5, NULL, 1);
    osd_initinput();
    return 0;
}
