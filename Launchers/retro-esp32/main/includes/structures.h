// SCR
typedef struct{
  int x;
  int y;
  int w;
  int h;
} SCR;
SCR SCREEN = {0,0,320,240};

// OFFSET
typedef struct{
  int x;
  int y;
} OFFSET;
OFFSET POS = {64,32};
OFFSET ORIGIN = {16,48};

// LIST
typedef struct{
  int limit;
  int16_t offset;
  int16_t total;
  int pages;
  int page;
} LIST;
LIST ROMS = {8, 0, 0, 0, 0};

// FILE
typedef struct{
  char name[256];
  char path[256];
  char art[256];
  char ext[256];
  uint32_t crc;
  bool ready;
  bool favorite;
} LOAD;
LOAD ROM;

// SYSTEM
typedef struct SYSTEM_T{
  int x;
  uint16_t (*system)[32][32];
  uint16_t (*color)[32][32];
} SYSTEM;
SYSTEM SYSTEMS[COUNT] = {
  /* OPTIONS */{0, &settings,&settings_color},
  /* FAVORITES */{0, &fav,&fav_color},
  /* NINTENDO ENTERTAINMENT SYSTEM */{0, &nes,&nes_color},
  /* NINTENDO GAME BOY */{0, &gb,&gb_color},
  /* NINTENDO GAME BOY COLOR */{0, &gbc,&gbc_color},
  /* SEGA MASTER SYSTEM */{0, &sms,&sms_color},
  /* SEGA GAME GEAR */{0, &gg,&gg_color},
  /* COLECOVISION */{0, &col,&col_color},
  /* SINCLAIR ZX SPECTRUM 48K */{0, &zx,&zx_color},
  /* ATARI 2600 */{0, &a26,&a26_color},
  /* ATARI 7800 */{0, &a78,&a78_color},
  /* ATARI LYNX */{0, &lnx,&lnx_color},
  /* PC ENGINE */{0, &pce,&pce_color},
};

// THEME
typedef struct{
  int bg;
  int fg;
  char name[10];
} THEME;
THEME THEMES[22] = {
  {32768,54580,"maroon"},
  {57545,62839,"red"},
  {64143,65049,"pink"},
  {39684,56918,"brown"},
  {62470,65174,"orange"},
  {50604,61240,"apricot"},
  {33792,54932,"olive"},
  {65283,65461,"yellow"},
  {60845,63289,"beige"},
  {49000,59351,"lime"},
  {15753,48951,"green"},
  {45048,57340,"mint"},
  {17617,48858,"teal"},
  {18078,49023,"cyan"},
  {14,42297,"navy"},
  {17178,48733,"blue"},
  {37110,54652,"purple"},
  {52318,61119,"lavender"},
  {59804,62910,"magenta"},
  {0,42292,"black"},
  {16936,48631,"dark"},
  {29614,52857,"light"}
};
THEME GUI;
