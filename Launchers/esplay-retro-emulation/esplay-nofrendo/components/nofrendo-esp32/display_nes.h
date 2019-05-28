#ifndef _DISPLAY_NES_H_
#define _DISPLAY_NES_H_
#include <stdint.h>
#include "settings.h"

#define NES_FRAME_WIDTH 256
#define NES_FRAME_HEIGHT 224

void write_nes_frame(const uint8_t * data, esplay_scale_option scale);

#endif /* _DISPLAY_NES_H_ */