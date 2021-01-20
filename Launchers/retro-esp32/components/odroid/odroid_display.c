#pragma GCC optimize ("O3")


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "odroid_display.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
//#include "esp_event.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "driver/ledc.h"
#include "driver/rtc_io.h"

#include "esp_log.h"

#define LCD_HOST    HSPI_HOST
#define DMA_CHAN    2

const int DUTY_MAX = 0x1fff;

///*
const gpio_num_t SPI_PIN_NUM_MISO = GPIO_NUM_19;
const gpio_num_t SPI_PIN_NUM_MOSI = GPIO_NUM_23;
const gpio_num_t SPI_PIN_NUM_CLK  = GPIO_NUM_18;

const gpio_num_t LCD_PIN_NUM_CS   = GPIO_NUM_5;
const gpio_num_t LCD_PIN_NUM_DC   = GPIO_NUM_21;
const gpio_num_t LCD_PIN_NUM_BCKL = GPIO_NUM_14;
//*/

/*
const gpio_num_t SPI_PIN_NUM_MISO = GPIO_NUM_19;
const gpio_num_t SPI_PIN_NUM_MOSI = GPIO_NUM_23;
const gpio_num_t SPI_PIN_NUM_CLK  = GPIO_NUM_18;

const gpio_num_t LCD_PIN_NUM_CS   = GPIO_NUM_14;
const gpio_num_t LCD_PIN_NUM_DC   = GPIO_NUM_27;
const gpio_num_t LCD_PIN_NUM_BCKL = GPIO_NUM_32;
*/

const int LCD_BACKLIGHT_ON_VALUE = 1;
const int LCD_SPI_CLOCK_RATE = SPI_MASTER_FREQ_40M;

#define PARALLEL_LINES 16

#define SPI_TRANSACTION_COUNT (4)
static spi_transaction_t trans[SPI_TRANSACTION_COUNT];
static spi_device_handle_t spi;


#define LINE_BUFFERS (2)
#define LINE_COUNT (5)
uint16_t* line[LINE_BUFFERS];
QueueHandle_t spi_queue;
QueueHandle_t line_buffer_queue;
SemaphoreHandle_t line_semaphore;
SemaphoreHandle_t spi_empty;
SemaphoreHandle_t spi_count_semaphore;


bool isBackLightIntialized = false;


 // GB
#define GAMEBOY_WIDTH (160)
#define GAMEBOY_HEIGHT (144)


// SMS
#define SMS_WIDTH (256)
#define SMS_HEIGHT (192)

#define GAMEGEAR_WIDTH (160)
#define GAMEGEAR_HEIGHT (144)

#define PIXEL_MASK          (0x1F)


// NES
#define NES_GAME_WIDTH (256)
#define NES_GAME_HEIGHT (224) /* NES_VISIBLE_HEIGHT */


/*
 The ILI9341 needs a bunch of command/argument values to be initialized. They are stored in this struct.
*/
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} ili_init_cmd_t;

#define TFT_CMD_SWRESET 0x01
#define TFT_CMD_SLEEP 0x10
#define TFT_CMD_DISPLAY_OFF 0x28

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_MH 0x04
#define TFT_RGB_BGR 0x08


DRAM_ATTR static const ili_init_cmd_t ili_sleep_cmds[] = {
    {TFT_CMD_SWRESET, {0}, 0x80},
    {TFT_CMD_DISPLAY_OFF, {0}, 0x80},
    {TFT_CMD_SLEEP, {0}, 0x80},
    {0, {0}, 0xff}
};

/*
 CONFIG_LCD_DRIVER_CHIP_ODROID_GO
*/
#ifdef CONFIG_LCD_DRIVER_CHIP_ODROID_GO
DRAM_ATTR static const ili_init_cmd_t ili_init_cmds[] = {
    // VCI=2.8V
    //************* Start Initial Sequence **********//
    {TFT_CMD_SWRESET, {0}, 0x80},
    {0xCF, {0x00, 0xc3, 0x30}, 3},
    {0xED, {0x64, 0x03, 0x12, 0x81}, 4},
    {0xE8, {0x85, 0x00, 0x78}, 3},
    {0xCB, {0x39, 0x2c, 0x00, 0x34, 0x02}, 5},
    {0xF7, {0x20}, 1},
    {0xEA, {0x00, 0x00}, 2},
    {0xC0, {0x1B}, 1},    //Power control   //VRH[5:0]
    {0xC1, {0x12}, 1},    //Power control   //SAP[2:0];BT[3:0]
    {0xC5, {0x32, 0x3C}, 2},    //VCM control
    {0xC7, {0x91}, 1},    //VCM control2
    //{0x36, {(MADCTL_MV | MADCTL_MX | TFT_RGB_BGR)}, 1},    // Memory Access Control
    {0x36, {(MADCTL_MV | MADCTL_MY | TFT_RGB_BGR)}, 1},    // Memory Access Control
    {0x3A, {0x55}, 1},
    {0xB1, {0x00, 0x1B}, 2},  // Frame Rate Control (1B=70, 1F=61, 10=119)
    {0xB6, {0x0A, 0xA2}, 2},    // Display Function Control
    {0xF6, {0x01, 0x30}, 2},
    {0xF2, {0x00}, 1},    // 3Gamma Function Disable
    {0x26, {0x01}, 1},     //Gamma curve selected

    //Set Gamma
    {0xE0, {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00}, 15},
    {0XE1, {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F}, 15},

    {0x11, {0}, 0x80},    //Exit Sleep
    {0x29, {0}, 0x80},    //Display on

    {0, {0}, 0xff}
};
#endif

/*
 CONFIG_LCD_DRIVER_CHIP_CUTE_ESP32
*/
#ifdef CONFIG_LCD_DRIVER_CHIP_CUTE_ESP32

