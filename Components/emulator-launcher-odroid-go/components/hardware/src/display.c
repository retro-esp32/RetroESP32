#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"

#include "display.h"


static const gpio_num_t SPI_PIN_NUM_MISO = GPIO_NUM_19;
static const gpio_num_t SPI_PIN_NUM_MOSI = GPIO_NUM_23;
static const gpio_num_t SPI_PIN_NUM_CLK  = GPIO_NUM_18;

static const gpio_num_t LCD_PIN_NUM_CS   = GPIO_NUM_5;
static const gpio_num_t LCD_PIN_NUM_DC   = GPIO_NUM_21;
const int LCD_SPI_CLOCK_RATE = SPI_MASTER_FREQ_40M;

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_MH 0x04
#define TFT_RGB_BGR 0x08

static spi_transaction_t trans[8];
static spi_device_handle_t spi;
static TaskHandle_t xTaskToNotify = NULL;
static bool waitForTransactions = false;

#define PARALLEL_LINES (5)

static uint16_t* pbuf[2];
gbuf_t *fb = NULL;

/*
 The ILI9341 needs a bunch of command/argument values to be initialized. They are stored in this struct.
*/
typedef struct {
    uint8_t cmd;
    uint8_t data[128];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} ili_init_cmd_t;

#define TFT_CMD_SWRESET 0x01
#define TFT_CMD_SLEEP 0x10
#define TFT_CMD_DISPLAY_OFF 0x28

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
 CONFIG_LCD_DRIVER_CHIP_RETRO_ESP32
*/
#ifdef CONFIG_LCD_DRIVER_CHIP_RETRO_ESP32
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
    
    // ILI9342 Specific
    {0x36, {0x40|0x80|0x08}, 1}, // <-- ROTATE
    {0x21, {0}, 0x80}, // <-- INVERT COLORS

    {0x11, {0}, 0x80},    //Exit Sleep
    {0x29, {0}, 0x80},    //Display on
    {0, {0}, 0xff}
};
#endif

// Send a command to the ILI9341. Uses spi_device_transmit, which waits until
// the transfer is complete.
static void ili_cmd(spi_device_handle_t spi, const uint8_t cmd)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));   // Zero out the transaction
    t.length = 8;               // Command is 8 bits
    t.tx_buffer = &cmd;         // The data is the cmd itself
    t.user = (void*)0;          // D/C needs to be set to 0
    ret = spi_device_transmit(spi, &t); // Transmit!
    assert(ret == ESP_OK);      //Should have had no issues.
}

// Send data to the ILI9341. Uses spi_device_transmit, which waits until the
// transfer is complete.
static void ili_data(spi_device_handle_t spi, const uint8_t *data, int len)
{
    esp_err_t ret;
    spi_transaction_t t;
    if (len == 0) return;       // No need to send anything
    memset(&t, 0, sizeof(t));   // Zero out the transaction
    t.length = len * 8;         // Len is in bytes, transaction length is in bits.
    t.tx_buffer = data;         // Data
    t.user = (void*)1;          // D/C needs to be set to 1
    ret = spi_device_transmit(spi, &t); // Transmit!
    assert(ret == ESP_OK);      // Should have had no issues.
}

// This function is called (in irq context!) just before a transmission starts.
// It will set the D/C line to the value indicated in the user field.
static void ili_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc = (int)t->user;
    gpio_set_level(LCD_PIN_NUM_DC, dc);
}

static void ili_spi_post_transfer_callback(spi_transaction_t *t)
{
    if(xTaskToNotify && t == &trans[7]) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        /* Notify the task that the transmission is complete. */
        vTaskNotifyGiveFromISR(xTaskToNotify, &xHigherPriorityTaskWoken);

        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}

// Initialize the display
static void ili_init()
{
    int cmd=0;
    // Initialize non-SPI GPIOs
    gpio_set_direction(LCD_PIN_NUM_DC, GPIO_MODE_OUTPUT);

    // Send all the commands
    while (ili_init_cmds[cmd].databytes!=0xff) {
        ili_cmd(spi, ili_init_cmds[cmd].cmd);
        ili_data(spi, ili_init_cmds[cmd].data, ili_init_cmds[cmd].databytes&0x7F);
        if (ili_init_cmds[cmd].databytes&0x80) {
            vTaskDelay(100 / portTICK_RATE_MS);
        }
        cmd++;
    }
}

