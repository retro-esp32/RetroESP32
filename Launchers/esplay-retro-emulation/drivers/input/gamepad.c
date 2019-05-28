#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include <esp_log.h>
#include "sdkconfig.h"
#include "gamepad.h"
#include <driver/adc.h>
#include "esp_adc_cal.h"
#if CONFIG_USE_LVGL
#include "lvgl/lv_hal/lv_hal_indev.h"
#include "lvgl/lv_core/lv_group.h"
#endif

static volatile bool input_task_is_running = false;
static volatile input_gamepad_state gamepad_state;
static input_gamepad_state previous_gamepad_state;
static uint8_t debounce[GAMEPAD_INPUT_MAX];
static volatile bool input_gamepad_initialized = false;
static SemaphoreHandle_t xSemaphore;

input_gamepad_state gamepad_input_read_raw()
{
    input_gamepad_state state = {0};

    int joyX = adc1_get_raw(IO_X);
    int joyY = adc1_get_raw(IO_Y);

    if (joyX > 2048 + 1024)
    {
        state.values[GAMEPAD_INPUT_LEFT] = 1;
        state.values[GAMEPAD_INPUT_RIGHT] = 0;
    }
    else if (joyX > 1024)
    {
        state.values[GAMEPAD_INPUT_LEFT] = 0;
        state.values[GAMEPAD_INPUT_RIGHT] = 1;
    }
    else
    {
        state.values[GAMEPAD_INPUT_LEFT] = 0;
        state.values[GAMEPAD_INPUT_RIGHT] = 0;
    }

    if (joyY > 2048 + 1024)
    {
        state.values[GAMEPAD_INPUT_UP] = 1;
        state.values[GAMEPAD_INPUT_DOWN] = 0;
    }
    else if (joyY > 1024)
    {
        state.values[GAMEPAD_INPUT_UP] = 0;
        state.values[GAMEPAD_INPUT_DOWN] = 1;
    }
    else
    {
        state.values[GAMEPAD_INPUT_UP] = 0;
        state.values[GAMEPAD_INPUT_DOWN] = 0;
    }

    state.values[GAMEPAD_INPUT_SELECT] = !(gpio_get_level(SELECT));
    state.values[GAMEPAD_INPUT_START] = !(gpio_get_level(START));

    state.values[GAMEPAD_INPUT_A] = !(gpio_get_level(A));
    state.values[GAMEPAD_INPUT_B] = !(gpio_get_level(B));

    state.values[GAMEPAD_INPUT_MENU] = !(gpio_get_level(MENU));

    return state;
}

