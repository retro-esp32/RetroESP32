#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/i2s.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_task_wdt.h"
#include "esp_spiffs.h"
#include "driver/rtc_io.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"

#include <loader.h>
#include <hw.h>
#include <lcd.h>
#include <fb.h>
#include <cpu.h>
#include <mem.h>
#include <sound.h>
#include <pcm.h>
#include <regs.h>
#include <rtc.h>
#include <gnuboy.h>

#include <string.h>

#include <display.h>
#include <gamepad.h>
#include <audio.h>
#include <sdcard.h>
#include <settings.h>
#include <power.h>
#include "display_gb.h"

extern int debug_trace;

struct fb fb;
struct pcm pcm;

uint16_t *displayBuffer[2]; //= { fb0, fb0 }; //[160 * 144];
uint8_t currentBuffer;

uint16_t *framebuffer;
int frame = 0;
uint elapsedTime = 0;

int32_t *audioBuffer[2];
volatile uint8_t currentAudioBuffer = 0;
volatile uint16_t currentAudioSampleCount;
volatile int16_t *currentAudioBufferPtr;

const char *StateFileName = "/gnuboy/data/gnuboy.sav";

const char *SD_BASE_PATH = "/sd";

#define GAMEBOY_WIDTH (160)
#define GAMEBOY_HEIGHT (144)

#define AUDIO_SAMPLE_RATE (32000)

// --- MAIN
QueueHandle_t vidQueue;
QueueHandle_t audioQueue;

int pcm_submit()
{
    audio_submit(currentAudioBufferPtr, currentAudioSampleCount >> 1);

    return 1;
}

void run_to_vblank()
{
    /* FRAME BEGIN */

    /* FIXME: djudging by the time specified this was intended
  to emulate through vblank phase which is handled at the
  end of the loop. */
    cpu_emulate(2280);

    /* FIXME: R_LY >= 0; comparsion to zero can also be removed
  altogether, R_LY is always 0 at this point */
    while (R_LY > 0 && R_LY < 144)
    {
        /* Step through visible line scanning phase */
        emu_step();
    }

    /* VBLANK BEGIN */

    //vid_end();
    if ((frame % 2) == 0)
    {
        xQueueSend(vidQueue, &framebuffer, portMAX_DELAY);

        // swap buffers
        currentBuffer = currentBuffer ? 0 : 1;
        framebuffer = displayBuffer[currentBuffer];

        fb.ptr = framebuffer;
    }

    rtc_tick();

    sound_mix();

    if (pcm.pos > 100)
    {
        currentAudioBufferPtr = audioBuffer[currentAudioBuffer];
        currentAudioSampleCount = pcm.pos;

        void *tempPtr = 0x1234;
        xQueueSend(audioQueue, &tempPtr, portMAX_DELAY);

        // Swap buffers
        currentAudioBuffer = currentAudioBuffer ? 0 : 1;
        pcm.buf = audioBuffer[currentAudioBuffer];
        pcm.pos = 0;
    }

    if (!(R_LCDC & 0x80))
    {
        /* LCDC operation stopped */
        /* FIXME: djudging by the time specified, this is
    intended to emulate through visible line scanning
    phase, even though we are already at vblank here */
        cpu_emulate(32832);
    }

    while (R_LY > 0)
    {
        /* Step through vblank phase */
        emu_step();
    }
}

volatile bool videoTaskIsRunning = false;
esplay_scale_option scale_opt;

void videoTask(void *arg)
{
    esp_err_t ret;
    scale_opt = get_scale_option_settings();
    videoTaskIsRunning = true;

    uint16_t *param;
    while (1)
    {
        xQueuePeek(vidQueue, &param, portMAX_DELAY);

        if (param == 1)
            break;

        write_gb_frame(param, scale_opt);

        xQueueReceive(vidQueue, &param, portMAX_DELAY);
    }

    display_show_hourglass();

    videoTaskIsRunning = false;
    vTaskDelete(NULL);

    while (1)
    {
    }
}

volatile bool AudioTaskIsRunning = false;
void audioTask(void *arg)
{
    // sound
    uint16_t *param;

    AudioTaskIsRunning = true;
    while (1)
    {
        xQueuePeek(audioQueue, &param, portMAX_DELAY);

        if (param == 0)
        {
            // TODO: determine if this is still needed
            abort();
        }
        else if (param == 1)
        {
            break;
        }
        else
        {
            pcm_submit();
        }

        xQueueReceive(audioQueue, &param, portMAX_DELAY);
    }

    printf("audioTask: exiting.\n");

    AudioTaskIsRunning = false;
    vTaskDelete(NULL);

    while (1)
    {
    }
}

