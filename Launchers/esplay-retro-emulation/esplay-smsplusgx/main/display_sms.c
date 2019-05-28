#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "display_sms.h"
#include "display.h"
#include "disp_spi.h"

extern uint16_t *line[];

#define NEAREST_NEIGHBOR_ALG
#define LINE_COUNT (8)

static uint8_t getPixelSms(const uint8_t *bufs, int x, int y, int w1, int h1, int w2, int h2, bool isGameGear, int x_ratio, int y_ratio)
{
    uint8_t col;
#ifdef NEAREST_NEIGHBOR_ALG
    /* Resize using nearest neighbor alghorithm */
    /* Simple and fastest way but low quality   */
    int x2 = ((x*x_ratio)>>16);
    int y2 = ((y*y_ratio)>>16);
    if (isGameGear)
        col = bufs[y2*256+x2+48];
    else
        col = bufs[(y2*w1)+x2];

    return col;
#else
    /* Resize using bilinear interpolation */
    /* higher quality but lower performance, */
    int x_diff, y_diff, xv, yv, red, green, blue, col, a, b, c, d, index;

    xv = (int)((x_ratio * x) >> 16);
    yv = (int)((y_ratio * y) >> 16);

    x_diff = ((x_ratio * x) >> 16) - (xv);
    y_diff = ((y_ratio * y) >> 16) - (yv);

    if (isGameGear)
    {
        index = yv * 256 + xv + 48;
    }
    else
    {
        index = yv * w1 + xv;
    }

    a = bufs[index];
    b = bufs[index + 1];
    c = bufs[index + w1];
    d = bufs[index + w1 + 1];

    red = (((a >> 11) & 0x1f) * (1 - x_diff) * (1 - y_diff) + ((b >> 11) & 0x1f) * (x_diff) * (1 - y_diff) +
           ((c >> 11) & 0x1f) * (y_diff) * (1 - x_diff) + ((d >> 11) & 0x1f) * (x_diff * y_diff));

    green = (((a >> 5) & 0x3f) * (1 - x_diff) * (1 - y_diff) + ((b >> 5) & 0x3f) * (x_diff) * (1 - y_diff) +
             ((c >> 5) & 0x3f) * (y_diff) * (1 - x_diff) + ((d >> 5) & 0x3f) * (x_diff * y_diff));

    blue = (((a)&0x1f) * (1 - x_diff) * (1 - y_diff) + ((b)&0x1f) * (x_diff) * (1 - y_diff) +
            ((c)&0x1f) * (y_diff) * (1 - x_diff) + ((d)&0x1f) * (x_diff * y_diff));

    col = ((int)red << 11) | ((int)green << 5) | ((int)blue);

    return col;
#endif
}