static void send_reset_drawing(int x, int y, int width, int height)
{
  trans[0].tx_data[0] = 0x2A;       // Column Address Set
  trans[1].tx_data[0] = x >> 8;     // Start Col High
  trans[1].tx_data[1] = x & 0xff;   // Start Col Low
  trans[1].tx_data[2] = (x + width - 1) >> 8;       // End Col High
  trans[1].tx_data[3] = (x + width - 1) & 0xff;     // End Col Low
  trans[2].tx_data[0] = 0x2B;       // Page address set
  trans[3].tx_data[0] = y >> 8;     // Start page high
  trans[3].tx_data[1] = y & 0xff;   // Start page low
  trans[3].tx_data[2] = (y + height - 1) >> 8;      // End page high
  trans[3].tx_data[3] = (y + height - 1) & 0xff;    // End page low
  trans[4].tx_data[0] = 0x2C;       // Memory write

  // Queue all transactions.
  for (int x = 0; x < 5; x++) {
      esp_err_t ret = spi_device_queue_trans(spi, &trans[x], 1000 / portTICK_RATE_MS);
      assert(ret == ESP_OK);
  }
}

static void send_continue_wait()
{
  if (waitForTransactions) {
    ulTaskNotifyTake(pdTRUE, 1000 / portTICK_RATE_MS);

    // Drain SPI queue
    esp_err_t err = ESP_OK;
    while(err == ESP_OK) {
        spi_transaction_t* trans_desc;
        err = spi_device_get_trans_result(spi, &trans_desc, 0);
    }

    waitForTransactions = false;
  }
}

static void send_continue_line(uint16_t *line, int width, int height)
{
  send_continue_wait();

  trans[6].tx_data[0] = 0x3C;   //memory write continue
  trans[6].length = 8;          //Data length, in bits
  trans[6].flags = SPI_TRANS_USE_TXDATA;

  trans[7].tx_buffer = line;    //finally send the line data
  trans[7].length = width * height * 16; //Data length, in bits
  trans[7].rxlength = 0;
  trans[7].flags = 0;

  // Queue transactions.
  for (int x = 6; x < 8; x++) {
      esp_err_t ret = spi_device_queue_trans(spi, &trans[x], 1000 / portTICK_RATE_MS);
      assert(ret == ESP_OK);
  }

  waitForTransactions = true;
}

static uint16_t *get_pbuf(void) {
    static uint16_t *current = NULL;
    if (current == NULL) {
        current = pbuf[0];
    }
    uint16_t *result = current;

    if (current == pbuf[0]) {
        current = pbuf[1];
    } else {
        current = pbuf[0];
    }
    return result;
}

void display_init(void)
{
    fb = gbuf_new(DISPLAY_WIDTH, DISPLAY_HEIGHT, 2, BIG_ENDIAN);
    memset(fb->data, 0, DISPLAY_WIDTH * DISPLAY_HEIGHT * 2);

    pbuf[0] = heap_caps_malloc(320 * PARALLEL_LINES * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!pbuf[0]) abort();

    pbuf[1] = heap_caps_malloc(320 * PARALLEL_LINES * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!pbuf[1]) abort();

  // Initialize transactions
    for (int x = 0; x < 8; x++) {
        memset(&trans[x], 0, sizeof(spi_transaction_t));
        if ((x & 1) == 0) {
            // Even transfers are commands
            trans[x].length = 8;
            trans[x].user = (void*)0;
        } else {
            // Odd transfers are data
            trans[x].length = 8 * 4;
            trans[x].user = (void*)1;
        }
        trans[x].flags = SPI_TRANS_USE_TXDATA;
    }

    // Initialize SPI
    esp_err_t ret;
    spi_bus_config_t buscfg;

    memset(&buscfg, 0, sizeof(buscfg));

    buscfg.miso_io_num = SPI_PIN_NUM_MISO;
    buscfg.mosi_io_num = SPI_PIN_NUM_MOSI;
    buscfg.sclk_io_num = SPI_PIN_NUM_CLK;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;

    spi_device_interface_config_t devcfg;

    memset(&devcfg, 0, sizeof(devcfg));

    devcfg.clock_speed_hz = LCD_SPI_CLOCK_RATE;
    devcfg.mode = 0;                                // SPI mode 0
    devcfg.spics_io_num = LCD_PIN_NUM_CS;           // CS pin
    devcfg.queue_size = 7;                          // We want to be able to queue 7 transactions at a time
    devcfg.pre_cb = ili_spi_pre_transfer_callback;  // Specify pre-transfer callback to handle D/C line
    devcfg.post_cb = ili_spi_post_transfer_callback;
    devcfg.flags = SPI_DEVICE_NO_DUMMY;

    ret = spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    assert(ret == ESP_OK);

    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    assert(ret == ESP_OK);

    ili_init();
}