DRAM_ATTR static const ili_init_cmd_t ili_init_cmds[] = {
    {TFT_CMD_SWRESET, {0}, 0x80},
    {0x36, {(1<<7)|(1<<5)}, 1}, // MY | MV | RGB
    {0x3A, {0x55}, 1},
    {0xB2, {0x0c, 0x0c, 0x00, 0x33, 0x33}, 5},
    {0xB7, {0x35}, 1},
    {0xBB, {0x2B}, 1},
    {0xC0, {0x2C}, 1},
    {0xC2, {0x01, 0xFF}, 2},
    {0xC3, {0x11}, 1},
    {0xC4, {0x20}, 1},
    {0xC6, {0x0f}, 1},
    {0xD0, {0xA4, 0xA1}, 2},
    {0xE0, {0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19}, 14},
    {0xE1, {0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18, 0x16, 0x19}, 14},

    {0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},
    {0x2B, {0x00, 0x00, 0x00, 0x3F}, 4},

    {0x21, {0}, 0x80},

    {0x11, {0}, 0x80},
    {0x29, {0}, 0x80},
    {0, {0}, 0xff},
};
#endif

/*
 CONFIG_LCD_DRIVER_CHIP_RETRO_ESP32
*/
/*
 CONFIG_LCD_DRIVER_CHIP_RETRO_ESP32
*/
#if defined (CONFIG_LCD_DRIVER_CHIP_RETRO_ESP32) || defined(CONFIG_LCD_DRIVER_CHIP_RETRO_ESP32_NEXT)
DRAM_ATTR static const ili_init_cmd_t ili_init_cmds[] = {
    // VCI=2.8V
    //************* Start Initial Sequence **********//
    {TFT_CMD_SWRESET, {0}, 0x80},
    {0xCF, {0x00, 0xc3, 0x30}, 3},
    {0xED, {0x64, 0x03, 0x12, 0x81}, 4},
    {0xE8, {0x85, 0x00, 0x78}, 3},
    {0xCB, {0x39, 0x2c, 0x00, 0x34, 0x02}, 5},
    {0xF7, {0x20}, 1},
    {0xEA, {0x00, 0x00}, 2},
    {0xC0, {0x1B}, 1},    //Power control   //VRH[5:0]
    {0xC1, {0x12}, 1},    //Power control   //SAP[2:0];BT[3:0]
    {0xC5, {0x32, 0x3C}, 2},    //VCM control
    {0x3A, {0x55}, 1},
    {0xB1, {0x00, 0x1B}, 2},  // Frame Rate Control (1B=70, 1F=61, 10=119)
    {0xB6, {0x0A, 0xA2}, 2},    // Display Function Control
    {0xF6, {0x01, 0x30}, 2},
    {0xF2, {0x00}, 1},    // 3Gamma Function Disable
    {0x26, {0x01}, 1},     //Gamma curve selected

    //Set Gamma
    {0xE0, {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00}, 15},
    {0XE1, {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F}, 15},
    
    {0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},
    {0x2B, {0x00, 0x00, 0x00, 0x3F}, 4},

    #ifdef CONFIG_LCD_DRIVER_CHIP_RETRO_ESP32_NEXT
        {0x36, {(MADCTL_MX | MADCTL_MY | TFT_RGB_BGR)}, 1},    // Memory Access Control
    #endif

    #ifdef CONFIG_LCD_DRIVER_CHIP_RETRO_ESP32
        {0x36, {(MADCTL_MV | MADCTL_MY | TFT_RGB_BGR)}, 1},    // Memory Access Control
        {0x21, {0}, 0x80}, // <-- INVERT COLORS
    #endif

    {0x11, {0}, 0x80},    //Exit Sleep
    {0x29, {0}, 0x80},    //Display on
    {0, {0}, 0xff}
};
#endif


static uint16_t* line_buffer_get()
{
    uint16_t* buffer;
    if (xQueueReceive(line_buffer_queue, &buffer, 1000 / portTICK_RATE_MS) != pdTRUE)
    {
        abort();
    }

    return buffer;
}

void line_buffer_put(uint16_t* buffer)
{
    if (xQueueSend(line_buffer_queue, &buffer, 1000 / portTICK_RATE_MS) != pdTRUE)
    {
        abort();
    }
}

static void spi_task(void *arg)
{
    printf("%s: Entered.\n", __func__);

    uint16_t* param;
    while(1)
    {
        // Ensure only LCD transactions are pulled
        if(xSemaphoreTake(spi_count_semaphore, portMAX_DELAY) == pdTRUE )
        {
            spi_transaction_t* t;

            esp_err_t ret = spi_device_get_trans_result(spi, &t, portMAX_DELAY);
            assert(ret==ESP_OK);

            int dc = (int)t->user & 0x80;
            if(dc)
            {
                xSemaphoreGive(line_semaphore);
                line_buffer_put(t->tx_buffer);
            }

            if(xQueueSend(spi_queue, &t, portMAX_DELAY) != pdPASS)
            {
                abort();
            }

            if(uxQueueMessagesWaiting(spi_queue) >= SPI_TRANSACTION_COUNT)
            {
                xSemaphoreGive(spi_empty);
            }
        }
        else
        {
            printf("%s: xSemaphoreTake failed.\n", __func__);
        }
    }

    printf("%s: Exiting.\n", __func__);
    vTaskDelete(NULL);

    while (1) {}
}

static void spi_initialize()
{
    spi_queue = xQueueCreate(SPI_TRANSACTION_COUNT, sizeof(void*));
    if(!spi_queue) abort();


    line_buffer_queue = xQueueCreate(LINE_BUFFERS, sizeof(void*));
    if(!line_buffer_queue) abort();


    line_semaphore = xSemaphoreCreateCounting(LINE_BUFFERS, LINE_BUFFERS);
    if (!line_semaphore) abort();


    spi_empty = xSemaphoreCreateBinary();
    if(!spi_empty) abort();

    xSemaphoreGive(spi_empty);


    spi_count_semaphore = xSemaphoreCreateCounting(SPI_TRANSACTION_COUNT, 0);
    if (!spi_count_semaphore) abort();

    xTaskCreatePinnedToCore(&spi_task, "spi_task", 1024 + 768, NULL, 5, NULL, 1);
}



static spi_transaction_t* spi_get_transaction()
{
    spi_transaction_t* t;
    xQueueReceive(spi_queue, &t, portMAX_DELAY);

    memset(t, 0, sizeof(*t));

    return t;
}