static void input_task(void *arg)
{
    input_task_is_running = true;

    // Initialize state
    for (int i = 0; i < GAMEPAD_INPUT_MAX; ++i)
    {
        debounce[i] = 0xff;
    }

    while (input_task_is_running)
    {
        // Shift current values
        for (int i = 0; i < GAMEPAD_INPUT_MAX; ++i)
        {
            debounce[i] <<= 1;
        }

        // Read hardware
        input_gamepad_state state = gamepad_input_read_raw();

        // Debounce
        xSemaphoreTake(xSemaphore, portMAX_DELAY);

        for (int i = 0; i < GAMEPAD_INPUT_MAX; ++i)
        {
            debounce[i] |= state.values[i] ? 1 : 0;
            uint8_t val = debounce[i] & 0x03; //0x0f;
            switch (val)
            {
            case 0x00:
                gamepad_state.values[i] = 0;
                break;

            case 0x03: //0x0f:
                gamepad_state.values[i] = 1;
                break;

            default:
                // ignore
                break;
            }
        }

        previous_gamepad_state = gamepad_state;

        xSemaphoreGive(xSemaphore);

        // delay
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    input_gamepad_initialized = false;

    vSemaphoreDelete(xSemaphore);

    // Remove the task from scheduler
    vTaskDelete(NULL);

    // Never return
    while (1)
    {
        vTaskDelay(1);
    }
}

void gamepad_read(input_gamepad_state *out_state)
{
    if (!input_gamepad_initialized)
        abort();

    xSemaphoreTake(xSemaphore, portMAX_DELAY);

    *out_state = gamepad_state;

    xSemaphoreGive(xSemaphore);
}

void gamepad_init()
{
    xSemaphore = xSemaphoreCreateMutex();

    if (xSemaphore == NULL)
    {
        printf("xSemaphoreCreateMutex failed.\n");
        abort();
    }

    //Configure button
    gpio_config_t btn_config;
    btn_config.intr_type = GPIO_INTR_ANYEDGE; //Enable interrupt on both rising and falling edges
    btn_config.mode = GPIO_MODE_INPUT;        //Set as Input
    btn_config.pin_bit_mask = (uint64_t)      //Bitmask
                              ((uint64_t)1 << A) |
                              ((uint64_t)1 << B) | ((uint64_t)1 << SELECT);

    btn_config.pull_up_en = GPIO_PULLUP_ENABLE;      //Disable pullup
    btn_config.pull_down_en = GPIO_PULLDOWN_DISABLE; //Enable pulldown
    gpio_config(&btn_config);

    gpio_set_direction(MENU, GPIO_MODE_INPUT);
    gpio_set_pull_mode(MENU, GPIO_PULLUP_ONLY);

    gpio_set_direction(START, GPIO_MODE_INPUT);

    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(IO_X, ADC_ATTEN_11db);
    adc1_config_channel_atten(IO_Y, ADC_ATTEN_11db);

    input_gamepad_initialized = true;

    // Start background polling
    xTaskCreatePinnedToCore(&input_task, "input_task", 1024 * 2, NULL, 5, NULL, 1);

    printf("input_gamepad_init done.\n");
}

void input_gamepad_terminate()
{
    if (!input_gamepad_initialized)
        abort();

    input_task_is_running = false;
}

#if CONFIG_USE_LVGL
bool lv_keypad_read(lv_indev_data_t *data)
{
    if (!input_gamepad_initialized)
        abort();

    if (gamepad_state.values[GAMEPAD_INPUT_UP] == 1)
    {
        //printf("UP\n");
        data->state = LV_INDEV_STATE_PR;
        data->key = LV_GROUP_KEY_UP;
    }
    else if (gamepad_state.values[GAMEPAD_INPUT_DOWN] == 1)
    {
        //printf("DOWN\n");
        data->state = LV_INDEV_STATE_PR;
        data->key = LV_GROUP_KEY_DOWN;
    }
    else if (gamepad_state.values[GAMEPAD_INPUT_LEFT] == 1)
    {
        //printf("LEFT\n");
        data->state = LV_INDEV_STATE_PR;
        data->key = LV_GROUP_KEY_LEFT;
    }
    else if (gamepad_state.values[GAMEPAD_INPUT_RIGHT] == 1)
    {
        //printf("RIGHT\n");
        data->state = LV_INDEV_STATE_PR;
        data->key = LV_GROUP_KEY_RIGHT;
    }
    else if (gamepad_state.values[GAMEPAD_INPUT_B] == 1)
    {
        //printf("ESC\n");
        data->state = LV_INDEV_STATE_PR;
        data->key = LV_GROUP_KEY_ESC;
    }
    else if (gamepad_state.values[GAMEPAD_INPUT_A] == 1)
    {
        //printf("ENTER\n");
        data->state = LV_INDEV_STATE_PR;
        data->key = LV_GROUP_KEY_ENTER;
    }
    else if (gamepad_state.values[GAMEPAD_INPUT_SELECT] == 1)
    {
        //printf("SELECT\n");
        data->state = LV_INDEV_STATE_PR;
        data->key = LV_GROUP_KEY_NEXT;
    }
    else if (gamepad_state.values[GAMEPAD_INPUT_START] == 1)
    {
        //printf("START\n");
        data->state = LV_INDEV_STATE_PR;
        data->key = LV_GROUP_KEY_PREV;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }

    return false;
}
#endif