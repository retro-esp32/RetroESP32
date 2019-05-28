// Esplay Launcher - launcher for ESPLAY based on Gogo Launcher for Odroid Go.

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

#include "settings.h"
#include "gamepad.h"
#include "display.h"
#include "audio.h"
#include "power.h"
#include "sdcard.h"

#include "font.h"
#include "volume.h"
#include "brightness.h"
#include "gameboy.h"
#include "nintendo.h"
#include "gameboy_color.h"
#include "gg.h"
#include "colecovision.h"
#include "main_title.h"
#include "sms.h"
#include "scale_option.h"

#include <string.h>
#include <dirent.h>

input_gamepad_state joystick;

battery_state bat_state;

int BatteryPercent = 100;

unsigned short buffer[40000];
int colour = 65535; // white text mostly.

int num_emulators = 9;
char emulator[10][32] = {"Nintendo", "GAMEBOY", "GAMEBOY COLOR", "SEGA MASTER SYSTEM", "GAME GEAR", "COLECO VISION", "Sound Volume", "Display Brightness", "Display Scale Options"};
char emu_dir[10][20] = {"nes", "gb", "gbc", "sms", "gg", "col"};
int emu_slot[10] = {1, 2, 2, 3, 3, 3};
char brightness[10][5] = {"10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%"};
char volume[10][10] = {"Mute", "25%", "50%", "75%", "100%"};
char scale_options[10][20] = {"Disable", "Screen Fit", "Stretch"};

char target[256] = "";
int e = 0, last_e = 100, x, y = 0, count = 0;

