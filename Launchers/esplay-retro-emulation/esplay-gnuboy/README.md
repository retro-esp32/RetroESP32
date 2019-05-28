# ESPlay GNUBoy - ESP32 Gameboy emulator 

This branch only support [esplay-hardware], a esp32 device designed by me, and this branch only support ESP32 with PSRAM. If you're looking for old version please use esplay1.0 branch.

[esplay-hardware]: https://github.com/pebri86/esplay-hardware

Port of GNUBoy to ESP32, i use WROVER module.

Place ROMS in folder named 'roms/gb' on root of sdcard, create following structure on sdcard:

	/-

	|

 	--roms 
	
	|	|
		
	|	--gbc (place your ROMS here)
	
 	|

 	--esplay

   	|	|

   	|	--data (this to place state file as .sav)

		|
		
		--gbc
		
		|
		
		--nes

Compiling
---------

This code is an esp-idf project. You will need esp-idf to compile it. Newer versions of esp-idf may introduce incompatibilities with this code;
for your reference, the code was tested against release/v3.2 branch of esp-idf.

ROM
--- 
Place your ROM on SD Card in roms folder. Please provide ROM by yourself.

