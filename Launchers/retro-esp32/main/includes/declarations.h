/*
 Helpers
*/
char *remove_ext (char* myStr, char extSep, char pathSep);
char *get_filename (char* myStr);
char *get_ext (char* myStr);
int get_application (char* ext);

/*
 Debounce
*/
void debounce(int key);

/*
 Text
*/
int get_letter(char letter);
void draw_text(short x, short y, char *string, bool ext, bool current, bool remove);

/*
 Mask
*/
void draw_mask(int x, int y, int w, int h);
void draw_background();

/*
 Settings
*/
void draw_settings();

/*
 Toggle
*/
void draw_toggle();
void get_toggle();
void set_toggle();
void draw_cover_toggle();
void get_cover_toggle();
void set_cover_toggle();

/*
 Volume
*/
void draw_volume();
int32_t get_volume();
void set_volume();

/*
 Brightness
*/
void draw_brightness();
int32_t get_brightness();
void set_brightness();
void apply_brightness();

/*
 Theme
*/
void draw_themes();
void get_theme();
void set_theme(int8_t i);
void update_theme();

/*
 States
*/
void get_step_state();
void set_step_state();

void get_list_state();
void set_list_state();

void set_restore_states();
void get_restore_states();


/*
 GUI
*/
void draw_systems();
void draw_media(int x, int y, bool current, int offset);
void draw_folder(int x, int y, bool current);
void draw_battery();
void draw_speaker();
void draw_contrast();
void draw_numbers();
void delete_numbers();
void draw_launcher();
void draw_launcher_options();

/*
  Files
*/
void count_files();
void seek_files();
void get_files();
void sort_files(char** files);
void draw_files();
void has_save_file(char *save_name);

/*
  Favorites
*/
void create_favorites();
void read_favorites();
void add_favorite(char *favorite);
void delete_favorite(char *favorite);
void is_favorite(char *favorite);
void get_favorites();
void process_favorites();
void draw_favorites();


/*
  Cover
*/

void get_cover();
void preview_cover(bool error);
void draw_cover();

/*
  Animations
*/
void animate(int dir);
void restore_layout();
void clean_up();

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
