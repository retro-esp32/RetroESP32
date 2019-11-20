#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/i2s.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_task_wdt.h"
#include "esp_spiffs.h"
#include "driver/rtc_io.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"


#include "../components/odroid/odroid_settings.h"
#include "../components/odroid/odroid_input.h"
#include "../components/odroid/odroid_display.h"
#include "../components/odroid/odroid_audio.h"
#include "../components/odroid/odroid_system.h"
#include "../components/odroid/odroid_sdcard.h"
#include "../components/odroid/odroid_ui.h"

#include "../components/spectrum/spmain.h"
#include "../components/spectrum/vgascr.h"
#include "../components/spectrum/spperif.h"
#include "../components/spectrum/snapshot.h"
#include "../components/spectrum/spsound.h"
#include "../components/spectrum/misc.h"

extern int my_lastborder;
extern int sp_nosync;
extern int kbpos;
extern unsigned char rom_imag[];

#include <string.h>
#include <dirent.h>

#ifdef CONFIG_IN_GAME_MENU_YES
  #include <stdarg.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include "../components/odroid/odroid_hud.h"
  int ACTION;
#endif

extern int debug_trace;

int keyboard=0; // on-screen keyboard active?
int b_up=6,b_down=5,b_left=4,b_right=7;
int b_a=9,b_b=8,b_select=39,b_start=29; // 8 button maps

char key[40][6]={
"1","2","3","4","5","6","7","8","9","0",
"q","w","e","r","t","y","u","i","o","p",
"a","s","d","f","g","h","j","k","l","enter",
"caps","z","x","c","v","b","n","m","symbl","space"
};

odroid_gamepad_state joystick;

odroid_battery_state battery_state;

#define AUDIO_SAMPLE_RATE (15600) // 624 samples/frame at 25fps

int BatteryPercent = 100;

uint16_t* menuFramebuffer = 0;

unsigned short *buffer;  // screen buffer.
const char* SD_BASE_PATH = "/sd";
char path[256]="/sd/roms/spectrum";
char file[256]="/sd/roms/spectrum/basic.sna";
bool use_goemu = false;

//----------------------------------------------------------------
void print(short x, short y, const char *s)
{
    int rompos,i,n,k,a,len,idx;
    char c;
    extern unsigned short  *buffer; // screen buffer, 16 bit pixels
    
    len=strlen(s);
    for (k=0; k<len; k++) {
      c=s[k];
      rompos=15616; // start of character set 
      rompos+=c*8-256;
      
      for (i=0; i<8; i++) {
        a=rom_imag[rompos++];
	for (n=7,idx=i*len*8+k*8+7; n>=0; n--, idx--) {
	  if (a%2==1) buffer[idx]=65535; else buffer[idx]=0;
	  a=a>>1;
	}
      }   
    }
    ili9341_write_frame_rectangle(x*8,y*8,len*8,8,buffer);
}

//----------------------------------------------------------------
void printx2(short x, short y, const char *s)
{
    int rompos,i,n,k,a,len,idx;
    char c;
    extern unsigned short  *buffer; // screen buffer, 16 bit pixels
    
    len=strlen(s);
    for (k=0; k<len; k++) {
      c=s[k];
      rompos=15616; // start of character set 
      rompos+=c*8-256;
      
      for (i=0; i<8; i++) {
        a=rom_imag[rompos++];
	for (n=7,idx=i*len*32+k*16+14; n>=0; n--, idx-=2) {
	  if (a%2==1) {buffer[idx]=-1; buffer[idx+1]=-1; buffer[idx+len*16]=-1; buffer[idx+len*16+1]=-1; }
	  else {buffer[idx]=0; buffer[idx+1]=0; buffer[idx+len*16]=0; buffer[idx+len*16+1]=0;}
	  a=a>>1;
	}
      }   
    }
    ili9341_write_frame_rectangle(x*8,y*8,len*16,16,buffer);
}

//----------------------------------------------------------------
void debounce(int key)
{
  while (joystick.values[key]) 
         odroid_input_gamepad_read(&joystick);
}

//---------------------------------------------------------------

