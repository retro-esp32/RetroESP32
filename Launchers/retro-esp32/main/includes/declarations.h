/*
 Debounce
*/
void debounce(int key);

/*
 Text
*/
int get_letter(char letter);
void draw_text(short x, short y, char *string, bool ext, bool current);

/*
 Mask
*/
void draw_mask(int x, int y, int w, int h);
void draw_background();

/*
 Theme
*/
void draw_themes();
void get_theme();
void set_theme(int8_t i);
void update_theme();

/*
 GUI
*/
void draw_systems();
void draw_media(int x, int y, bool current);
void draw_battery();
void draw_numbers();
void draw_launcher();
void draw_options();

/*
  Files
*/
void draw_files();
void has_save_file(char *name);

/*
  Animations
*/
void animate(int dir);

/*
  Boot Screens
*/
void splash();
void boot();
void restart();

/*
  ROM Options
*/
void rom_run(bool resume);
void rom_resume();
void rom_delete_save();

/*
  Launcher
*/
static void launcher();