static void spi_put_transaction(spi_transaction_t* t)
{
    t->rx_buffer = NULL;
    t->rxlength = t->length;

    if (t->flags & SPI_TRANS_USE_TXDATA)
    {
        t->flags |= SPI_TRANS_USE_RXDATA;
    }

    if (uxSemaphoreGetCount(spi_empty) > 0)
    {
        if(xSemaphoreTake(spi_empty, portMAX_DELAY) != pdTRUE)
        {
            abort();
        }
    }

    esp_err_t ret = spi_device_queue_trans(spi, t, portMAX_DELAY);
    assert(ret==ESP_OK);

    xSemaphoreGive(spi_count_semaphore);
}


//Send a command to the ILI9341. Uses spi_device_transmit, which waits until the transfer is complete.
static void ili_cmd(const uint8_t cmd)
{
    spi_transaction_t* t = spi_get_transaction();

    t->length = 8;                     //Command is 8 bits
    t->tx_data[0] = cmd;               //The data is the cmd itself
    t->user = (void*)0;                //D/C needs to be set to 0
    t->flags = SPI_TRANS_USE_TXDATA;

    spi_put_transaction(t);
}

//Send data to the ILI9341. Uses spi_device_transmit, which waits until the transfer is complete.
static void ili_data(const uint8_t *data, int len)
{
    if (!len) abort();

    spi_transaction_t* t = spi_get_transaction();

    if (len < 5)
    {
        for (int i = 0; i < len; ++i)
        {
            t->tx_data[i] = data[i];
        }
        t->length = len * 8;                 //Len is in bytes, transaction length is in bits.
        t->user = (void*)1;                //D/C needs to be set to 1
        t->flags = SPI_TRANS_USE_TXDATA;
    }
    else
    {
        t->length = len * 8;                 //Len is in bytes, transaction length is in bits.
        t->tx_buffer = data;               //Data
        t->user = (void*)1;                //D/C needs to be set to 1
        t->flags = 0; //SPI_TRANS_USE_TXDATA;
    }

    spi_put_transaction(t);
}

//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
static void ili_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user & 0x01;
    gpio_set_level(LCD_PIN_NUM_DC, dc);
}



//Initialize the display
static void ili_init()
{

    //Initialize non-SPI GPIOs
    gpio_set_direction(LCD_PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(LCD_PIN_NUM_BCKL, GPIO_MODE_OUTPUT);

    int cmd = 0;

    //Initialize non-SPI GPIOs
    gpio_set_direction(LCD_PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(LCD_PIN_NUM_BCKL, GPIO_MODE_OUTPUT);

    //Send all the commands
    while (ili_init_cmds[cmd].databytes != 0xff)
    {
        ili_cmd(ili_init_cmds[cmd].cmd);

        int len = ili_init_cmds[cmd].databytes & 0x7f;
        if (len) ili_data(ili_init_cmds[cmd].data, len);

        if (ili_init_cmds[cmd].databytes & 0x80)
        {
            vTaskDelay(100 / portTICK_RATE_MS);
        }

        cmd++;
    }
}


void send_reset_drawing(int left, int top, int width, int height)
{
    
    ili_cmd(0x2A);

    const uint8_t data1[] = { (left) >> 8, (left) & 0xff, (left + width - 1) >> 8, (left + width - 1) & 0xff };
    ili_data(data1, 4);

    ili_cmd(0x2B);          //Page address set

    const uint8_t data2[] = { top >> 8, top & 0xff, (top + height - 1) >> 8, (top + height - 1) & 0xff };
    ili_data(data2, 4);
   
    ili_cmd(0x2C);           //memory write
}

// static void wait_for_line_buffer()
// {
//     // if(xSemaphoreTake(line_semaphore, 1000 / portTICK_RATE_MS) != pdTRUE )
//     // {
//     //     abort();
//     // }
// }

void send_continue_wait()
{
    if(xSemaphoreTake(spi_empty, 1000 / portTICK_RATE_MS) != pdTRUE )
    {
        abort();
    }
}

void send_continue_line(uint16_t *line, int width, int lineCount)
{
    spi_transaction_t* t;

    #if defined (CONFIG_LCD_DRIVER_CHIP_ODROID_GO) || defined (CONFIG_LCD_DRIVER_CHIP_RETRO_ESP32)
        t = spi_get_transaction();


        t->tx_data[0] = 0x3C;   //memory write continue
        t->length = 8;
        t->user = (void*)0;
        t->flags = SPI_TRANS_USE_TXDATA;

        spi_put_transaction(t);
    #endif

    t = spi_get_transaction();

    t->length = width * 2 * lineCount * 8;
    t->tx_buffer = line;
    t->user = (void*)0x81;
    t->flags = 0;

    spi_put_transaction(t);
}

static void backlight_init()
{
    // Note: In esp-idf v3.0, settings flash speed to 80Mhz causes the LCD controller
    // to malfunction after a soft-reset.

    // (duty range is 0 ~ ((2**bit_num)-1)


    //configure timer0
    ledc_timer_config_t ledc_timer;
    memset(&ledc_timer, 0, sizeof(ledc_timer));

    ledc_timer.bit_num = LEDC_TIMER_13_BIT; //set timer counter bit number
    ledc_timer.freq_hz = 5000;              //set frequency of pwm
    ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;   //timer mode,
    ledc_timer.timer_num = LEDC_TIMER_0;    //timer index


    ledc_timer_config(&ledc_timer);


    //set the configuration
    ledc_channel_config_t ledc_channel;
    memset(&ledc_channel, 0, sizeof(ledc_channel));

    //set LEDC channel 0
    ledc_channel.channel = LEDC_CHANNEL_0;
    //set the duty for initialization.(duty range is 0 ~ ((2**bit_num)-1)
    ledc_channel.duty = (LCD_BACKLIGHT_ON_VALUE) ? 0 : DUTY_MAX;
    //GPIO number
    ledc_channel.gpio_num = LCD_PIN_NUM_BCKL;
    //GPIO INTR TYPE, as an example, we enable fade_end interrupt here.
    ledc_channel.intr_type = LEDC_INTR_FADE_END;
    //set LEDC mode, from ledc_mode_t
    ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    //set LEDC timer source, if different channel use one timer,
    //the frequency and bit_num of these channels should be the same
    ledc_channel.timer_sel = LEDC_TIMER_0;


    ledc_channel_config(&ledc_channel);


    //initialize fade service.
    ledc_fade_func_install(0);

    // duty range is 0 ~ ((2**bit_num)-1)
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (LCD_BACKLIGHT_ON_VALUE) ? DUTY_MAX : 0, 500);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);

    isBackLightIntialized = true;
}

