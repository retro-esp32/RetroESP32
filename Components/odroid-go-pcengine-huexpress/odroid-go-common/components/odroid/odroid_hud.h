/*
 Debounce
*/
void debounce(int key);

/*
  Debug
*/   
void hud_debug(char *string);

/*
  File
*/
void hud_prepare_delete(int del);
void hud_delete_save(char *file_to_delete);
void hud_check_saves(char *name);

/*
 Text
*/
int hud_letter(char letter);
void hud_text(short x, short y, char *string, bool ext, bool current);

/*
 Mask
*/
void hud_mask(int x, int y, int w, int h);
void hud_background();

/*
  Theme
*/   
void hud_theme();

/*
  Init
*/   
void hud_init();
void hud_deinit();

/*
  Menu
*/   
void hud_menu();

/*
  Display
*/   
void hud_logo(void);
void hud_progress(char *string, bool bar);
void hud_bar(int x, int y, int percent, bool active);
void hud_volume();
void hud_brightness();
void hud_frameskip();
void hud_options();