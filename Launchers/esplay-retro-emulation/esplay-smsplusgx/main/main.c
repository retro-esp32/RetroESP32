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

#include "../components/smsplus/shared.h"

#include "settings.h"
#include "audio.h"
#include "gamepad.h"
#include "system.h"
#include "display.h"
#include "sdcard.h"
#include "power.h"
#include "display_sms.h"

#include <dirent.h>

const char *SD_BASE_PATH = "/sd";

#define AUDIO_SAMPLE_RATE (32000)

uint16 palette[PALETTE_SIZE];
uint8_t *framebuffer[2];
int currentFramebuffer = 0;

uint32_t *audioBuffer = NULL;
int audioBufferCount = 0;

spi_flash_mmap_handle_t hrom;

QueueHandle_t vidQueue;
TaskHandle_t videoTaskHandle;

esplay_volume_level Volume;
battery_state battery;

volatile bool videoTaskIsRunning = false;
esplay_scale_option opt;
void videoTask(void *arg)
{
    uint8_t *param;
    opt = get_scale_option_settings();
    videoTaskIsRunning = true;

    const bool isGameGear = (sms.console == CONSOLE_GG) | (sms.console == CONSOLE_GGMS);

    while (1)
    {
        xQueuePeek(vidQueue, &param, portMAX_DELAY);

        if (param == 1)
            break;

        render_copy_palette(palette);
        write_sms_frame(param, palette, isGameGear, opt);

        battery_level_read(&battery);

        xQueueReceive(vidQueue, &param, portMAX_DELAY);
    }

    // Draw hourglass
    display_show_hourglass();

    videoTaskIsRunning = false;
    vTaskDelete(NULL);

    while (1)
    {
    }
}

//Read an unaligned byte.
char unalChar(const unsigned char *adr)
{
    //See if the byte is in memory that can be read unaligned anyway.
    if (!(((int)adr) & 0x40000000))
        return *adr;
    //Nope: grab a word and distill the byte.
    int *p = (int *)((int)adr & 0xfffffffc);
    int v = *p;
    int w = ((int)adr & 3);
    if (w == 0)
        return ((v >> 0) & 0xff);
    if (w == 1)
        return ((v >> 8) & 0xff);
    if (w == 2)
        return ((v >> 16) & 0xff);
    if (w == 3)
        return ((v >> 24) & 0xff);

    abort();
    return 0;
}

const char *StateFileName = "/storage/smsplus.sav";
const char *StoragePath = "/storage";

static void SaveState()
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

        FILE *f = fopen(pathName, "w");

        if (f == NULL)
        {
            printf("SaveState: fopen save failed\n");
        }
        else
        {
            system_save_state(f);
            fclose(f);

            printf("SaveState: system_save_state OK.\n");
        }

        free(pathName);
        free(fileName);
        free(romName);
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
            system_save_state(f);
            fclose(f);

            printf("SaveState: system_save_state OK.\n");
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
            system_load_state(f);
            fclose(f);

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
            system_load_state(f);
            fclose(f);

            printf("LoadState: system_load_state OK.\n");
        }
    }

    Volume = get_volume_settings();
    audio_volume_set(Volume);
}

static void PowerDown()
{
    uint16_t *param = 1;

    // Clear audio to prevent studdering
    printf("PowerDown: stopping audio.\n");
    audio_terminate();

    // Stop tasks
    printf("PowerDown: stopping tasks.\n");

    xQueueSend(vidQueue, &param, portMAX_DELAY);
    while (videoTaskIsRunning)
    {
        vTaskDelay(1);
    }

    // state
    printf("PowerDown: Saving state.\n");
    SaveState();

    // LCD
    printf("PowerDown: Powerdown LCD panel.\n");
    display_poweroff();

    system_sleep();

    // Should never reach here
    abort();
}

static void DoHome()
{
    esp_err_t err;
    uint16_t *param = 1;

    // Clear audio to prevent studdering
    printf("PowerDown: stopping audio.\n");
    audio_terminate();

    // Stop tasks
    printf("PowerDown: stopping tasks.\n");

    xQueueSend(vidQueue, &param, portMAX_DELAY);
    while (videoTaskIsRunning)
    {
        vTaskDelay(1);
    }

    // state
    printf("PowerDown: Saving state.\n");
    SaveState();

    // Set menu application
    system_application_set(0);

    // Reset
    esp_restart();
}

void system_manage_sram(uint8 *sram, int slot, int mode)
{
    printf("system_manage_sram\n");
    //sram_load();
}