static void SaveState()
{
    // Save sram
    char *romPath = get_rom_name_settings();
    if (romPath)
    {
        char *fileName = system_util_GetFileName(romPath);
        if (!fileName)
            abort();

        char *pathName = sdcard_create_savefile_path(SD_BASE_PATH, fileName);
        if (!pathName)
            abort();

        FILE *f = fopen(pathName, "w");
        if (f == NULL)
        {
            printf("%s: fopen save failed\n", __func__);
            abort();
        }

        savestate(f);
        fclose(f);

        printf("%s: savestate OK.\n", __func__);

        free(pathName);
        free(fileName);
        free(romPath);
    }
    else
    {
        FILE *f = fopen(StateFileName, "w");
        if (f == NULL)
        {
            printf("SaveState: fopen save failed\n");
        }
        else
        {
            savestate(f);
            fclose(f);

            printf("SaveState: savestate OK.\n");
        }
    }
}

static void LoadState(const char *cartName)
{
    char *romName = get_rom_name_settings();
    if (romName)
    {
        char *fileName = system_util_GetFileName(romName);
        if (!fileName)
            abort();

        char *pathName = sdcard_create_savefile_path(SD_BASE_PATH, fileName);
        if (!pathName)
            abort();

        FILE *f = fopen(pathName, "r");
        if (f == NULL)
        {
            printf("LoadState: fopen load failed\n");
        }
        else
        {
            loadstate(f);
            fclose(f);

            vram_dirty();
            pal_dirty();
            sound_dirty();
            mem_updatemap();

            printf("LoadState: loadstate OK.\n");
        }

        free(pathName);
        free(fileName);
        free(romName);
    }
    else
    {
        FILE *f = fopen(StateFileName, "r");
        if (f == NULL)
        {
            printf("LoadState: fopen load failed\n");
        }
        else
        {
            loadstate(f);
            fclose(f);

            vram_dirty();
            pal_dirty();
            sound_dirty();
            mem_updatemap();

            printf("LoadState: loadstate OK.\n");
        }
    }
}

static void PowerDown()
{
    uint16_t *param = 1;

    // Clear audio to prevent studdering
    printf("PowerDown: stopping audio.\n");

    xQueueSend(audioQueue, &param, portMAX_DELAY);
    while (AudioTaskIsRunning)
    {
    }

    // Stop tasks
    printf("PowerDown: stopping tasks.\n");

    xQueueSend(vidQueue, &param, portMAX_DELAY);
    while (videoTaskIsRunning)
    {
    }

    // state
    printf("PowerDown: Saving state.\n");
    SaveState();

    // LCD
    printf("PowerDown: Powerdown LCD panel.\n");
    display_poweroff();

    system_sleep();
    //esp_deep_sleep_start();

    // Should never reach here
    abort();
}

static void DoMenuHome()
{
    esp_err_t err;
    uint16_t *param = 1;

    // Clear audio to prevent studdering
    printf("PowerDown: stopping audio.\n");

    xQueueSend(audioQueue, &param, portMAX_DELAY);
    while (AudioTaskIsRunning)
    {
    }

    // Stop tasks
    printf("PowerDown: stopping tasks.\n");

    xQueueSend(vidQueue, &param, portMAX_DELAY);
    while (videoTaskIsRunning)
    {
    }

    // Set menu application
    system_application_set(0);

    // state
    printf("PowerDown: Saving state.\n");
    SaveState();

    // Reset
    esp_restart();
}