#if 1
void backlight_percentage_set(int value)
{
    int duty = DUTY_MAX * (value * 0.01f);

    // //set the configuration
    // ledc_channel_config_t ledc_channel;
    // memset(&ledc_channel, 0, sizeof(ledc_channel));
    //
    // //set LEDC channel 0
    // ledc_channel.channel = LEDC_CHANNEL_0;
    // //set the duty for initialization.(duty range is 0 ~ ((2**bit_num)-1)
    // ledc_channel.duty = duty;
    // //GPIO number
    // ledc_channel.gpio_num = LCD_PIN_NUM_BCKL;
    // //GPIO INTR TYPE, as an example, we enable fade_end interrupt here.
    // ledc_channel.intr_type = LEDC_INTR_FADE_END;
    // //set LEDC mode, from ledc_mode_t
    // ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    // //set LEDC timer source, if different channel use one timer,
    // //the frequency and bit_num of these channels should be the same
    // ledc_channel.timer_sel = LEDC_TIMER_0;
    //
    //
    // ledc_channel_config(&ledc_channel);

    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty, 500);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
}
#endif

static uint16_t Blend(uint16_t a, uint16_t b)
{
  // Big endian
  // rrrrrGGG gggbbbbb

  char r0 = (a >> 11) & 0x1f;
  char g0 = (a >> 5) & 0x3f;
  char b0 = (a) & 0x1f;

  char r1 = (b >> 11) & 0x1f;
  char g1 = (b >> 5) & 0x3f;
  char b1 = (b) & 0x1f;

  uint16_t rv = ((r1 - r0) >> 1) + r0;
  uint16_t gv = ((g1 - g0) >> 1) + g0;
  uint16_t bv = ((b1 - b0) >> 1) + b0;

  return (rv << 11) | (gv << 5) | (bv);
}

void ili9341_write_frame_gb(uint16_t* buffer, int scale)
{
    short x, y;

    odroid_display_lock_gb_display();

    //xTaskToNotify = xTaskGetCurrentTaskHandle();

    if (buffer == NULL)
    {
        // clear the buffer
        for (int i = 0; i < LINE_BUFFERS; ++i)
        {
            memset(line[i], 0, 320 * sizeof(uint16_t) * LINE_COUNT);
        }

        // clear the screen
        send_reset_drawing(0, 0, 320, 240);

        for (y = 0; y < 240; y += LINE_COUNT)
        {
            uint16_t* line_buffer = line_buffer_get();
            send_continue_line(line_buffer, 320, LINE_COUNT);
        }
    }
    else
    {
        uint16_t* framePtr = buffer;

        if (scale)
        {
            // NOTE: LINE_COUNT must be 3 or greater
            const short outputWidth = 265;
            const short outputHeight = 240;

            send_reset_drawing(26, 0, outputWidth, outputHeight);

            for (y = 0; y < GAMEBOY_HEIGHT; y += 3)
            {
                uint16_t* line_buffer = line_buffer_get();

                for (int i = 0; i < 3; ++i)
                {
                    // skip middle vertical line
                    int index = i * outputWidth * 2;
                    int bufferIndex = ((y + i) * GAMEBOY_WIDTH);

                    for (x = 0; x < GAMEBOY_WIDTH; x += 3)
                    {
                        uint16_t a = framePtr[bufferIndex++];
                        uint16_t b;
                        uint16_t c;

                        if (x < GAMEBOY_WIDTH - 1)
                        {
                            b = framePtr[bufferIndex++];
                            c = framePtr[bufferIndex++];
                        }
                        else
                        {
                            b = framePtr[bufferIndex++];
                            c = 0;
                        }

                        uint16_t mid1 = Blend(a, b);
                        uint16_t mid2 = Blend(b, c);

                        line_buffer[index++] = ((a >> 8) | ((a) << 8));
                        line_buffer[index++] = ((mid1 >> 8) | ((mid1) << 8));
                        line_buffer[index++] = ((b >> 8) | ((b) << 8));
                        line_buffer[index++] = ((mid2 >> 8) | ((mid2) << 8));
                        line_buffer[index++] = ((c >> 8) | ((c ) << 8));
                    }
                }

                // Blend top and bottom lines into middle
                short sourceA = 0;
                short sourceB = outputWidth * 2;
                short sourceC = sourceB + (outputWidth * 2);

                short output1 = outputWidth;
                short output2 = output1 + (outputWidth * 2);

                for (short j = 0; j < outputWidth; ++j)
                {
                    uint16_t a = line_buffer[sourceA++];
                    a = ((a >> 8) | ((a) << 8));

                    uint16_t b = line_buffer[sourceB++];
                    b = ((b >> 8) | ((b) << 8));

                    uint16_t c = line_buffer[sourceC++];
                    c = ((c >> 8) | ((c) << 8));

                    uint16_t mid = Blend(a, b);
                    mid = ((mid >> 8) | ((mid) << 8));

                    line_buffer[output1++] = mid;

                    uint16_t mid2 = Blend(b, c);
                    mid2 = ((mid2 >> 8) | ((mid2) << 8));

                    line_buffer[output2++] = mid2;
                }

                // send the data
                send_continue_line(line_buffer, outputWidth, 5);
            }
        }
        else
        {
            send_reset_drawing((320 / 2) - (GAMEBOY_WIDTH / 2),
                (240 / 2) - (GAMEBOY_HEIGHT / 2),
                GAMEBOY_WIDTH,
                GAMEBOY_HEIGHT);

            for (y = 0; y < GAMEBOY_HEIGHT; y += LINE_COUNT)
            {
              uint16_t* line_buffer = line_buffer_get();

              int linesWritten = 0;

              for (int i = 0; i < LINE_COUNT; ++i)
              {
                  if((y + i) >= GAMEBOY_HEIGHT) break;

                  int index = (i) * GAMEBOY_WIDTH;
                  int bufferIndex = ((y + i) * GAMEBOY_WIDTH);

                  for (x = 0; x < GAMEBOY_WIDTH; ++x)
                  {
                    uint16_t sample = framePtr[bufferIndex++];
                    line_buffer[index++] = ((sample >> 8) | ((sample & 0xff) << 8));
                  }

                  ++linesWritten;
              }

              send_continue_line(line_buffer, GAMEBOY_WIDTH, linesWritten);
            }
        }
    }

    send_continue_wait();

    odroid_display_unlock_gb_display();
}

