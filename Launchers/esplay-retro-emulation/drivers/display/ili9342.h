/**
 * @file lv_templ.h
 *
 */

#ifndef __ILI9342_H__
#define __ILI9342_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*********************
 *      INCLUDES
 *********************/
#if CONFIG_USE_LVGL
#include "../lvgl/lvgl.h"
#endif

/*********************
 *      DEFINES
 *********************/
#define ILI9342_HOR_RES 320
#define ILI9342_VER_RES 240

#define ILI9342_BCKL 27

    /**********************
 *      TYPEDEFS
 **********************/

    /**********************
 * GLOBAL PROTOTYPES
 **********************/
    int ili9342_is_backlight_initialized();
    void ili9342_backlight_percentage_set(int value);
    void ili9342_init(void);
    void ili9342_backlight_deinit();
    void ili9342_prepare();
    void ili9342_poweroff();

#if CONFIG_USE_LVGL
    void ili9342_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t color);
    void ili9342_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t *color_map);
#endif

    /**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*ILI9342_H*/