void display_drain(void)
{
    // Drain SPI queue
    xTaskToNotify = 0;

    esp_err_t err = ESP_OK;

    while(err == ESP_OK) {
        spi_transaction_t* trans_desc;
        err = spi_device_get_trans_result(spi, &trans_desc, 0);
    }
}

void display_poweroff()
{
    display_drain();

    // Disable LCD panel
    int cmd = 0;
    while (ili_sleep_cmds[cmd].databytes != 0xff) {
        ili_cmd(spi, ili_sleep_cmds[cmd].cmd);
        ili_data(spi, ili_sleep_cmds[cmd].data, ili_sleep_cmds[cmd].databytes & 0x7f);
        if (ili_sleep_cmds[cmd].databytes & 0x80) {
            vTaskDelay(100 / portTICK_RATE_MS);
        }
        cmd++;
    }
}

void display_clear(uint16_t color)
{
    xTaskToNotify = xTaskGetCurrentTaskHandle();

    send_reset_drawing(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    uint16_t *pbuf = get_pbuf();

    // clear the buffer
    for (int i = 0; i < DISPLAY_WIDTH * PARALLEL_LINES; i++) {
        pbuf[i] = (color << 8) | (color >> 8);
    }

    // clear the screen
    for (short dy = 0; dy < DISPLAY_HEIGHT; dy += PARALLEL_LINES) {
        send_continue_line(pbuf, DISPLAY_WIDTH, PARALLEL_LINES);
    }

    send_continue_wait();
}

void display_update(void)
{
    xTaskToNotify = xTaskGetCurrentTaskHandle();

    send_reset_drawing(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    for (short dy = 0; dy < DISPLAY_HEIGHT; dy += PARALLEL_LINES) {
        uint16_t *pbuf = get_pbuf();
        memcpy(pbuf, ((uint16_t *)fb->data) + DISPLAY_WIDTH * dy, DISPLAY_WIDTH * PARALLEL_LINES * sizeof(uint16_t));
        send_continue_line(pbuf, DISPLAY_WIDTH, PARALLEL_LINES);
    }

    send_continue_wait();
}

void display_update_rect(rect_t r)
{
    assert(r.x >= 0);
    assert(r.y >= 0);
    assert(r.width > 0);
    assert(r.height > 0);
    assert(r.x + r.width <= DISPLAY_WIDTH);
    assert(r.y + r.height <= DISPLAY_HEIGHT);

    xTaskToNotify = xTaskGetCurrentTaskHandle();

    send_reset_drawing(r.x, r.y, r.width, r.height);

    if (r.width == DISPLAY_WIDTH) {
        for (short dy = 0; dy < r.height; dy += PARALLEL_LINES) {
            uint16_t *pbuf = get_pbuf();
            short numLines = r.height - dy;
            numLines = numLines < PARALLEL_LINES ? numLines : PARALLEL_LINES;
            memcpy(pbuf, ((uint16_t *)fb->data) + DISPLAY_WIDTH * (r.y + dy) + r.x, r.width * numLines * sizeof(uint16_t));
            send_continue_line(pbuf, r.width, numLines);
        }
    } else {
        for (short dy = 0; dy < r.height; dy += PARALLEL_LINES) {
            uint16_t *pbuf = get_pbuf();
            short numLines = r.height - dy;
            numLines = numLines < PARALLEL_LINES ? numLines : PARALLEL_LINES;
            for (short line = 0; line < numLines; line++) {
                memcpy(pbuf + r.width * line, ((uint16_t *)fb->data) + DISPLAY_WIDTH * (r.y + dy + line) + r.x, r.width * sizeof(uint16_t));
            }
            send_continue_line(pbuf, r.width, numLines);
        }
    }

    send_continue_wait();
}
