/**
 * @file ili9341.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ili9341.h"
#include "disp_spi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "lcd_struct.h"
#include "driver/ledc.h"
#include "driver/rtc_io.h"

#if CONFIG_USE_LVGL
#include "../lvgl/lvgl.h"
#endif

/*********************
 *      DEFINES
 *********************/
#define MADCTL_MY 0x80
#define MADCTL_MX 0x40
#define MADCTL_MV 0x20
#define MADCTL_ML 0x10
#define MADCTL_MH 0x04
#define TFT_RGB_BGR 0x08

#define TFT_CMD_SWRESET 0x01
#define TFT_CMD_SLEEP 0x10
#define TFT_CMD_DISPLAY_OFF 0x28
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void ili9341_send_cmd(uint8_t cmd);
static void ili9341_send_data(void *data, uint16_t length);
static void backlight_init();

/**********************
 *  STATIC VARIABLES
 **********************/
static const int DUTY_MAX = 0x1fff;
static const int LCD_BACKLIGHT_ON_VALUE = 1;
static bool isBackLightIntialized = false;

DRAM_ATTR static const lcd_init_cmd_t ili_sleep_cmds[] = {
	{TFT_CMD_SWRESET, {0}, 0x80},
	{TFT_CMD_DISPLAY_OFF, {0}, 0x80},
	{TFT_CMD_SLEEP, {0}, 0x80},
	{0, {0}, 0xff}};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void ili9341_init(void)
{
	lcd_init_cmd_t ili_init_cmds[] = {
		{TFT_CMD_SWRESET, {0}, 0x80},
		{0xCF, {0x00, 0x83, 0X30}, 3},
		{0xED, {0x64, 0x03, 0X12, 0X81}, 4},
		{0xE8, {0x85, 0x01, 0x79}, 3},
		{0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
		{0xF7, {0x20}, 1},
		{0xEA, {0x00, 0x00}, 2},
		{0xC0, {0x26}, 1},											  /*Power control*/
		{0xC1, {0x11}, 1},											  /*Power control */
		{0xC5, {0x35, 0x3E}, 2},									  /*VCOM control*/
		{0xC7, {0xBE}, 1},											  /*VCOM control*/
		{0x36, {MADCTL_MY | MADCTL_MX | MADCTL_MV | TFT_RGB_BGR}, 1}, /*Memory Access Control*/
		{0x3A, {0x55}, 1},											  /*Pixel Format Set*/
		{0xB1, {0x00, 0x1B}, 2},
		{0xF2, {0x08}, 1},
		{0x26, {0x01}, 1},
		{0xE0, {0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0X87, 0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00}, 15},
		{0XE1, {0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F}, 15},
		{0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},
		{0x2B, {0x00, 0x00, 0x01, 0x3f}, 4},
		{0x2C, {0}, 0},
		{0xB7, {0x07}, 1},
		{0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},
		{0x11, {0}, 0x80},
		{0x29, {0}, 0x80},
		{0, {0}, 0xff},
	};

	//Initialize non-SPI GPIOs
	gpio_set_direction(ILI9341_BCKL, GPIO_MODE_OUTPUT);

	//Reset the display
	ili9341_send_cmd(TFT_CMD_SWRESET);
	vTaskDelay(100 / portTICK_RATE_MS);

	printf("ILI9341 initialization.\n");

	//Send all the commands
	uint16_t cmd = 0;
	while (ili_init_cmds[cmd].databytes != 0xff)
	{
		ili9341_send_cmd(ili_init_cmds[cmd].cmd);
		ili9341_send_data(ili_init_cmds[cmd].data, ili_init_cmds[cmd].databytes & 0x1F);
		if (ili_init_cmds[cmd].databytes & 0x80)
		{
			vTaskDelay(100 / portTICK_RATE_MS);
		}
		cmd++;
	}

	///Enable backlight
	printf("Enable backlight.\n");
	backlight_init();
}

int ili9341_is_backlight_initialized()
{
	return isBackLightIntialized;
}

void ili9341_backlight_percentage_set(int value)
{
	int duty = DUTY_MAX * (value * 0.01f);

	ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty, 500);
	ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
}

#if CONFIG_USE_LVGL
void ili9431_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t color)
{
	uint8_t data[4];

	//Column addresses
	ili9341_send_cmd(0x2A);
	data[0] = (x1 >> 8) & 0xFF;
	data[1] = x1 & 0xFF;
	data[2] = (x2 >> 8) & 0xFF;
	data[3] = x2 & 0xFF;
	ili9341_send_data(data, 4);

	//Page addresses
	ili9341_send_cmd(0x2B);
	data[0] = (y1 >> 8) & 0xFF;
	data[1] = y1 & 0xFF;
	data[2] = (y2 >> 8) & 0xFF;
	data[3] = y2 & 0xFF;
	ili9341_send_data(data, 4);

	//Memory write
	ili9341_send_cmd(0x2C);

	uint32_t size = (x2 - x1 + 1) * (y2 - y1 + 1);
	uint16_t color_swap = ((color.full >> 8) & 0xFF) | ((color.full & 0xFF) << 8); //It's a 8 bit SPI bytes need to be swapped
	uint16_t buf[ILI9341_HOR_RES];

	uint32_t i;
	if (size < ILI9341_HOR_RES)
	{
		for (i = 0; i < size; i++)
			buf[i] = color_swap;
	}
	else
	{
		for (i = 0; i < ILI9341_HOR_RES; i++)
			buf[i] = color_swap;
	}

	while (size > ILI9341_HOR_RES)
	{
		ili9341_send_data(buf, ILI9341_HOR_RES * 2);
		size -= ILI9341_HOR_RES;
	}

	ili9341_send_data(buf, size * 2); //Send the remaining data
}