//----------------------------------------------------------------
int read_config()
{
    FILE *fp;
    int v;

    if ((fp = fopen("/sd/esplay/data/gogo_conf.txt", "r")))
    {
        while (!feof(fp))
        {
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
        gamepad_read(&joystick);
}

//----------------------------------------------------------------
// print a character string to screen...
void print(short x, short y, char *s)
{
    int i, n, k, a, len, idx;
    unsigned char c;
    //extern unsigned short  *buffer; // screen buffer, 16 bit pixels

    len = strlen(s);
    for (k = 0; k < len; k++)
    {
        c = s[k];
        for (i = 0; i < 8; i++)
        {
            a = font_8x8[c][i];
            for (n = 7, idx = i * len * 8 + k * 8; n >= 0; n--, idx++)
            {
                if (a % 2 == 1)
                    buffer[idx] = colour;
                else
                    buffer[idx] = 0;
                a = a >> 1;
            }
        }
    }
    write_frame_rectangleLE(x * 8, y * 8, len * 8, 8, buffer);
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

    if (e != last_e)
    {
        for (i = 0; i < 320 * 56; i++)
            buffer[i] = 65535;
        write_frame_rectangleLE(0, 50, 320, 56, buffer);

        switch (e)
        {
        case 0:
            write_frame_rectangleLE(85, 50, 150, 56, nintendo_icon.pixel_data);
            break;
        case 1:
            write_frame_rectangleLE(85, 50, 150, 56, gameboy_icon.pixel_data);
            break;
        case 2:
            write_frame_rectangleLE(85, 50, 150, 56, gameboy_color_icon.pixel_data);
            break;
        case 3:
            write_frame_rectangleLE(85, 50, 150, 56, sms_icon.pixel_data);
            break;
        case 4:
            write_frame_rectangleLE(85, 50, 150, 56, gg_icon.pixel_data);
            break;
        case 5:
            write_frame_rectangleLE(85, 50, 150, 56, colecovision_icon.pixel_data);
            break;
        case 6:
            write_frame_rectangleLE(85, 50, 150, 56, volume_icon.pixel_data);
            break;
        case 7:
            write_frame_rectangleLE(85, 50, 150, 56, brightness_icon.pixel_data);
            break;        
        case 8:
            write_frame_rectangleLE(85, 50, 150, 56, scale_option_icon.pixel_data);
            break;
        }

        last_e = e;
    }

    if (e < 6)
    {
        for (i = 0; i < 40; i++)
            s[i] = ' ';
        s[i] = 0;
        len = strlen(emulator[e]);
        for (i = 0; i < len; i++)
            s[i + 19 - len / 2] = emulator[e][i];
        print(0, 18, s);

        strcat(&path[strlen(path) - 1], emu_dir[e]);
        count = 0;
        for (i = 0; i < 40; i++)
            s[i] = ' ';
        s[i] = 0;
        if (!(dr = opendir(path)))
        {
            for (i = 20; i < 30; i++)
                print(0, i, s);
            return (0);
        }

        while ((de = readdir(dr)) != NULL)
        {

            len = strlen(de->d_name);
            dotlen = strlen(emu_dir[e]);
            // only show files that have matching extension...
            if (strcmp(&de->d_name[len - dotlen], emu_dir[e]) == 0 && de->d_name[0] != '.')
            {
                printf("file: %s\n", de->d_name);
                if (count == y)
                {
                    strcpy(target, path);
                    i = strlen(target);
                    target[i] = '/';
                    target[i + 1] = 0;
                    strcat(target, de->d_name);
                    printf("target=%s\n", target);
                }
                // strip extension from file...
                de->d_name[len - dotlen - 1] = 0;
                if (strlen(de->d_name) > 39)
                    de->d_name[39] = 0;
                if (y / 10 == count / 10)
                { // right page?
                    for (i = 0; i < 40; i++)
                        s[i] = ' ';
                    for (i = 0; i < strlen(de->d_name); i++)
                        s[i] = de->d_name[i];
                    if (count == y)
                        print_y(0, (count % 10) + 20, s); // highlight
                    else
                        print(0, (count % 10) + 20, s);
                }
                count++;
            }
        }
        if (y / 10 == count / 10)
            for (i = count % 10; i < 10; i++)
                print(0, i + 20, "                                        ");
        closedir(dr);
        printf("total=%i\n", count);
    }
    else if (e == 6)
    {
        for (i = 0; i < 40; i++)
            s[i] = ' ';
        s[i] = 0;
        len = strlen(emulator[e]);
        for (i = 0; i < len; i++)
            s[i + 19 - len / 2] = emulator[e][i];
        print(0, 18, s);

        count = 0;
        while (count < 5)
        {
            for (i = 0; i < 40; i++)
                s[i] = ' ';
            for (i = 0; i < strlen(volume[count]); i++)
                s[i] = volume[count][i];
            if (count == y)
                print_y(0, (count % 10) + 20, s); // highlight
            else
                print(0, (count % 10) + 20, s);
            count++;
        }
        if (y / 10 == count / 10)
            for (i = count % 10; i < 10; i++)
                print(0, i + 20, "                                        ");
    }
    else if (e == 7)
    {
        for (i = 0; i < 40; i++)
            s[i] = ' ';
        s[i] = 0;
        len = strlen(emulator[e]);
        for (i = 0; i < len; i++)
            s[i + 19 - len / 2] = emulator[e][i];
        print(0, 18, s);

        count = 0;
        while (count < 10)
        {
            for (i = 0; i < 40; i++)
                s[i] = ' ';
            for (i = 0; i < strlen(brightness[count]); i++)
                s[i] = brightness[count][i];
            if (count == y)
                print_y(0, (count % 10) + 20, s); // highlight
            else
                print(0, (count % 10) + 20, s);
            count++;
        }
        if (y / 10 == count / 10)
            for (i = count % 10; i < 10; i++)
                print(0, i + 20, "                                        ");
    }

    else if (e == 8)
    {
        for (i = 0; i < 40; i++)
            s[i] = ' ';
        s[i] = 0;
        len = strlen(emulator[e]);
        for (i = 0; i < len; i++)
            s[i + 19 - len / 2] = emulator[e][i];
        print(0, 18, s);

        count = 0;
        while (count < 10)
        {
            for (i = 0; i < 40; i++)
                s[i] = ' ';
            for (i = 0; i < strlen(scale_options[count]); i++)
                s[i] = scale_options[count][i];
            if (count == y)
                print_y(0, (count % 10) + 20, s); // highlight
            else
                print(0, (count % 10) + 20, s);
            count++;
        }
        if (y / 10 == count / 10)
            for (i = count % 10; i < 10; i++)
                print(0, i + 20, "                                        ");
    }

    return (0);
}

//----------------------------------------------------------------
// Return to last emulator if 'B' pressed....
int resume(void)
{
    int i;
    char *extension;
    char *romPath;

    printf("trying to resume...\n");
    romPath = get_rom_name_settings();
    if (romPath)
    {
        extension = system_util_GetFileExtenstion(romPath);
        for (i = 0; i < strlen(extension); i++)
            extension[i] = extension[i + 1];
        printf("extension=%s\n", extension);
    }
    else
    {
        printf("can't resume!\n");
        return (0);
    }
    for (i = 0; i < num_emulators; i++)
    {
        if (strcmp(extension, &emu_dir[i][0]) == 0)
        {
            printf("resume - extension=%s, slot=%i\n", extension, i);
            system_application_set(emu_slot[i]); // set emulator slot
            print(14, 15, "RESUMING....");
            usleep(500000);
            esp_restart(); // reboot!
        }
    }
    free(romPath);
    free(extension);

    return (0); // nope!
}

//----------------------------------------------------------------
void app_main(void)
{
    FILE *fp;
    char s[256];

    printf("esplay start.\n");

    nvs_flash_init();
    system_init();
    gamepad_init();
    audio_init(16000);

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
    default:
        printf("app_main: Not a deep sleep reset\n");
        break;
    }

    // Display
    display_prepare();
    display_init();

    set_display_brightness((int) get_backlight_settings());

    battery_level_init();

    display_show_splash();
    vTaskDelay(200);

    sdcard_open("/sd"); // map SD card.

    display_clear(0); // clear screen
    sprintf(s, "Volume: %i%%   ", get_volume_settings() * 25);
    print(0, 0, s);
    print(16, 0, "ESPLAY");
    battery_level_read(&bat_state);
    sprintf(s, "Battery: %i%%", bat_state.percentage);
    print(26, 0, s);

    write_frame_rectangleLE((320 - main_title.width) / 2, 12, main_title.width, main_title.height, main_title.pixel_data);

    read_config();

    if (bat_state.percentage < 1)
    {
        display_show_empty_battery();
        vTaskDelay(500);

        printf("PowerDown: Powerdown LCD panel.\n");
        display_poweroff();

        printf("PowerDown: Entering deep sleep.\n");
        system_sleep();

        // Should never reach here
        abort();
    }

    print_emulator(e, y);
    while (1)
    {
        gamepad_read(&joystick);
        if (joystick.values[GAMEPAD_INPUT_LEFT])
        {
            y = 0;
            e--;
            if (e < 0)
                e = (num_emulators - 1) - 1;
            print_emulator(e, y);
            debounce(GAMEPAD_INPUT_LEFT);
        }
        if (joystick.values[GAMEPAD_INPUT_RIGHT])
        {
            y = 0;
            e++;
            if (e == num_emulators - 1)
                e = 0;
            print_emulator(e, y);
            debounce(GAMEPAD_INPUT_RIGHT);
        }
        if (joystick.values[GAMEPAD_INPUT_UP])
        {
            y--;
            if (y < 0)
                y = count - 1;
            print_emulator(e, y);
            debounce(GAMEPAD_INPUT_UP);
        }
        if (joystick.values[GAMEPAD_INPUT_DOWN])
        {
            y++;
            if (y >= count)
                y = 0;
            print_emulator(e, y);
            debounce(GAMEPAD_INPUT_DOWN);
        }
        if (joystick.values[GAMEPAD_INPUT_SELECT] && y > 9)
        {
            y -= 10;
            print_emulator(e, y);
            debounce(GAMEPAD_INPUT_SELECT);
        }
        if (joystick.values[GAMEPAD_INPUT_START] && y < count - 10)
        {
            y += 10;
            print_emulator(e, y);
            debounce(GAMEPAD_INPUT_START);
        }
        if (joystick.values[GAMEPAD_INPUT_A])
        {
            if (count != 0)
            {
                if (e == 6)
                {
                    set_volume_settings(y);
                    sprintf(s, "Volume: %i%%   ", y * 25);
                    print(0, 0, s);
                    print(14, 15, "Saved....");
                    vTaskDelay(20);
                    print(14, 15, "         ");
                }
                else if (e == 7)
                {
                    set_backlight_settings((y + 1) * 10);
                    set_display_brightness((y + 1) * 10);
                    print(14, 15, "Saved....");
                    vTaskDelay(20);
                    print(14, 15, "         ");
                }
                else if (e == 8)
                {
                    set_scale_option_settings(y);
                    print(14, 15, "Saved....");
                    vTaskDelay(20);
                    print(14, 15, "         ");
                }
                else
                {
                    // not in an empty directory...
                    set_rom_name_settings(target);
                    print(14, 15, "Loading....");
                    system_application_set(emu_slot[e]); // set emulator slot
                    esp_restart();                       // reboot!
                }
            }
            debounce(GAMEPAD_INPUT_A);
        }
        if (joystick.values[GAMEPAD_INPUT_B])
            resume();
    }
}