void ili9341_init()
{
    // Init
    spi_initialize();


    // Line buffers
    const size_t lineSize = 320 * LINE_COUNT * sizeof(uint16_t);
    for (int x = 0; x < LINE_BUFFERS; x++)
    {
        line[x] = heap_caps_malloc(lineSize, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        if (!line[x]) abort();

        printf("%s: line_buffer_put(%p)\n", __func__, line[x]);
        line_buffer_put(line[x]);
    }

    // Initialize transactions
    for (int x = 0; x < SPI_TRANSACTION_COUNT; x++)
    {
        void* param = &trans[x];
        xQueueSend(spi_queue, &param, portMAX_DELAY);
    }


    // Initialize SPI
    esp_err_t ret;
   
    #define TAG "TFT LCD"
	ESP_LOGI(TAG, "SPI_PIN_NUM_MOSI=%d",SPI_PIN_NUM_MOSI);
    ESP_LOGI(TAG, "SPI_PIN_NUM_MISO=%d",SPI_PIN_NUM_MISO);
	ESP_LOGI(TAG, "SPI_PIN_NUM_CLK=%d",SPI_PIN_NUM_CLK);
	spi_bus_config_t buscfg = {
		.sclk_io_num = SPI_PIN_NUM_CLK,
		.mosi_io_num = SPI_PIN_NUM_MOSI,
		.miso_io_num = SPI_PIN_NUM_MISO,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1
	};
    ret = spi_bus_initialize( HSPI_HOST, &buscfg, 1 );
    ESP_LOGD(TAG, "spi_bus_initialize=%d",ret);
    assert(ret==ESP_OK);

	spi_device_interface_config_t devcfg={
		.clock_speed_hz = LCD_SPI_CLOCK_RATE,
		.queue_size = 7,
		.mode = 2,
		.flags = SPI_DEVICE_NO_DUMMY,
        .pre_cb = ili_spi_pre_transfer_callback,
        .spics_io_num = LCD_PIN_NUM_CS >= 0 ? LCD_PIN_NUM_CS : -1
	};

	ret = spi_bus_add_device( HSPI_HOST, &devcfg, &spi);
	ESP_LOGD(TAG, "spi_bus_add_device=%d",ret);
	assert(ret==ESP_OK);       


    //Initialize the LCD
    printf("LCD: calling ili_init.\n");
    ili_init();

    printf("LCD: calling backlight_init.\n");
    backlight_init();

    printf("LCD Initialized (%d Hz).\n", LCD_SPI_CLOCK_RATE);
}

void ili9341_poweroff()
{
    // // Drain SPI queue
    // xTaskToNotify = 0;
    //
     esp_err_t err = ESP_OK;
    //
    // while(err == ESP_OK)
    // {
    //     spi_transaction_t* trans_desc;
    //     err = spi_device_get_trans_result(spi, &trans_desc, 0);
    //
    //     printf("ili9341_poweroff: removed pending transfer.\n");
    // }


    // fade off backlight
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (LCD_BACKLIGHT_ON_VALUE) ? 0 : DUTY_MAX, 100);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_WAIT_DONE);


    // Disable LCD panel
    int cmd = 0;
    while (ili_sleep_cmds[cmd].databytes != 0xff)
    {
        //printf("ili9341_poweroff: cmd=%d, ili_sleep_cmds[cmd].cmd=0x%x, ili_sleep_cmds[cmd].databytes=0x%x\n",
        //    cmd, ili_sleep_cmds[cmd].cmd, ili_sleep_cmds[cmd].databytes);

        ili_cmd(ili_sleep_cmds[cmd].cmd);
        ili_data(ili_sleep_cmds[cmd].data, ili_sleep_cmds[cmd].databytes & 0x7f);
        if (ili_sleep_cmds[cmd].databytes & 0x80)
        {
            vTaskDelay(100 / portTICK_RATE_MS);
        }
        cmd++;
    }


    err = rtc_gpio_init(LCD_PIN_NUM_BCKL);
    if (err != ESP_OK)
    {
        abort();
    }

    err = rtc_gpio_set_direction(LCD_PIN_NUM_BCKL, RTC_GPIO_MODE_OUTPUT_ONLY);
    if (err != ESP_OK)
    {
        abort();
    }

    err = rtc_gpio_set_level(LCD_PIN_NUM_BCKL, LCD_BACKLIGHT_ON_VALUE ? 0 : 1);
    if (err != ESP_OK)
    {
        abort();
    }
}

void ili9341_prepare()
{
    // Return use of backlight pin
    esp_err_t err = rtc_gpio_deinit(LCD_PIN_NUM_BCKL);
    if (err != ESP_OK)
    {
        abort();
    }

#if 0
    // Disable backlight
    err = gpio_set_direction(LCD_PIN_NUM_BCKL, GPIO_MODE_OUTPUT);
    if (err != ESP_OK)
    {
        abort();
    }

    err = gpio_set_level(LCD_PIN_NUM_BCKL, LCD_BACKLIGHT_ON_VALUE ? 0 : 1);
    if (err != ESP_OK)
    {
        abort();
    }
#endif
}