void write_sms_frame(const uint8_t *data, uint16_t color[], bool isGameGear, esplay_scale_option scale)
{
    short x, y, xpos, ypos, outputWidth, outputHeight;
    int sending_line = -1;
    int calc_line = 0;
    int x_ratio, y_ratio;

    if (data == NULL)
    {
        for (y = 0; y < LCD_HEIGHT; ++y)
        {
            for (x = 0; x < LCD_WIDTH; x++)
            {
                line[calc_line][x] = 0;
            }
            if (sending_line != -1)
                send_line_finish();
            sending_line = calc_line;
            calc_line = (calc_line == 1) ? 0 : 1;
            send_lines_ext(y, 0, LCD_WIDTH, line[sending_line], 1);
        }
        send_line_finish();
    }
    else
    {
        if (!isGameGear)
        {
            switch (scale)
            {
            case SCALE_NONE:
                outputHeight = SMS_FRAME_HEIGHT;
                outputWidth = SMS_FRAME_WIDTH;
                xpos = (LCD_WIDTH - outputWidth) / 2;
                ypos = (LCD_HEIGHT - outputHeight) / 2;

                for (y = 0; y < outputHeight; y += LINE_COUNT)
                {
                    for (int i = 0; i < LINE_COUNT; ++i)
                    {
                        if ((y + i) >= outputHeight)
                            break;

                        int index = (i)*outputWidth;
                        int bufferIndex = ((y + i) * SMS_FRAME_WIDTH);

                        for (x = 0; x < outputWidth; ++x)
                        {
                            uint16_t sample = color[data[bufferIndex++] & PIXEL_MASK];
                            line[calc_line][index++] = ((sample >> 8) | ((sample) << 8));
                        }
                    }
                    if (sending_line != -1)
                        send_line_finish();
                    sending_line = calc_line;
                    calc_line = (calc_line == 1) ? 0 : 1;
                    send_lines_ext(ypos + y, xpos, outputWidth, line[sending_line], LINE_COUNT);
                }
                send_line_finish();
                break;

            case SCALE_STRETCH:

                outputHeight = LCD_HEIGHT;
                outputWidth = LCD_WIDTH;
                x_ratio = (int)((SMS_FRAME_WIDTH<<16)/outputWidth) +1;
                y_ratio = (int)((SMS_FRAME_HEIGHT<<16)/outputHeight) +1; 

                for (y = 0; y < outputHeight; y += LINE_COUNT)
                {
                    for (int i = 0; i < LINE_COUNT; ++i)
                    {
                        if ((y + i) >= outputHeight)
                            break;

                        int index = (i)*outputWidth;

                        for (x = 0; x < outputWidth; ++x)
                        {
                            uint16_t sample = color[getPixelSms(data, x, (y + i), SMS_FRAME_WIDTH, SMS_FRAME_HEIGHT, outputWidth, outputHeight, isGameGear, x_ratio, y_ratio) & PIXEL_MASK];
                            line[calc_line][index++] = ((sample >> 8) | ((sample) << 8));
                        }
                    }
                    if (sending_line != -1)
                        send_line_finish();
                    sending_line = calc_line;
                    calc_line = (calc_line == 1) ? 0 : 1;
                    send_lines_ext(y, 0, outputWidth, line[sending_line], LINE_COUNT);
                }
                send_line_finish();
                break;

            default:
                outputHeight = LCD_HEIGHT;
                outputWidth = SMS_FRAME_WIDTH + (LCD_HEIGHT - SMS_FRAME_HEIGHT);
                xpos = (LCD_WIDTH - outputWidth) / 2;
                x_ratio = (int)((SMS_FRAME_WIDTH<<16)/outputWidth) +1;
                y_ratio = (int)((SMS_FRAME_HEIGHT<<16)/outputHeight) +1;

                for (y = 0; y < outputHeight; y += LINE_COUNT)
                {
                    for (int i = 0; i < LINE_COUNT; ++i)
                    {
                        if ((y + i) >= outputHeight)
                            break;

                        int index = (i)*outputWidth;

                        for (x = 0; x < outputWidth; ++x)
                        {
                            uint16_t sample = color[getPixelSms(data, x, (y + i), SMS_FRAME_WIDTH, SMS_FRAME_HEIGHT, outputWidth, outputHeight, isGameGear, x_ratio, y_ratio) & PIXEL_MASK];
                            line[calc_line][index++] = ((sample >> 8) | ((sample) << 8));
                        }
                    }
                    if (sending_line != -1)
                        send_line_finish();
                    sending_line = calc_line;
                    calc_line = (calc_line == 1) ? 0 : 1;
                    send_lines_ext(y, xpos, outputWidth, line[sending_line], LINE_COUNT);
                }
                send_line_finish();
                break;
            }
        }
        else
        {
            switch (scale)
            {
            case SCALE_NONE:
                outputHeight = GAMEGEAR_FRAME_HEIGHT;
                outputWidth = GAMEGEAR_FRAME_WIDTH;
                xpos = (LCD_WIDTH - outputWidth) / 2;
                ypos = (LCD_HEIGHT - outputHeight) / 2;

                for (y = 0; y < outputHeight; y += LINE_COUNT)
                {
                    for (int i = 0; i < LINE_COUNT; ++i)
                    {
                        if ((y + i) >= outputHeight)
                            break;

                        int index = (i)*outputWidth;
                        int bufferIndex = ((y + i) * 256) + 48;

                        for (x = 0; x < outputWidth; ++x)
                        {
                            uint16_t sample = color[data[bufferIndex++] & PIXEL_MASK];
                            line[calc_line][index++] = ((sample >> 8) | ((sample) << 8));
                        }
                    }
                    if (sending_line != -1)
                        send_line_finish();
                    sending_line = calc_line;
                    calc_line = (calc_line == 1) ? 0 : 1;
                    send_lines_ext(ypos + y, xpos, outputWidth, line[sending_line], LINE_COUNT);
                }
                send_line_finish();
                break;

            case SCALE_STRETCH:
                outputHeight = LCD_HEIGHT;
                outputWidth = LCD_WIDTH;
                x_ratio = (int)((GAMEGEAR_FRAME_WIDTH<<16)/outputWidth) +1;
                y_ratio = (int)((GAMEGEAR_FRAME_HEIGHT<<16)/outputHeight) +1;

                for (y = 0; y < outputHeight; y += LINE_COUNT)
                {
                    for (int i = 0; i < LINE_COUNT; ++i)
                    {
                        if ((y + i) >= outputHeight)
                            break;

                        int index = (i)*outputWidth;

                        for (x = 0; x < outputWidth; ++x)
                        {
                            uint16_t sample = color[getPixelSms(data, x, (y + i), GAMEGEAR_FRAME_WIDTH, GAMEGEAR_FRAME_HEIGHT, outputWidth, outputHeight, isGameGear, x_ratio, y_ratio) & PIXEL_MASK];
                            line[calc_line][index++] = ((sample >> 8) | ((sample) << 8));
                        }
                    }
                    if (sending_line != -1)
                        send_line_finish();
                    sending_line = calc_line;
                    calc_line = (calc_line == 1) ? 0 : 1;
                    send_lines_ext(y, 0, outputWidth, line[sending_line], LINE_COUNT);
                }
                send_line_finish();
                break;

            default:
                outputHeight = LCD_HEIGHT;
                outputWidth = GAMEGEAR_FRAME_WIDTH + (LCD_HEIGHT - GAMEGEAR_FRAME_HEIGHT);
                xpos = (LCD_WIDTH - outputWidth) / 2;
                x_ratio = (int)((GAMEGEAR_FRAME_WIDTH<<16)/outputWidth) +1;
                y_ratio = (int)((GAMEGEAR_FRAME_HEIGHT<<16)/outputHeight) +1;

                for (y = 0; y < outputHeight; y += LINE_COUNT)
                {
                    for (int i = 0; i < LINE_COUNT; ++i)
                    {
                        if ((y + i) >= outputHeight)
                            break;

                        int index = (i)*outputWidth;

                        for (x = 0; x < outputWidth; ++x)
                        {
                            uint16_t sample = color[getPixelSms(data, x, (y + i), GAMEGEAR_FRAME_WIDTH, GAMEGEAR_FRAME_HEIGHT, outputWidth, outputHeight, isGameGear, x_ratio, y_ratio) & PIXEL_MASK];
                            line[calc_line][index++] = ((sample >> 8) | ((sample) << 8));
                        }
                    }
                    if (sending_line != -1)
                        send_line_finish();
                    sending_line = calc_line;
                    calc_line = (calc_line == 1) ? 0 : 1;
                    send_lines_ext(y, xpos, outputWidth, line[sending_line], LINE_COUNT);
                }
                send_line_finish();
                break;
            }
        }
    }
}