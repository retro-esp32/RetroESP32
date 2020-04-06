

#include "cog.h"

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/rtc_io.h"

#include <string.h>

void cog_init(void) {
    printf("\n**********\n%s\n**********\n", __func__);
}

void cog_deinit() {
    printf("\n**********\n%s\n**********\n", __func__);
}