//
void ili9341_write_frame_sms(uint8_t* buffer, uint16_t color[], uint8_t isGameGear, uint8_t scale)
{
    short x, y;

    odroid_display_lock_sms_display();

    if (buffer == NULL)
    {
        // clear the buffer
        for (int i = 0; i < LINE_BUFFERS; ++i)
        {
            memset(line[i], 0, 320 * sizeof(uint16_t) * LINE_COUNT);
        }

        // clear the screen
        send_reset_drawing(0, 0, 320, 240);

        for (y = 0; y < 240; y += LINE_COUNT)
        {
            uint16_t* line_buffer = line_buffer_get();
            send_continue_line(line_buffer, 320, LINE_COUNT);
        }
    }
    else
    {
        uint8_t* framePtr = buffer;


        if (!isGameGear)
        {
            if (scale)
            {
                // const short xOffset = 4;
                // const uint16_t displayWidth = 320 - (xOffset / 4 * 5);
                // const short centerX = (320 - displayWidth) >> 1;

                //send_reset_drawing(centerX, 0, displayWidth, 240);

                const uint16_t displayWidth = 320;
                send_reset_drawing(0, 0, 320, 240);

                for (y = 0; y < SMS_HEIGHT; y += 4)
                {
                  int linesWritten = 0;
                  uint16_t* line_buffer = line_buffer_get();

                  for (short i = 0; i < 4; ++i)
                  {
                      if((y + i) >= SMS_HEIGHT)
                        break;

                      int index = (i) * displayWidth;
                      if (i > 1) index += displayWidth; // skip a line for blending

                      //int bufferIndex = ((y + i) * SMS_WIDTH) + xOffset;
                      int bufferIndex = ((y + i) * SMS_WIDTH);

                      uint16_t samples[4];

                      //for (x = 0; x < SMS_WIDTH - (xOffset * 2); x += 4)
                      for (x = 0; x < SMS_WIDTH; x += 4)
                      {
                        for (short j = 0; j < 4; ++j)
                        {
                            uint8_t val = framePtr[bufferIndex++] & PIXEL_MASK;
                            //
                            // uint8_t r = color[val][0];
                            // uint8_t g = color[val][1];
                            // uint8_t b = color[val][2];
                            //
                            // samples[j] = (((r << 8) & 0xF800) | ((g << 3) & 0x07E0) | ((b >> 3) & 0x001F));

                            samples[j] = color[val];
                        }

                        uint16_t mid1 = Blend(samples[1], samples[2]);

                        line_buffer[index++] = ((samples[0] >> 8) | (samples[0] << 8));
                        line_buffer[index++] = ((samples[1] >> 8) | (samples[1] << 8));
                        line_buffer[index++] = ((mid1 >> 8) | (mid1 << 8));
                        line_buffer[index++] = ((samples[2] >> 8) | (samples[2] << 8));
                        line_buffer[index++] = ((samples[3] >> 8) | (samples[3] << 8));
                      }

                      ++linesWritten;
                  }

                  // blend horizontal
                  short srcIndex1 = displayWidth * 1;
                  short srcIndex2 = displayWidth * 3;
                  short dstIndex = displayWidth * 2;

                  for (short i = 0; i < displayWidth; ++i)
                  {
                      uint16_t sample1 = line_buffer[srcIndex1++];
                      sample1 = ((sample1 >> 8) | (sample1 << 8));

                      uint16_t sample2 = line_buffer[srcIndex2++];
                      sample2 = ((sample2 >> 8) | (sample2 << 8));

                      uint16_t mid1 = Blend(sample1, sample2);

                      line_buffer[dstIndex++] = ((mid1 >> 8) | (mid1 << 8));
                  }

                  ++linesWritten;

                  // display
                  send_continue_line(line_buffer, displayWidth, linesWritten);
                }
            }
            else
            {
                send_reset_drawing((320 / 2) - (SMS_WIDTH / 2),
                    (240 / 2) - (SMS_HEIGHT / 2),
                    SMS_WIDTH,
                    SMS_HEIGHT);

                for (y = 0; y < SMS_HEIGHT; y += LINE_COUNT)
                {
                  int linesWritten = 0;
                  uint16_t* line_buffer = line_buffer_get();

                  for (short i = 0; i < LINE_COUNT; ++i)
                  {
                      if((y + i) >= SMS_HEIGHT)
                        break;

                      int index = (i) * SMS_WIDTH;
                      int bufferIndex = ((y + i) * SMS_WIDTH);

                      for (x = 0; x < SMS_WIDTH; ++x)
                      {
                        // uint8_t val = framePtr[bufferIndex++] & PIXEL_MASK;
                        //
                        // uint8_t r = color[val][0];
                        // uint8_t g = color[val][1];
                        // uint8_t b = color[val][2];
                        //
                        // uint16_t sample = (((r << 8) & 0xF800) | ((g << 3) & 0x07E0) | ((b >> 3) & 0x001F));

                        uint16_t sample = color[framePtr[bufferIndex++] & PIXEL_MASK];
                        line_buffer[index++] = ((sample >> 8) | (sample << 8));
                      }

                      ++linesWritten;
                  }

                  // display
                  send_continue_line(line_buffer, SMS_WIDTH, linesWritten);
                }
            }
        }
        else
        {
            // game Gear
            //framePtr += (24 * 256);

            if (scale)
            {
                const short outputWidth = 320;
                const short outputHeight = 240;

                send_reset_drawing(0, 0, outputWidth, outputHeight);

                for (y = 0; y < 144; y += 3)
                {
                    uint16_t* line_buffer = line_buffer_get();

                    for (short i = 0; i < 3; ++i)
                    {
                        // skip middle vertical line
                        int index = i * outputWidth * 2;
                        int bufferIndex = ((y + i) * 256) + 48;

                        for (x = 0; x < GAMEGEAR_WIDTH; ++x)
                        {
                            uint8_t val = framePtr[bufferIndex++] & PIXEL_MASK;

                            // uint8_t r = color[val][0];
                            // uint8_t g = color[val][1];
                            // uint8_t b = color[val][2];
                            //
                            // uint16_t sample = (((r << 8) & 0xF800) | ((g << 3) & 0x07E0) | ((b >> 3) & 0x001F));

                            uint16_t sample = color[val];
                            sample = (sample >> 8) | (sample << 8);

                            line_buffer[index++] = sample;
                            line_buffer[index++] = sample;
                        }
                    }

                    // Blend top and bottom lines into middle
                    short sourceA = 0;
                    short sourceB = outputWidth * 2;
                    short sourceC = sourceB + (outputWidth * 2);

                    short output1 = outputWidth;
                    short output2 = output1 + (outputWidth * 2);

                    for (short j = 0; j < outputWidth; ++j)
                    {
                      uint16_t a = line_buffer[sourceA++];
                      a = ((a >> 8) | ((a) << 8));

                      uint16_t b = line_buffer[sourceB++];
                      b = ((b >> 8) | ((b) << 8));

                      uint16_t c = line_buffer[sourceC++];
                      c = ((c >> 8) | ((c) << 8));

                      uint16_t mid = Blend(a, b);
                      mid = ((mid >> 8) | ((mid) << 8));

                      line_buffer[output1++] = mid;

                      uint16_t mid2 = Blend(b, c);
                      mid2 = ((mid2 >> 8) | ((mid2) << 8));

                      line_buffer[output2++] = mid2;
                    }

                    // send the data
                    send_continue_line(line_buffer, outputWidth, 5);
                }
            }
            else
            {
                send_reset_drawing((320 / 2) - (GAMEGEAR_WIDTH / 2),
                    (240 / 2) - (GAMEGEAR_HEIGHT / 2),
                    GAMEGEAR_WIDTH,
                    GAMEGEAR_HEIGHT);

                for (y = 0; y < GAMEGEAR_HEIGHT; y += LINE_COUNT)
                {
                  int linesWritten = 0;
                  uint16_t* line_buffer = line_buffer_get();

                  for (short i = 0; i < LINE_COUNT; ++i)
                  {
                      if((y + i) >= GAMEGEAR_HEIGHT)
                        break;

                      int index = (i) * GAMEGEAR_WIDTH;
                      int bufferIndex = ((y + i) * 256) + 48;

                      for (x = 0; x < GAMEGEAR_WIDTH; ++x)
                      {
                        uint8_t val = framePtr[bufferIndex++] & PIXEL_MASK;

                        // uint8_t r = color[val][0];
                        // uint8_t g = color[val][1];
                        // uint8_t b = color[val][2];
                        //
                        // uint16_t sample = (((r << 8) & 0xF800) | ((g << 3) & 0x07E0) | ((b >> 3) & 0x001F));

                        uint16_t sample = color[val];
                        line_buffer[index++] = ((sample >> 8) | (sample << 8));
                      }

                      ++linesWritten;
                  }

                  // display
                  send_continue_line(line_buffer, GAMEGEAR_WIDTH, linesWritten);
                }
            }
        }
    }

    send_continue_wait();
    odroid_display_unlock_sms_display();
}

