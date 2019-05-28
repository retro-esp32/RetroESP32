#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <stdint.h>
#include <stdbool.h>
#if CONFIG_USE_LVGL_LIBRARY
#include "lvgl/lv_hal/lv_hal_indev.h"
#endif

#define A 32
#define B 33
#define START 36
#define SELECT 0
#define IO_Y ADC1_CHANNEL_7
#define IO_X ADC1_CHANNEL_6
#define MENU 13

enum
{
    GAMEPAD_INPUT_UP = 0,
    GAMEPAD_INPUT_RIGHT,
    GAMEPAD_INPUT_DOWN,
    GAMEPAD_INPUT_LEFT,
    GAMEPAD_INPUT_SELECT,
    GAMEPAD_INPUT_START,
    GAMEPAD_INPUT_A,
    GAMEPAD_INPUT_B,
    GAMEPAD_INPUT_MENU,

    GAMEPAD_INPUT_MAX
};

typedef struct
{
    uint8_t values[GAMEPAD_INPUT_MAX];
} input_gamepad_state;

void gamepad_init();
void input_gamepad_terminate();
void gamepad_read(input_gamepad_state *out_state);
input_gamepad_state gamepad_input_read_raw();

#if CONFIG_USE_LVGL_LIBRARY
bool lv_keypad_read(lv_indev_data_t *data);
#endif

#endif