//char cartName[1024];
void app_main(void)
{
    printf("smsplusgx (%s-%s).\n", COMPILEDATE, GITREV);

    framebuffer[0] = heap_caps_malloc(256 * 192, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if (!framebuffer[0])
        abort();
    printf("app_main: framebuffer[0]=%p\n", framebuffer[0]);

    framebuffer[1] = heap_caps_malloc(256 * 192, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if (!framebuffer[1])
        abort();
    printf("app_main: framebuffer[1]=%p\n", framebuffer[1]);

    nvs_flash_init();

    system_init();

    // Joystick.
    gamepad_init();

    // audio
    audio_init(AUDIO_SAMPLE_RATE);

    // battery
    battery_level_init();

    // Boot state overrides
    bool forceConsoleReset = false;

    display_prepare();

    switch (esp_sleep_get_wakeup_cause())
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
        printf("app_main: Unexpected deep sleep reset\n");

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
            forceConsoleReset = true; //emu_reset();
        }
    }
    break;

    default:
        printf("app_main: Not a deep sleep reset\n");
        break;
    }

    display_init();
    set_display_brightness((int) get_backlight_settings());

    const char *FILENAME = NULL;

    char *cartName = get_rom_name_settings();
    if (!cartName)
    {
        // Load fixed file name
        FILENAME = "/sd/default.sms";
    }
    else
    {
        FILENAME = cartName;
    }

    // Open SD card
    esp_err_t r = sdcard_open(SD_BASE_PATH);
    if (r != ESP_OK)
    {
        abort();
    }

    display_show_hourglass();
    // Load the ROM
    load_rom(FILENAME);

    write_sms_frame(NULL, NULL, false, SCALE_STRETCH);

    vidQueue = xQueueCreate(1, sizeof(uint16_t *));
    xTaskCreatePinnedToCore(&videoTask, "videoTask", 1024 * 4, NULL, 5, &videoTaskHandle, 1);

    sms.use_fm = 0;

    bitmap.width = 256;
    bitmap.height = 192;
    bitmap.pitch = bitmap.width;
    //bitmap.depth = 8;
    bitmap.data = framebuffer[0];

    //system_init2(AUDIO_SAMPLE_RATE);
    set_option_defaults();

    option.sndrate = AUDIO_SAMPLE_RATE;
    option.overscan = 0;
    option.extra_gg = 0;

    system_init2();
    system_reset();

    // Restore state
    LoadState(cartName);

    if (forceConsoleReset)
    {
        // Reset emulator if button held at startup to
        // override save state
        printf("%s: forceConsoleReset=true\n", __func__);
        system_reset();
    }

    input_gamepad_state previousState;
    gamepad_read(&previousState);

    uint startTime;
    uint stopTime;
    uint totalElapsedTime = 0;
    int frame = 0;
    uint16_t muteFrameCount = 0;
    uint16_t powerFrameCount = 0;

    bool ignoreMenuButton = previousState.values[GAMEPAD_INPUT_MENU];

    while (true)
    {
        input_gamepad_state joystick;
        gamepad_read(&joystick);

        if (ignoreMenuButton)
        {
            ignoreMenuButton = previousState.values[GAMEPAD_INPUT_MENU];
        }

        if (!ignoreMenuButton && previousState.values[GAMEPAD_INPUT_MENU] && joystick.values[GAMEPAD_INPUT_MENU])
        {
            ++powerFrameCount;
        }
        else
        {
            powerFrameCount = 0;
        }

        // Note: this will cause an exception on 2nd Core in Debug mode
        if (powerFrameCount > 60 * 2)
        {
            PowerDown();
        }

        if (!ignoreMenuButton && previousState.values[GAMEPAD_INPUT_MENU] && !joystick.values[GAMEPAD_INPUT_MENU])
        {
            DoHome();
        }

        startTime = xthal_get_ccount();

        int smsButtons = 0;
        if (joystick.values[GAMEPAD_INPUT_UP])
            smsButtons |= INPUT_UP;
        if (joystick.values[GAMEPAD_INPUT_DOWN])
            smsButtons |= INPUT_DOWN;
        if (joystick.values[GAMEPAD_INPUT_LEFT])
            smsButtons |= INPUT_LEFT;
        if (joystick.values[GAMEPAD_INPUT_RIGHT])
            smsButtons |= INPUT_RIGHT;
        if (joystick.values[GAMEPAD_INPUT_A])
            smsButtons |= INPUT_BUTTON2;
        if (joystick.values[GAMEPAD_INPUT_B])
            smsButtons |= INPUT_BUTTON1;

        int smsSystem = 0;
        if (joystick.values[GAMEPAD_INPUT_START])
            smsSystem |= INPUT_START;
        if (joystick.values[GAMEPAD_INPUT_SELECT])
            smsSystem |= INPUT_PAUSE;

        input.pad[0] = smsButtons;
        input.system = smsSystem;

        if (sms.console == CONSOLE_COLECO)
        {
            input.system = 0;
            coleco.keypad[0] = 0xff;
            coleco.keypad[1] = 0xff;

            // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, *, #
            switch (cart.crc)
            {
            case 0x798002a2: // Frogger
            case 0x32b95be0: // Frogger
            case 0x9cc3fabc: // Alcazar
            case 0x964db3bc: // Fraction Fever
                if (joystick.values[GAMEPAD_INPUT_START])
                {
                    coleco.keypad[0] = 10; // *
                }

                if (previousState.values[GAMEPAD_INPUT_SELECT] &&
                    !joystick.values[GAMEPAD_INPUT_SELECT])
                {
                    system_reset();
                }
                break;

            case 0x1796de5e: // Boulder Dash
            case 0x5933ac18: // Boulder Dash
            case 0x6e5c4b11: // Boulder Dash
                if (joystick.values[GAMEPAD_INPUT_START])
                {
                    coleco.keypad[0] = 11; // #
                }

                if (joystick.values[GAMEPAD_INPUT_START] &&
                    joystick.values[GAMEPAD_INPUT_LEFT])
                {
                    coleco.keypad[0] = 1;
                }

                if (previousState.values[GAMEPAD_INPUT_SELECT] &&
                    !joystick.values[GAMEPAD_INPUT_SELECT])
                {
                    system_reset();
                }
                break;
            case 0x109699e2: // Dr. Seuss's Fix-Up The Mix-Up Puzzler
            case 0x614bb621: // Decathlon
                if (joystick.values[GAMEPAD_INPUT_START])
                {
                    coleco.keypad[0] = 1;
                }
                if (joystick.values[GAMEPAD_INPUT_START] &&
                    joystick.values[GAMEPAD_INPUT_LEFT])
                {
                    coleco.keypad[0] = 10; // *
                }
                break;

            default:
                if (joystick.values[GAMEPAD_INPUT_START])
                {
                    coleco.keypad[0] = 1;
                }

                if (previousState.values[GAMEPAD_INPUT_SELECT] &&
                    !joystick.values[GAMEPAD_INPUT_SELECT])
                {
                    system_reset();
                }
                break;
            }
        }

        if (0 || (frame % 2) == 0)
        {
            system_frame(0);

            xQueueSend(vidQueue, &bitmap.data, portMAX_DELAY);

            currentFramebuffer = currentFramebuffer ? 0 : 1;
            bitmap.data = framebuffer[currentFramebuffer];
        }
        else
        {
            system_frame(1);
        }

        // Create a buffer for audio if needed
        if (!audioBuffer || audioBufferCount < snd.sample_count)
        {
            if (audioBuffer)
                free(audioBuffer);

            size_t bufferSize = snd.sample_count * 2 * sizeof(int16_t);
            //audioBuffer = malloc(bufferSize);
            audioBuffer = heap_caps_malloc(bufferSize, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
            if (!audioBuffer)
                abort();

            audioBufferCount = snd.sample_count;

            printf("app_main: Created audio buffer (%d bytes).\n", bufferSize);
        }

        // Process audio
        for (int x = 0; x < snd.sample_count; x++)
        {
            uint32_t sample;

            if (muteFrameCount < 60 * 2)
            {
                // When the emulator starts, audible poping is generated.
                // Audio should be disabled during this startup period.
                sample = 0;
                ++muteFrameCount;
            }
            else
            {
                sample = (snd.output[0][x] << 16) + snd.output[1][x];
            }

            audioBuffer[x] = sample;
        }

        // send audio

        audio_submit((short *)audioBuffer, snd.sample_count - 1);

        stopTime = xthal_get_ccount();

        previousState = joystick;

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

            printf("HEAP:0x%x, FPS:%f, BATTERY:%d [%d]\n", esp_get_free_heap_size(), fps, battery.millivolts, battery.percentage);

            frame = 0;
            totalElapsedTime = 0;
        }
    }
}
