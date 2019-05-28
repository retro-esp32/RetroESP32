/**
 * @file settings.h
 *
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>

    /*********************
 *      DEFINES
 *********************/

    /**********************
 *      TYPEDEFS
 **********************/
    typedef enum
    {
        ESPLAY_VOLUME_LEVEL0 = 0,
        ESPLAY_VOLUME_LEVEL1 = 1,
        ESPLAY_VOLUME_LEVEL2 = 2,
        ESPLAY_VOLUME_LEVEL3 = 3,
        ESPLAY_VOLUME_LEVEL4 = 4,

        _ODROID_VOLUME_FILLER = 0xffffffff
    } esplay_volume_level;

    typedef enum
    {
        SCALE_NONE = 0,
        SCALE_FIT = 1,
        SCALE_STRETCH = 2
    } esplay_scale_option;

    /**********************
 * GLOBAL PROTOTYPES
 **********************/
    void system_application_set(int slot);
    int32_t get_backlight_settings();
    void set_backlight_settings(int32_t value);
    char *system_util_GetFileName(const char *path);
    char *system_util_GetFileExtenstion(const char *path);
    char *system_util_GetFileNameWithoutExtension(const char *path);
    esplay_volume_level get_volume_settings();
    void set_volume_settings(esplay_volume_level value);
    char *get_rom_name_settings();
    void set_rom_name_settings(char *value);
    esplay_scale_option get_scale_option_settings();
    void set_scale_option_settings(esplay_scale_option scale);

    /**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*SETTINGS_H*/
