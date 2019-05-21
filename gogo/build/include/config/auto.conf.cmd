deps_config := \
	/Users/eugene/esp/esp-idf/components/app_trace/Kconfig \
	/Users/eugene/esp/esp-idf/components/aws_iot/Kconfig \
	/Users/eugene/esp/esp-idf/components/bt/Kconfig \
	/Users/eugene/esp/esp-idf/components/driver/Kconfig \
	/Users/eugene/esp/esp-idf/components/esp32/Kconfig \
	/Users/eugene/esp/esp-idf/components/esp_adc_cal/Kconfig \
	/Users/eugene/esp/esp-idf/components/esp_http_client/Kconfig \
	/Users/eugene/esp/esp-idf/components/ethernet/Kconfig \
	/Users/eugene/esp/esp-idf/components/fatfs/Kconfig \
	/Users/eugene/esp/esp-idf/components/freertos/Kconfig \
	/Users/eugene/esp/esp-idf/components/heap/Kconfig \
	/Users/eugene/esp/esp-idf/components/libsodium/Kconfig \
	/Users/eugene/esp/esp-idf/components/log/Kconfig \
	/Users/eugene/esp/esp-idf/components/lwip/Kconfig \
	/Users/eugene/esp/esp-idf/components/mbedtls/Kconfig \
	/Users/eugene/esp/esp-idf/components/openssl/Kconfig \
	/Users/eugene/esp/esp-idf/components/pthread/Kconfig \
	/Users/eugene/esp/esp-idf/components/spi_flash/Kconfig \
	/Users/eugene/esp/esp-idf/components/spiffs/Kconfig \
	/Users/eugene/esp/esp-idf/components/tcpip_adapter/Kconfig \
	/Users/eugene/esp/esp-idf/components/vfs/Kconfig \
	/Users/eugene/esp/esp-idf/components/wear_levelling/Kconfig \
	/Users/eugene/esp/esp-idf/components/bootloader/Kconfig.projbuild \
	/Users/eugene/esp/esp-idf/components/esptool_py/Kconfig.projbuild \
	/Users/eugene/esp/esp-idf/components/partition_table/Kconfig.projbuild \
	/Users/eugene/esp/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)

ifneq "$(IDF_CMAKE)" "n"
include/config/auto.conf: FORCE
endif

$(deps_config): ;
