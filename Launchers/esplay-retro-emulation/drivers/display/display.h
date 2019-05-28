#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include <stdint.h>
#include "disp_spi.h"

//*****************************************************************************
//
// Make sure all of the definitions in this header have a C binding.
//
//*****************************************************************************

#define LCD_TYPE_ILI9341 0
#define LCD_TYPE_ILI9342 1

#if (CONFIG_HW_LCD_TYPE == LCD_TYPE_ILI9342)
    #include "ili9342.h"
    #define LCD_WIDTH ILI9342_HOR_RES
    #define LCD_HEIGHT ILI9342_VER_RES
#else
    #include "ili9341.h"
    #define LCD_WIDTH ILI9341_HOR_RES
    #define LCD_HEIGHT ILI9341_VER_RES
#endif

#ifdef __cplusplus
extern "C"
{
#endif

void display_init();
void backlight_deinit();
void write_frame_rectangleLE(short left, short top, short width, short height, uint16_t* buffer);
void display_prepare();
void display_show_hourglass();
void display_show_empty_battery();
void display_show_splash();
void display_clear(uint16_t color);
void set_display_brightness(int percent);
void display_prepare();
void display_poweroff();

#ifdef __cplusplus
}
#endif

#endif /*_DISPLAY_H_*/
