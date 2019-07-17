#pragma once

void ili9341_init();
void ili9341_write_frame(uint16_t* buffer);
void ili9341_write_frame_rectangle(short left, short top, short width, short height, uint16_t* buffer);
void ili9341_write_frame_rectangleLE(short left, short top, short width, short height, uint16_t* buffer);

void ili9341_clear(uint16_t color);

void backlight_deinit();