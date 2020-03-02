
//extern const uint8_t* battery_start asm("_binary_battery_raw_start");
//extern const uint8_t* battery_end   asm("_binary_battery_raw_end");

extern const uint16_t battery[16][16] asm("_binary_battery_raw_start");
