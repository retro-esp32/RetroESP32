//{#pragma region Includes
  #include "includes/core.h"
  #include "includes/declarations.h"
//}#pragma endregion Includes

//{#pragma region Buffer
  unsigned short buffer[40000];
//}#pragma endregion Buffer

uint16_t myPalette[256];

/*
  APPLICATION
*/
//{#pragma region Main
  void app_main(void) {
    nvs_flash_init();
    printf("\n----- %s  -----\n", __func__);
    
    // Display
    ili9341_init();
    draw_background();
    launcher();
  }
//}#pragma endregion Main   


//{#pragma region Mask
  void draw_mask(int x, int y, int w, int h){
    for (int i = 0; i < w * h; i++) buffer[i] = 49120;
    ili9341_write_frame(x, y, w, h, (const uint8_t **)buffer);
  }

  void draw_background() {
    int w = 320;
    int h = 60;
    for (int i = 0; i < 4; i++) draw_mask(0, i*h, w, h);
  }
//}#pragma endregion Mask

//{#pragma region Launcher
  static void launcher() {
    printf("\n----- %s  -----\n", __func__);

    int w = 320/4;
    int h = 240/4;    
    int x = (320/2)-(w/2);
    int y = (240/2)-(h/2);
    for (int i = 0; i < w * h; i++) buffer[i] = 65535;
    ili9341_write_frame(x, y, w, h, (const uint8_t **)buffer);    
  }
//}#pragma endregion Launcher  