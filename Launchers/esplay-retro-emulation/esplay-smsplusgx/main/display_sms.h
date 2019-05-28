#ifndef _DISPLAY_SMS_H_
#define _DISPLAY_SMS_H_
#include <stdint.h>
#include "settings.h"

// SMS
#define SMS_FRAME_WIDTH 256
#define SMS_FRAME_HEIGHT 192

// GG
#define GAMEGEAR_FRAME_WIDTH 160
#define GAMEGEAR_FRAME_HEIGHT 144

#define PIXEL_MASK (0x1F)

void write_sms_frame(const uint8_t *data, uint16_t color[], bool isGameGear, esplay_scale_option scale);

#endif /* _DISPLAY_SMS_H_ */