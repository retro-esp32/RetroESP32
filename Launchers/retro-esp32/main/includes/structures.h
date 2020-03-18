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
  /* OPTIONS */{0, &settings ,&settings_color},
  /* FAVORITES */{0, &fav ,&fav_color},
  /* RECENT */{0, &recent ,&recent_color},
  /* NINTENDO ENTERTAINMENT SYSTEM */{0, &nes ,&nes_color},
  /* NINTENDO GAME BOY */{0, &gb ,&gb_color},
  /* NINTENDO GAME BOY COLOR */{0, &gbc ,&gbc_color},
  /* SEGA MASTER SYSTEM */{0, &sms ,&sms_color},
  /* SEGA GAME GEAR */{0, &gg ,&gg_color},
  /* COLECOVISION */{0, &col ,&col_color},
  /* SINCLAIR ZX SPECTRUM 48K */{0, &zx ,&zx_color},
  /* ATARI 2600 */{0, &a26 ,&a26_color},
  /* ATARI 7800 */{0, &a78 ,&a78_color},
  /* ATARI LYNX */{0, &lnx ,&lnx_color},
  /* PC ENGINE */{0, &pce ,&pce_color},
};

// THEME
typedef struct{
  int bg;
  int fg;
  int hl;
  char name[10];
} THEME;
THEME THEMES[22] = {
  {63877,51492,20610,"grapefruit"},
  {64452,51971,20770,"caramel"},
  {64963,52386,20961,"beer"},
  {65504,52832,21121,"mustard"},
  {49120,38497,14977,"slime"},
  {30689,24161,8833,"algae"},
  {8162,7778,4737,"jade"},
  {8174,7787,4740,"frog"},
  {8183,5746,2695,"seafoam"},
  {8191,5721,2698,"celeste"},
  {9727,9433,4586,"denim"},
  {11231,8985,4394,"sky"},
  {12511,8344,4202,"cobalt"},
  {28959,22777,8298,"indigo"},
  {47359,37081,14442,"plum"},
  {63743,51417,20586,"orchid"},
  {63799,51443,20583,"cupcake"},
  {63855,51500,20613,"lemonade"},
  {0,25388,48631,"night"},
  {25388,38066,55002,"carbon"},
  {44373,55002,65535,"smoke"},
  {65535,52825,16904,"cloud"},
};
THEME GUI;


