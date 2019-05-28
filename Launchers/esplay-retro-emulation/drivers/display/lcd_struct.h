/**
 * @file lcd_struct.h
 *
 */

#ifndef LCD_STRUCT_H
#define LCD_STRUCT_H

/*The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct. */
typedef struct
{
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

#endif /*LCD_STRUCT_H*/