int choose_file() 
{
  FILE *fp;
  struct dirent *de; 
  int y=0,count,redraw=0,len;
  DIR *dr;
  
  while(1) { // stay in file chooser until something loaded or menu button pressed
    redraw=0; count=0;
    ili9341_clear(0); // clear screen
    printx2(5,0,"Load Snapshot");
    print(0,3,path);
    dr = opendir(path);
    if (dr == NULL)  // opendir returns NULL if couldn't open directory
      {
        printf("Could not open current directory" );
        return 0;
      }
      count=0;
      while ((de = readdir(dr)) != NULL) {
            if (count/20==y/20) {
	      if (strlen(de->d_name)>35) de->d_name[35]=0;
	      print(1,(count%20)+5, de->d_name);
	      if (de->d_type==DT_DIR) print(1+strlen(de->d_name),(count%20)+5,"/");
	    }
	    count++;
      } 
      closedir(dr);
      print(0,(y%20)+5,">");
      
      // process key presses...
      while(!joystick.values[ODROID_INPUT_A] && 
           !joystick.values[ODROID_INPUT_B] &&
           !redraw)
      {
        odroid_input_gamepad_read(&joystick);
	if (joystick.values[ODROID_INPUT_UP]) {
	  debounce(ODROID_INPUT_UP);
	  print(0,(y%20)+5," ");
          if (y>0) {
	    y--;
	    if (y%20==19) redraw=1;
	  } else {
	    y=count-1;
	    redraw=1;
	  }
	  print (0,(y%20)+5,">");
	}
	if (joystick.values[ODROID_INPUT_DOWN]) {
	  debounce(ODROID_INPUT_DOWN);
	  print(0,(y%20)+5," ");
	  if (y<count-1) {
	    y++;
	    if (y%20==0) redraw=1;
	  } else {
	    y=0;
	    redraw=1;
	  }
	  print (0,(y%20)+5,">");
	}
	if (joystick.values[ODROID_INPUT_LEFT]) {
	  debounce(ODROID_INPUT_LEFT);
	  print(0,(y%20)+5," ");
	  y-=20;
	  if (y<0) y=0;
	  print(0,(y%20)+5,">");
	  redraw=1;
	}
	if (joystick.values[ODROID_INPUT_RIGHT]) {
	  debounce(ODROID_INPUT_RIGHT);
	  print(0,(y%20)+5," ");
	  y+=20;
	  if (y>count-1) y=count-1;
	  print(0,(y%20)+5,">");
	  redraw=1;
	}
        if (joystick.values[ODROID_INPUT_MENU]) {  // just exit menu...
          debounce(ODROID_INPUT_MENU);
           return(0);
        }
      }
      // OK, either 'A' pressed or screen re-draw needed....
      if (joystick.values[ODROID_INPUT_A] && count==0) // empty directory
            debounce(ODROID_INPUT_A); else 
      if (joystick.values[ODROID_INPUT_A]) {
        debounce(ODROID_INPUT_A);
	dr = opendir(path);
	de = readdir(dr);
	for (count=0; count<y; count++) de = readdir(dr);
	
	if (de->d_type == DT_DIR) { // go into directory...
	  len=strlen(path); path[len]='/';
	  strcat(&path[len+1],de->d_name);
	  y=0; // go to first option
	}
	if (de->d_type == DT_REG) { // file....	
	  file[0]=0; strcat(file,path);
	  strcat(file,"/");
	  strcat(file,de->d_name);
	  load_snapshot_file_type(file,-1);
	  if ((fp=fopen("/sd/roms/spectrum/resume.txt","w"))) {
	    fprintf(fp,"%s\n",path); fprintf(fp,"%s\n",file);
	    fprintf(fp,"%i,%i,%i,%i,%i,%i,%i,%i\n",
	     b_up,b_down,b_left,b_right,b_a,b_b,b_select,b_start);	    
	    fclose(fp);
	  }
	  ili9341_clear(0); // clear screen
	  return(0);	  
        }
      }
      if (joystick.values[ODROID_INPUT_B]) { // up a directory?
        debounce(ODROID_INPUT_B);
	y=0;  // back to first item in list
        if (strlen(path)>3) {  // safe to go up a directory...
          while(path[strlen(path)-1]!='/') path[strlen(path)-1]=0;
 	  path[strlen(path)-1]=0;
        } 
      }
  }
}

bool QuickSaveState(FILE *file)
{
   snsh_save(file, snsh_get_type(file));
   {
        uint8_t buf[32];
        memset(buf, 0, 32);
        buf[0] = b_up&0xff;
        buf[1] = b_down&0xff;
        buf[2] = b_left&0xff;
        buf[3] = b_right&0xff;
        buf[4] = b_a&0xff;
        buf[5] = b_b&0xff;
        buf[6] = b_select&0xff;
        buf[7] = b_start&0xff;
        fwrite(buf,sizeof(uint8_t),32,file);
   }
   return true;
}

