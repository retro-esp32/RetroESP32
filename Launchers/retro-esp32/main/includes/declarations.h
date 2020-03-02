/*
 Helpers
*/
static char *remove_ext (char* myStr, char extSep, char pathSep);
static const char *get_filename (char* myStr);
static const char *get_ext (char* myStr);
static int get_application (char* ext);

/*
 Debounce
*/
static void debounce(int key);

/*
 Text
*/
static int get_letter(char letter);
static void draw_text(short x, short y, const char *string, bool ext, bool current, bool remove);

/*
 Mask
*/
static void draw_mask(int x, int y, int w, int h);
static void draw_background();

/*
 Settings
*/
static void draw_settings();

/*
 Toggle
*/
static void draw_toggle();
static void get_toggle();
static void set_toggle();
static void draw_cover_toggle();
static void get_cover_toggle();
static void set_cover_toggle();

/*
 Volume
*/
static void draw_volume();
static int32_t get_volume();
static void set_volume();

/*
 Brightness
*/
static void draw_brightness();
static int32_t get_brightness();
static void set_brightness();
static void apply_brightness();

/*
 Theme
*/
static void draw_themes();
static void get_theme();
static void set_theme(int8_t i);
static void update_theme();

/*
 States
*/
static void get_step_state();
static void set_step_state();

static void get_list_state();
static void set_list_state();

static void set_restore_states();
static void get_restore_states();


/*
 GUI
*/
static void draw_systems();
static void draw_media(int x, int y, bool current, int offset);
static void draw_folder(int x, int y, bool current);
static void draw_battery();
static void draw_speaker();
static void draw_contrast();
static void draw_numbers();
static void delete_numbers();
static void draw_launcher();
static void draw_launcher_options();

/*
  Files
*/
static void count_files();
static void seek_files();
static void get_files();
//static void sort_files(char** files);
static void draw_files();
static void has_save_file(char *save_name);

/*
  Favorites
*/
static void create_favorites();
static void read_favorites();
static void add_favorite(char *favorite);
static void delete_favorite(char *favorite);
static void is_favorite(char *favorite);
static void get_favorites();
static void process_favorites();
static void draw_favorites();


/*
  Cover
*/

static void get_cover();
static void preview_cover(bool error);
static void draw_cover();

/*
  Animations
*/
static void animate(int dir);
static void restore_layout();
static void clean_up();

/*
  Boot Screens
*/
static void splash();
//static void boot();
static void restart();

/*
  ROM Options
*/
static void rom_run(bool resume);
static void rom_resume();
static void rom_delete_save();

/*
  Launcher
*/
static void launcher();