//

void ili9341_write_frame_nes(uint8_t* buffer, uint16_t* myPalette, uint8_t scale)
{
    short x, y;

    odroid_display_lock_nes_display();

    //xTaskToNotify = xTaskGetCurrentTaskHandle();

    if (buffer == NULL)
    {
        // clear the buffer
        for (int i = 0; i < LINE_BUFFERS; ++i)
        {
            memset(line[i], 0, 320 * sizeof(uint16_t) * LINE_COUNT);
        }

        // clear the screen
        send_reset_drawing(0, 0, 320, 240);

        for (y = 0; y < 240; y += LINE_COUNT)
        {
            uint16_t* line_buffer = line_buffer_get();
            send_continue_line(line_buffer, 320, LINE_COUNT);
        }
    }
    else
    {
        uint8_t* framePtr = buffer;

        if (scale)
        {
            const uint16_t displayWidth = 320 - 10;
            const uint16_t top = (240 - NES_GAME_HEIGHT) / 2;

            send_reset_drawing((320 / 2) - (displayWidth / 2), top, displayWidth, NES_GAME_HEIGHT);

            for (y = 0; y < NES_GAME_HEIGHT; y += LINE_COUNT)
            {
              int linesWritten = 0;
              uint16_t* line_buffer = line_buffer_get();

              for (short i = 0; i < LINE_COUNT; ++i)
              {
                  if((y + i) >= NES_GAME_HEIGHT)
                    break;

                  int index = (i) * displayWidth;

                  int bufferIndex = ((y + i) * NES_GAME_WIDTH) + 4;

                  uint16_t samples[4];
                  for (x = 4; x < NES_GAME_WIDTH - 4; x += 4)
                  {
                    for (short j = 0; j < 4; ++j)
                    {
                        uint8_t val = framePtr[bufferIndex++];
                        samples[j] = myPalette[val];
                    }

                    uint16_t mid = Blend(samples[1] >> 8 | samples[1] << 8, samples[2] >> 8 | samples[2] << 8);

                    line_buffer[index++] = samples[0];
                    line_buffer[index++] = samples[1];
                    line_buffer[index++] = mid >> 8 | mid << 8;
                    line_buffer[index++] = samples[2];
                    line_buffer[index++] = samples[3];
                  }

                  ++linesWritten;
              }

              // display
              send_continue_line(line_buffer, displayWidth, linesWritten);
            }
        }
        else
        {
            send_reset_drawing((320 / 2) - (NES_GAME_WIDTH / 2), (240 / 2) - (NES_GAME_HEIGHT / 2), NES_GAME_WIDTH, NES_GAME_HEIGHT);

            for (y = 0; y < NES_GAME_HEIGHT; y += LINE_COUNT)
            {
              int linesWritten = 0;
              uint16_t* line_buffer = line_buffer_get();

              for (short i = 0; i < LINE_COUNT; ++i)
              {
                  if((y + i) >= NES_GAME_HEIGHT)
                    break;

                  int index = (i) * NES_GAME_WIDTH;
                  int bufferIndex = ((y + i) * NES_GAME_WIDTH);

                  for (x = 0; x < NES_GAME_WIDTH; ++x)
                  {
                    line_buffer[index++] = myPalette[framePtr[bufferIndex++]];
                  }

                  ++linesWritten;
              }

              // display
              send_continue_line(line_buffer, NES_GAME_WIDTH, linesWritten);
            }
        }
    }

    send_continue_wait();

    odroid_display_unlock_nes_display();
}