bool QuickLoadState(FILE *file)
{
    snsh_load(file, snsh_get_type(file));
    {
        uint8_t buf[32];
        memset(buf, 0, 32);
        if (fread(buf,sizeof(uint8_t),32,file)>=8)
        {
            b_up = buf[0];
            b_down = buf[1];
            b_left = buf[2];
            b_right = buf[3];
            b_a = buf[4];
            b_b = buf[5];
            b_select = buf[6];
            b_start = buf[7];
        }
   }
    return true;
}

//----------------------------------------------------------------
void save()
{
  printx2(12,22,"SAVING");
  save_snapshot_file(file);
  printx2(12,22,"SAVED."); sleep(1);
}

void kb_set() 
{
   if (kbpos==29 || kbpos==30 || kbpos==38) {
    print(4+(kbpos%10)*3,25+(kbpos/10),">");
    print(8+(kbpos%10)*3,25+(kbpos/10),"<");
  } else {
    print(5+(kbpos%10)*3,25+(kbpos/10),">");
    print(7+(kbpos%10)*3,25+(kbpos/10),"<");
  } 
}

//----------------------------------------------------------------
void draw_keyboard()
{
   print(0,24,"                                     ");
   print(0,25,"      1  2  3  4  5  6  7  8  9  0   ");
   print(0,26,"      q  w  e  r  t  y  u  i  o  p   ");
   print(0,27,"      a  s  d  f  g  h  j  k  l ent  ");
   print(0,28,"     cap z  x  c  v  b  n  m sym _   ");
   print(0,29,"                                     ");
  
   kb_set(); // add cursor
}
//---------------------------------------------------------------
void kb_blank()
{
  if (kbpos==29 || kbpos==30 || kbpos==38) {
    print(4+(kbpos%10)*3,25+(kbpos/10)," ");
    print(8+(kbpos%10)*3,25+(kbpos/10)," ");
  } else {
    print(5+(kbpos%10)*3,25+(kbpos/10)," ");
    print(7+(kbpos%10)*3,25+(kbpos/10)," ");
  } 
}

int get_key(int current)
{
  draw_keyboard();
  while(1) {
    odroid_input_gamepad_read(&joystick);
    
    if (joystick.values[ODROID_INPUT_UP]) {
      kb_blank(); kbpos-=10; if (kbpos<0) kbpos+=40;
      kb_set();
      debounce(ODROID_INPUT_UP);   
    }
    if (joystick.values[ODROID_INPUT_DOWN]) {
      kb_blank(); kbpos+=10; if (kbpos>39) kbpos-=40; 
      kb_set();
      debounce(ODROID_INPUT_DOWN);     
    }
    
    if (joystick.values[ODROID_INPUT_LEFT]) {
      kb_blank(); kbpos--; if (kbpos%10==9 || kbpos==-1) kbpos+=10;
      kb_set();
      debounce(ODROID_INPUT_LEFT);   
    }
    if (joystick.values[ODROID_INPUT_RIGHT]) {
      kb_blank(); kbpos++; if (kbpos%10==0) kbpos-=10;
      kb_set();
      debounce(ODROID_INPUT_RIGHT);   
    }
    if (joystick.values[ODROID_INPUT_A]) {     
      debounce(ODROID_INPUT_A);
      return(kbpos);
    }
    if (joystick.values[ODROID_INPUT_B]) {
     debounce(ODROID_INPUT_B);
     return(current);
    }
    if (joystick.values[ODROID_INPUT_MENU]) {
     debounce(ODROID_INPUT_MENU);
     return(current);
    }
  }
 
}
//-----------------------------------------------------------------
// screen for choosing joystick setups like Cursor or Sinclair.
int load_set()
{
  int posn=3;
  
  ili9341_clear(0); // clear screen
  printx2(0,0,"Choose Button Set");
  printx2(4,3,"Cursor/Protek");
  printx2(4,6,"q,a,o,p,m");
  printx2(4,9,"Sinclair");
  printx2(4,12,"Go Back");
  printx2(0,posn,">");
  while(1) {
      odroid_input_gamepad_read(&joystick);
      if (joystick.values[ODROID_INPUT_DOWN]) {
        printx2(0,posn," "); posn+=3; if (posn>12) posn=3;
        printx2(0,posn,">");
        debounce(ODROID_INPUT_DOWN);     
      }
      if (joystick.values[ODROID_INPUT_UP]) {
        printx2(0,posn," "); posn-=3; if (posn<3) posn=12;
        printx2(0,posn,">");
        debounce(ODROID_INPUT_UP);   
      }
      if (joystick.values[ODROID_INPUT_A]) {     
        debounce(ODROID_INPUT_A);
        if (posn==3 ) {b_up=6; b_down=5;b_left=4;b_right=7; b_a=9; return(0);}
        if (posn==6 ) {b_up=10; b_down=20;b_left=18;b_right=19; b_a=37; return(0);}
        if (posn==9 ) {b_up=8; b_down=7;b_left=5;b_right=6; b_a=9; return(0);}	
        if (posn==12) return(0);  
      }
      if (joystick.values[ODROID_INPUT_MENU]) {
       debounce(ODROID_INPUT_MENU);
       return(0);
      }
  }     
}