void app_main(void)
{
    printf("gnuboy (%s-%s).\n", COMPILEDATE, GITREV);

    nvs_flash_init();

    system_init();

    // Audio
    audio_init(AUDIO_SAMPLE_RATE);

    // Display
    display_prepare();
    display_init();

    // set brightness
    set_display_brightness(get_backlight_settings());
    audio_volume_set(get_volume_settings());

    //display_show_splash();
    //vTaskDelay(2000);

    // Gamepad
    gamepad_init();

    // ui init
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

            input_gamepad_state bootState = gamepad_input_read_raw();

            if (bootState.values[GAMEPAD_INPUT_MENU])
            {
                // Force return to factory app to recover from
                // ROM loading crashes

                // Set menu application
                system_application_set(0);

                // Reset
                esp_restart();
            }

            if (bootState.values[GAMEPAD_INPUT_START])
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

    // Clear display
    write_gb_frame(NULL, SCALE_STRETCH);

    // Load ROM
    loader_init(NULL);

    // Allocate display buffers
    displayBuffer[0] = heap_caps_malloc(160 * 144 * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    displayBuffer[1] = heap_caps_malloc(160 * 144 * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);

    if (displayBuffer[0] == 0 || displayBuffer[1] == 0)
        abort();

    framebuffer = displayBuffer[0];

    for (int i = 0; i < 2; ++i)
    {
        memset(displayBuffer[i], 0, 160 * 144 * 2);
    }

    printf("app_main: displayBuffer[0]=%p, [1]=%p\n", displayBuffer[0], displayBuffer[1]);

    // video
    vidQueue = xQueueCreate(1, sizeof(uint16_t *));
    audioQueue = xQueueCreate(1, sizeof(uint16_t *));

    xTaskCreatePinnedToCore(&videoTask, "videoTask", 2048, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&audioTask, "audioTask", 2048, NULL, 5, NULL, 1); //768

    //debug_trace = 1;
    emu_reset();

    //&rtc.carry, &rtc.stop,
    rtc.d = 1;
    rtc.h = 1;
    rtc.m = 1;
    rtc.s = 1;
    rtc.t = 1;

    // vid_begin
    memset(&fb, 0, sizeof(fb));
    fb.w = 160;
    fb.h = 144;
    fb.pelsize = 2;
    fb.pitch = fb.w * fb.pelsize;
    fb.indexed = 0;
    fb.ptr = framebuffer;
    fb.enabled = 1;
    fb.dirty = 0;

    // Note: Magic number obtained by adjusting until audio buffer overflows stop.
    const int audioBufferLength = AUDIO_SAMPLE_RATE / 10 + 1;
    //printf("CHECKPOINT AUDIO: HEAP:0x%x - allocating 0x%x\n", esp_get_free_heap_size(), audioBufferLength * sizeof(int16_t) * 2 * 2);
    const int AUDIO_BUFFER_SIZE = audioBufferLength * sizeof(int16_t) * 2;

    // pcm.len = count of 16bit samples (x2 for stereo)
    memset(&pcm, 0, sizeof(pcm));
    pcm.hz = AUDIO_SAMPLE_RATE;
    pcm.stereo = 1;
    pcm.len = /*pcm.hz / 2*/ audioBufferLength;
    pcm.buf = heap_caps_malloc(AUDIO_BUFFER_SIZE, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    pcm.pos = 0;

    audioBuffer[0] = pcm.buf;
    audioBuffer[1] = heap_caps_malloc(AUDIO_BUFFER_SIZE, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);

    if (audioBuffer[0] == 0 || audioBuffer[1] == 0)
        abort();

    sound_reset();

    lcd_begin();

    // Load state
    LoadState(rom.name);

    uint startTime;
    uint stopTime;
    uint totalElapsedTime = 0;
    uint actualFrameCount = 0;
    input_gamepad_state lastJoysticState;

    ushort menuButtonFrameCount = 0;
    bool ignoreMenuButton = lastJoysticState.values[GAMEPAD_INPUT_MENU];

    // Reset if button held at startup
    if (forceConsoleReset)
    {
        emu_reset();
    }

    gamepad_read(&lastJoysticState);
    while (true)
    {
        input_gamepad_state joystick;
        gamepad_read(&joystick);

        if (ignoreMenuButton)
        {
            ignoreMenuButton = lastJoysticState.values[GAMEPAD_INPUT_MENU];
        }

        if (!ignoreMenuButton && lastJoysticState.values[GAMEPAD_INPUT_MENU] && joystick.values[GAMEPAD_INPUT_MENU])
        {
            ++menuButtonFrameCount;
        }
        else
        {
            menuButtonFrameCount = 0;
        }

        //if (!lastJoysticState.Menu && joystick.Menu)
        if (menuButtonFrameCount > 60 * 2)
        {
            PowerDown();
            //DoMenuHome();
        }

        if (!ignoreMenuButton && lastJoysticState.values[GAMEPAD_INPUT_MENU] && !joystick.values[GAMEPAD_INPUT_MENU])
        {
            // Save State and Go To Menu
            //PowerDown();
            DoMenuHome();
        }

        pad_set(PAD_UP, joystick.values[GAMEPAD_INPUT_UP]);
        pad_set(PAD_RIGHT, joystick.values[GAMEPAD_INPUT_RIGHT]);
        pad_set(PAD_DOWN, joystick.values[GAMEPAD_INPUT_DOWN]);
        pad_set(PAD_LEFT, joystick.values[GAMEPAD_INPUT_LEFT]);

        pad_set(PAD_SELECT, joystick.values[GAMEPAD_INPUT_SELECT]);
        pad_set(PAD_START, joystick.values[GAMEPAD_INPUT_START]);

        pad_set(PAD_A, joystick.values[GAMEPAD_INPUT_A]);
        pad_set(PAD_B, joystick.values[GAMEPAD_INPUT_B]);

        startTime = xthal_get_ccount();
        run_to_vblank();
        stopTime = xthal_get_ccount();

        lastJoysticState = joystick;

        if (stopTime > startTime)
            elapsedTime = (stopTime - startTime);
        else
            elapsedTime = ((uint64_t)stopTime + (uint64_t)0xffffffff) - (startTime);

        totalElapsedTime += elapsedTime;
        ++frame;
        ++actualFrameCount;

        if (actualFrameCount == 60)
        {
            float seconds = totalElapsedTime / (CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ * 1000000.0f); // 240000000.0f; // (240Mhz)
            float fps = actualFrameCount / seconds;

            printf("HEAP:0x%x, FPS:%f\n", esp_get_free_heap_size(), fps);

            actualFrameCount = 0;
            totalElapsedTime = 0;
        }
    }
}
