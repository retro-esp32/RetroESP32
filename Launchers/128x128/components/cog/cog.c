

#include "cog.h"

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/rtc_io.h"

#include <string.h>

/*
const gpio_num_t COG_CS     // 5
const gpio_num_t COG_RST    // RST
const gpio_num_t COG_RS 
const gpio_num_t COG_D7
const gpio_num_t COG_D6
const gpio_num_t COG_D3     // 23
const gpio_num_t COG_D0     // 18
*/

void debug(char *string);

void cog_init(void) {
    char message[256] = "";
    sprintf(message, "%s\nfile %s\nline #%d", __func__, __FILE__, __LINE__);
    debug(message);
}

void cog_deinit() {
    printf("\n**********\n%s\n**********\n", __func__);
}