//-----------------------------------------------------------------
int setup_buttons()
{
  int posn=0,redraw=0;
  
  while(1) {
    ili9341_clear(0); // clear screen
   
    printx2(4, 0,"Up     = "); printx2(22,0,key[b_up]);
    printx2(4, 3,"Down   = "); printx2(22,3,key[b_down]);
    printx2(4, 6,"Left   = "); printx2(22,6,key[b_left]);
    printx2(4, 9,"Right  = "); printx2(22,9,key[b_right]);
    printx2(4,12,"A      = "); printx2(22,12,key[b_a]);
    printx2(4,15,"B      = "); printx2(22,15,key[b_b]);    
    printx2(4,18,"Select = "); printx2(22,18,key[b_select]);
    printx2(4,21,"Start  = "); printx2(22,21,key[b_start]);
    printx2(4,24,"Choose a Set");
    printx2(4,27,"Done.");
    printx2(0,posn,">");
    redraw=0;
    while(!redraw) {
      odroid_input_gamepad_read(&joystick);
      if (joystick.values[ODROID_INPUT_DOWN]) {
        printx2(0,posn," "); posn+=3;  if (posn>27) posn=0;
        printx2(0,posn,">");
        debounce(ODROID_INPUT_DOWN);     
      }
      if (joystick.values[ODROID_INPUT_UP]) {
        printx2(0,posn," "); posn-=3;  if (posn<0) posn=27;
        printx2(0,posn,">");
        debounce(ODROID_INPUT_UP);   
      }
      if (joystick.values[ODROID_INPUT_A]) {     
        debounce(ODROID_INPUT_A);
        if (posn==0 ) {b_up=get_key(b_up); redraw=1;}
        if (posn==3 ) {b_down=get_key(b_down); redraw=1;}
        if (posn==6 ) {b_left=get_key(b_left); redraw=1;}
        if (posn==9 ) {b_right=get_key(b_right); redraw=1;}
	if (posn==12 ) {b_a=get_key(b_a); redraw=1;}
	if (posn==15 ) {b_b=get_key(b_b); redraw=1;}
	if (posn==18) {b_select=get_key(b_select); redraw=1;}
	if (posn==21) {b_start=get_key(b_start); redraw=1;}
	if (posn==24) {load_set(); redraw=1;}
        if (posn==27) return(0);  
      }
      if (joystick.values[ODROID_INPUT_MENU]) {
       debounce(ODROID_INPUT_MENU);
       return(0);
      }
    }    
  }
}

