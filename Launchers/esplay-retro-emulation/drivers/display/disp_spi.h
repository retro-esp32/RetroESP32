/**
 * @file disp_spi.h
 *
 */

#ifndef DISP_SPI_H
#define DISP_SPI_H

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

#define DISP_SPI_MOSI 23
#define DISP_SPI_CLK 18
#define DISP_SPI_CS 5
#define DISP_SPI_DC 12
#define CMD_ON 0
#define DATA_ON 1

    /**********************
 *      TYPEDEFS
 **********************/

    /**********************
 * GLOBAL PROTOTYPES
 **********************/
    void disp_spi_init(void);
    void disp_spi_send(uint8_t *data, uint16_t length, int dc);
    void send_lines(int ypos, int width, uint16_t *linedata, int lineCount);
    void send_lines_ext(int ypos, int xpos, int width, uint16_t *linedata, int lineCount);
    void send_line_finish(void);

    /**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*DISP_SPI_H*/