void ili9341_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t *color_map)
{
	uint8_t data[4];

	//Column addresses
	ili9341_send_cmd(0x2A);
	data[0] = (x1 >> 8) & 0xFF;
	data[1] = x1 & 0xFF;
	data[2] = (x2 >> 8) & 0xFF;
	data[3] = x2 & 0xFF;
	ili9341_send_data(data, 4);

	//Page addresses
	ili9341_send_cmd(0x2B);
	data[0] = (y1 >> 8) & 0xFF;
	data[1] = y1 & 0xFF;
	data[2] = (y2 >> 8) & 0xFF;
	data[3] = y2 & 0xFF;
	ili9341_send_data(data, 4);

	//Memory write
	ili9341_send_cmd(0x2C);

	uint32_t size = (x2 - x1 + 1) * (y2 - y1 + 1);

	//Byte swapping is required
	uint32_t i;
	uint8_t *color_u8 = (uint8_t *)color_map;
	uint8_t color_tmp;
	for (i = 0; i < size * 2; i += 2)
	{
		color_tmp = color_u8[i + 1];
		color_u8[i + 1] = color_u8[i];
		color_u8[i] = color_tmp;
	}

	while (size > ILI9341_HOR_RES)
	{

		ili9341_send_data((void *)color_map, ILI9341_HOR_RES * 2);
		size -= ILI9341_HOR_RES;
		color_map += ILI9341_HOR_RES;
	}

	ili9341_send_data((void *)color_map, size * 2); //Send the remaining data

	lv_flush_ready();
}
#endif

void ili9341_poweroff()
{
	esp_err_t err = ESP_OK;

	// fade off backlight
	ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (LCD_BACKLIGHT_ON_VALUE) ? 0 : DUTY_MAX, 100);
	ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_WAIT_DONE);

	// Disable LCD panel
	int cmd = 0;
	while (ili_sleep_cmds[cmd].databytes != 0xff)
	{
		ili9341_send_cmd(ili_sleep_cmds[cmd].cmd);
		ili9341_send_data(ili_sleep_cmds[cmd].data, ili_sleep_cmds[cmd].databytes & 0x7f);
		if (ili_sleep_cmds[cmd].databytes & 0x80)
		{
			vTaskDelay(100 / portTICK_RATE_MS);
		}
		cmd++;
	}

	err = rtc_gpio_init(ILI9341_BCKL);
	if (err != ESP_OK)
	{
		abort();
	}

	err = rtc_gpio_set_direction(ILI9341_BCKL, RTC_GPIO_MODE_OUTPUT_ONLY);
	if (err != ESP_OK)
	{
		abort();
	}

	err = rtc_gpio_set_level(ILI9341_BCKL, LCD_BACKLIGHT_ON_VALUE ? 0 : 1);
	if (err != ESP_OK)
	{
		abort();
	}
}

void ili9341_prepare()
{
	// Return use of backlight pin
	esp_err_t err = rtc_gpio_deinit(ILI9341_BCKL);
	if (err != ESP_OK)
	{
		abort();
	}

#if 0
    // Disable backlight
    err = gpio_set_direction(ILI9341_BCKL, GPIO_MODE_OUTPUT);
    if (err != ESP_OK)
    {
        abort();
    }

    err = gpio_set_level(ILI9341_BCKL, LCD_BACKLIGHT_ON_VALUE ? 0 : 1);
    if (err != ESP_OK)
    {
        abort();
    }
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void ili9341_send_cmd(uint8_t cmd)
{
	disp_spi_send(&cmd, 1, CMD_ON);
}

static void ili9341_send_data(void *data, uint16_t length)
{
	disp_spi_send(data, length, DATA_ON);
}

void ili9341_backlight_deinit()
{
	ledc_fade_func_uninstall();
	esp_err_t err = ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
	if (err != ESP_OK)
	{
		printf("%s: ledc_stop failed.\n", __func__);
	}
}

static void backlight_init()
{
	//configure timer0
	ledc_timer_config_t ledc_timer;
	memset(&ledc_timer, 0, sizeof(ledc_timer));

	ledc_timer.duty_resolution = LEDC_TIMER_13_BIT; //set timer counter bit number
	ledc_timer.freq_hz = 5000;						//set frequency of pwm
	ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;	//timer mode,
	ledc_timer.timer_num = LEDC_TIMER_0;			//timer index

	ledc_timer_config(&ledc_timer);

	//set the configuration
	ledc_channel_config_t ledc_channel;
	memset(&ledc_channel, 0, sizeof(ledc_channel));

	//set LEDC channel 0
	ledc_channel.channel = LEDC_CHANNEL_0;
	//set the duty for initialization.(duty range is 0 ~ ((2**duty_resolution)-1)
	ledc_channel.duty = (LCD_BACKLIGHT_ON_VALUE) ? 0 : DUTY_MAX;
	//GPIO number
	ledc_channel.gpio_num = ILI9341_BCKL;
	//GPIO INTR TYPE, as an example, we enable fade_end interrupt here.
	ledc_channel.intr_type = LEDC_INTR_FADE_END;
	//set LEDC mode, from ledc_mode_t
	ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
	//set LEDC timer source, if different channel use one timer,
	//the frequency and duty_resolution of these channels should be the same
	ledc_channel.timer_sel = LEDC_TIMER_0;

	ledc_channel_config(&ledc_channel);

	//initialize fade service.
	ledc_fade_func_install(0);

	// duty range is 0 ~ ((2**duty_resolution)-1)
	ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (LCD_BACKLIGHT_ON_VALUE) ? DUTY_MAX : 0, 500);
	ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);

	isBackLightIntialized = true;

	printf("Backlight initialization done.\n");
}