//----------------------------------------------------------------
int menu()
{
  int posn;
  odroid_volume_level level;
  FILE *fp;
  char s[80];

  level=odroid_audio_volume_get();
  odroid_audio_volume_set(0); // turn off sound when in menu...
  process_sound();
  
  my_lastborder=100; // force border re-draw after menu exit....
  
  //debounce inital menu button press first....
  odroid_input_gamepad_read(&joystick);
  debounce(ODROID_INPUT_MENU);
  keyboard=0; // make sure virtual keyboard swtiched off now
  printf("STEP #01\n");
  ili9341_clear(0); // clear screen
  printf("STEP #02\n");
  printx2(0,0,"ZX Spectrum Emulator");
  printf("STEP #03\n");
  printx2(4,6,"Keyboard");
  printx2(4,9,"Load Snapshot");
  printx2(4,12,"Save Snapshot");
  printx2(4,15,"Setup Buttons");
  printx2(4,18,"Toggle Turbo Mode");
  printx2(4,21,"Back To Emulator");
  printf("STEP #08\n");
  
  odroid_input_battery_level_read(&battery_state);
  sprintf(s,"Battery: %i%%",battery_state.percentage);
  printx2(0,28,s);
  posn=6;
  printx2(0,posn,">");
  while(1) {
    odroid_input_gamepad_read(&joystick);
    if (joystick.values[ODROID_INPUT_DOWN]) {
      printx2(0,posn," "); posn+=3; if (posn>21) posn=6;
      printx2(0,posn,">");
      debounce(ODROID_INPUT_DOWN);     
    }
    if (joystick.values[ODROID_INPUT_UP]) {
      printx2(0,posn," "); posn-=3; if (posn<6) posn=21;
      printx2(0,posn,">");
      debounce(ODROID_INPUT_UP);   
    }
    if (joystick.values[ODROID_INPUT_A]) {     
      debounce(ODROID_INPUT_A);
      if (posn==6) {keyboard=1; draw_keyboard();}
      if (posn==9) choose_file(); 
      if (posn==12) save();
      if (posn==15) { // set up buttons, save back new settings
         setup_buttons();
	 if ((fp=fopen("/sd/roms/spectrum/resume.txt","w"))) {
	    fprintf(fp,"%s\n",path); fprintf(fp,"%s\n",file);
	    fprintf(fp,"%i,%i,%i,%i,%i,%i,%i,%i\n",
	     b_up,b_down,b_left,b_right,b_a,b_b,b_select,b_start);	    
	    fclose(fp);
	  }
      }
      if (posn==18) sp_nosync = !sp_nosync;

      //      if (posn==21) {
      //        odroid_system_application_set(0); // set menu slot 
      //	esp_restart(); // reboot!
      //      }
      
      odroid_audio_volume_set(level);  // restore sound...
      ili9341_clear(0);
      return(0);
    }
    if (joystick.values[ODROID_INPUT_MENU]) {
     debounce(ODROID_INPUT_MENU);
     odroid_audio_volume_set(level); // restore sound...
     ili9341_clear(0);
     return(0);
    }
  }
}

extern bool forceConsoleReset;
//----------------------------------------------------------------
void app_main(void)
{
    printf("spectrum (%s-%s).\n", COMPILEDATE, GITREV);

    FILE *fp;
    
    printf("odroid start.\n");
  
    nvs_flash_init();
    odroid_system_init();
    odroid_input_gamepad_init();

    check_boot_cause();
   
    // allocate a screen buffer...
    buffer=(unsigned short *)malloc(16384);
    if (!buffer)
    {
        printf("malloc failed\n");
        abort();
    }
   
    // Display
    ili9341_prepare();
    ili9341_init();
    
    odroid_input_battery_level_init();
    
    odroid_sdcard_open(SD_BASE_PATH);   // map SD card.
    // see if there is a 'resume.txt' file, use it if so...
    
    odroid_settings_RomFilePath_set("/sd/roms/spectrum/Donkey Kong (1986)(Ocean Software).z80");
    char* romPath = odroid_settings_RomFilePath_get();
    if (romPath)
    {
      if(check_ext(romPath, "z80") || check_ext(romPath, "sna"))
      {
        //use_goemu = true;
        strcpy(file, romPath);
      }
      printf("\n-----------\nFound File To Load:%s\n-----------\n",file);
    } else {
      printf("\n-----------\nNo file selected!\n-----------\n");
      if ((fp=fopen("/sd/roms/spectrum/resume.txt","r"))) {
        fgets(path,sizeof path,fp); path[strlen(path)-1]=0;
        fgets(file, sizeof file,fp); file[strlen(file)-1]=0; 
        fscanf(fp,"%i,%i,%i,%i,%i,%i,%i,%i\n",
         &b_up,&b_down,&b_left,&b_right,&b_a,&b_b,&b_select,&b_start);
        fclose(fp); 
        printf("\n-----------\nAuto Select File!\nfile:%s\n-----------\n", file);
      } else {
        printf("\n-----------\nCould Not Auto Select!\n-----------\n");
        abort();  
      }                  
    }    

    printf("\n-----------\nLoading File:%s\n-----------\n",file);

    /*
      Check if Save File Exists
    */
    #ifdef CONFIG_IN_GAME_MENU_YES
        char* save_name = odroid_util_GetFileName(romPath);      
        hud_check_saves(odroid_util_GetFileName(romPath));
    #endif       

    // Audio hardware
    odroid_audio_init(odroid_settings_AudioSink_get(), AUDIO_SAMPLE_RATE);
  
    // Start Spectrum emulation here.......
    sp_init();
    QuickSaveSetBuffer( (void*)heap_caps_malloc(512*1024, MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT) );
    if (use_goemu)
    {
        load_snapshot_file_type(file,-1);
        DoStartupPost();
    }
    else
    {
        load_snapshot_file_type(file,-1);
    }
    odroid_ui_enter_loop();
    start_spectemu();
}