// void ili9341_write_frame(uint16_t* buffer)
// {
//     short x, y;
//
//     //xTaskToNotify = xTaskGetCurrentTaskHandle();
//
//     if (buffer == NULL)
//     {
//         // clear the buffer
//         memset(line[0], 0x00, 320 * sizeof(uint16_t));
//
//         // clear the screen
//         send_reset_drawing(0, 0, 320, 240);
//
//         for (y = 0; y < 240; ++y)
//         {
//             send_continue_line(line[0], 320, 1);
//         }
//
//         send_continue_wait();
//     }
//     else
//     {
//         const int displayWidth = 320;
//         const int displayHeight = 240;
//
//
//         send_reset_drawing(0, 0, displayWidth, displayHeight);
//
//         for (y = 0; y < displayHeight; y += 4)
//         {
//             send_continue_line(buffer + y * displayWidth, displayWidth, 4);
//         }
//
//         send_continue_wait();
//     }
// }

void ili9341_write_frame_rectangle(short left, short top, short width, short height, uint16_t* buffer)
{
    short x, y;

    if (left < 0 || top < 0) abort();
    if (width < 1 || height < 1) abort();

    //xTaskToNotify = xTaskGetCurrentTaskHandle();

    send_reset_drawing(left, top, width, height);

    if (buffer == NULL)
    {
        // clear the buffer
        for (int i = 0; i < LINE_BUFFERS; ++i)
        {
            memset(line[i], 0, 320 * sizeof(uint16_t) * LINE_COUNT);
        }

        // clear the screen
        send_reset_drawing(0, 0, 320, 240);

        for (y = 0; y < 240; y += LINE_COUNT)
        {
            uint16_t* line_buffer = line_buffer_get();
            send_continue_line(line_buffer, 320, LINE_COUNT);
        }
    }
    else
    {
        uint16_t* line_buffer = line_buffer_get();

        for (y = 0; y < height; y++)
        {
            memcpy(line_buffer, buffer + y * width, width * sizeof(uint16_t));
            send_continue_line(line_buffer, width, 1);
        }
    }

    send_continue_wait();
}

void ili9341_clear(uint16_t color)
{
    //xTaskToNotify = xTaskGetCurrentTaskHandle();

    send_reset_drawing(0, 0, 320, 240);


    // clear the buffer
    for (int i = 0; i < LINE_BUFFERS; ++i)
    {
        for (int j = 0; j < 320 * LINE_COUNT; ++j)
        {
            line[i][j] = color;
        }
    }

    // clear the screen
    send_reset_drawing(0, 0, 320, 240);

    for (int y = 0; y < 240; y += LINE_COUNT)
    {
        uint16_t* line_buffer = line_buffer_get();
        send_continue_line(line_buffer, 320, LINE_COUNT);
    }

    send_continue_wait();
}

void ili9341_write_frame_rectangleLE(short left, short top, short width, short height, uint16_t* buffer)
{
    short x, y;

    if (left < 0 || top < 0) abort();
    if (width < 1 || height < 1) abort();

    //xTaskToNotify = xTaskGetCurrentTaskHandle();

    send_reset_drawing(left, top, width, height);

    if (buffer == NULL)
    {
        // clear the buffer
        for (int i = 0; i < LINE_BUFFERS; ++i)
        {
            memset(line[i], 0, 320 * sizeof(uint16_t) * LINE_COUNT);
        }

        // clear the screen
        send_reset_drawing(0, 0, 320, 240);

        for (y = 0; y < 240; y += LINE_COUNT)
        {
            uint16_t* line_buffer = line_buffer_get();
            send_continue_line(line_buffer, 320, LINE_COUNT);
        }
    }
    else
    {
        for (y = 0; y < height; y++)
        {
            uint16_t* line_buffer = line_buffer_get();

            for (int i = 0; i < width; ++i)
            {
                uint16_t pixel = buffer[y * width + i];
                line_buffer[i] = pixel << 8 | pixel >> 8;
            }

            send_continue_line(line_buffer, width, 1);
        }
    }

    send_continue_wait();
}

void display_tasktonotify_set(int value)
{
    //xTaskToNotify = value;
}

int is_backlight_initialized()
{
    return isBackLightIntialized;
}

void odroid_display_drain_spi()
{
    // if(xSemaphoreTake(spi_empty, 1000 / portTICK_RATE_MS) != pdTRUE )
    // {
    //     abort();
    // }
}

SemaphoreHandle_t gb_mutex = NULL;

void odroid_display_lock_gb_display()
{
    if (!gb_mutex)
    {
        gb_mutex = xSemaphoreCreateMutex();
        if (!gb_mutex) abort();
    }

    if (xSemaphoreTake(gb_mutex, 1000 / portTICK_RATE_MS) != pdTRUE)
    {
        abort();
    }
}

void odroid_display_unlock_gb_display()
{
    if (!gb_mutex) abort();

    xSemaphoreGive(gb_mutex);
}


SemaphoreHandle_t nes_mutex = NULL;

void odroid_display_lock_nes_display()
{
    if (!nes_mutex)
    {
        nes_mutex = xSemaphoreCreateMutex();
        if (!nes_mutex) abort();
    }

    if (xSemaphoreTake(nes_mutex, 1000 / portTICK_RATE_MS) != pdTRUE)
    {
        abort();
    }
}

void odroid_display_unlock_nes_display()
{
    if (!nes_mutex) abort();

    xSemaphoreGive(nes_mutex);
}


SemaphoreHandle_t sms_mutex = NULL;

void odroid_display_lock_sms_display()
{
    if (!sms_mutex)
    {
        sms_mutex = xSemaphoreCreateMutex();
        if (!sms_mutex) abort();
    }

    if (xSemaphoreTake(sms_mutex, 1000 / portTICK_RATE_MS) != pdTRUE)
    {
        abort();
    }
}

void odroid_display_unlock_sms_display()
{
    if (!sms_mutex) abort();

    xSemaphoreGive(sms_mutex);
}