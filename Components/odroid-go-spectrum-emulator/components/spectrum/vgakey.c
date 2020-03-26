/* 
 * Copyright (C) 1996-1998 Szeredi Miklos
 * Email: mszeredi@inf.bme.hu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See the file COPYING. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include "config.h"

#include "spkey.h"
#include "spkey_p.h"
#include "spperif.h"
#include "vgascr.h"
#include "akey.h"

#include <stdlib.h>
#include "../odroid/odroid_input.h"
#include "../odroid/odroid_audio.h"
#include "../odroid/odroid_display.h"
#include "../odroid/odroid_ui.h"

extern void keyboard_init();
extern void keyboard_close();
extern void keyboard_translatekeys();
extern int keyboard;

extern int menu();
extern void kb_blank();
extern void kb_set();

extern int b_up,b_down,b_left,b_right,b_a,b_b,b_start,b_select;
int pressed=0;  // used for de-bouncing buttons
int kbpos=0; // virtual keyboard position

const int need_switch_mode = 1;
volatile int spvk_after_switch = 0;


const int map[40]={
2,3,4,5,6,7,8,9,10,11,
16,17,18,19,20,21,22,23,24,25,
30,31,32,33,34,35,36,37,38,28,
42,44,45,46,47,48,49,50,54,57
};


#define LASTKEYCODE 111

#define KC_F1 59

#define KC_L_SHIFT 42
#define KC_R_SHIFT 54

#define KC_L_CTRL 29
#define KC_R_CTRL 97

#define KC_L_ALT 56
#define KC_R_ALT 100

static char kbstate[LASTKEYCODE];


extern void DoMenuHome(bool save);
extern void draw_keyboard();
extern void setup_buttons();
extern bool use_goemu;
extern int my_lastborder;
extern int sp_nosync;
bool menu_restart = false;
int menuButtonFrameCount = -10;

void menu_keyboard_update(odroid_ui_entry *entry) {
    sprintf(entry->text, "%-9s: %s", "keyboard", keyboard?"on":"off");
}

odroid_ui_func_toggle_rc menu_keyboard_toggle(odroid_ui_entry *entry, odroid_gamepad_state *joystick) {
    keyboard=!keyboard;
    return ODROID_UI_FUNC_TOGGLE_RC_MENU_CLOSE;
}

void menu_keyboard_configure_update(odroid_ui_entry *entry) {
    sprintf(entry->text, "%-9s", "keyboard mapping");
}

odroid_ui_func_toggle_rc menu_keyboard_configure_toggle(odroid_ui_entry *entry, odroid_gamepad_state *joystick) {
    if (!joystick->values[ODROID_INPUT_A]) return ODROID_UI_FUNC_TOGGLE_RC_NOTHING;
    odroid_display_unlock();
    odroid_ui_wait_for_key(ODROID_INPUT_A, false);
    setup_buttons();
    odroid_display_lock();
    return ODROID_UI_FUNC_TOGGLE_RC_MENU_CLOSE;
}

void menu_spectrum_init(odroid_ui_window *window) {
    odroid_ui_create_entry(window, &menu_keyboard_update, &menu_keyboard_toggle);
    odroid_ui_create_entry(window, &menu_keyboard_configure_update, &menu_keyboard_configure_toggle);
}

void keyboard_update()
{
  odroid_gamepad_state joystick;
  int i;
  
  for (i=0; i<LASTKEYCODE; i++) kbstate[i]=0;  
  odroid_input_gamepad_read(&joystick); 
  // first, process volume button...
  
  //djk
//---------------------------------------
  if (!pressed && joystick.values[ODROID_INPUT_VOLUME])
  {
    pressed=1;
    int keyboard_old = keyboard;
    bool config_speedup_old = config_speedup;
    menu_restart = odroid_ui_menu_ext(menu_restart, &menu_spectrum_init);
    my_lastborder=100;
    if (config_speedup_old != config_speedup) sp_nosync = config_speedup;
    if (keyboard != keyboard_old) draw_keyboard();
  }

//---------------------------------------   
#ifdef CONFIG_IN_GAME_MENU_YES    
  if(joystick.values[ODROID_INPUT_MENU] && menuButtonFrameCount!=0) {
    menuButtonFrameCount = 0;
    printf("\n\n-----------\nYou Pressed MENU\n-----------\n");
    DoMenuHome(false);
  }
  if(!joystick.values[ODROID_INPUT_MENU]) {
    menuButtonFrameCount = 1;                                             
  }
#else
  // 2 different approaches depending if virtual keyboard is actice...  
  if (!keyboard) {      
    if (joystick.values[ODROID_INPUT_UP])     kbstate[map[b_up]]=1;
    if (joystick.values[ODROID_INPUT_DOWN])   kbstate[map[b_down]]=1; 
    if (joystick.values[ODROID_INPUT_LEFT])   kbstate[map[b_left]]=1;
    if (joystick.values[ODROID_INPUT_RIGHT])  kbstate[map[b_right]]=1; 
    if (joystick.values[ODROID_INPUT_A])      kbstate[map[b_a]]=1; 
    if (joystick.values[ODROID_INPUT_B])      kbstate[map[b_b]]=1; 
    if (joystick.values[ODROID_INPUT_SELECT]) kbstate[map[b_select]]=1; 
    if (joystick.values[ODROID_INPUT_START])  kbstate[map[b_start]]=1;
    if (!use_goemu)
    {  
        if (joystick.values[ODROID_INPUT_MENU]) menu();
    }
    else if (!joystick.values[ODROID_INPUT_MENU] && menuButtonFrameCount!=0)
    {
        if (menuButtonFrameCount>1)
        {
            DoMenuHome(false);
        }
        menuButtonFrameCount = 0;
    } else if (joystick.values[ODROID_INPUT_MENU])
    {
        menuButtonFrameCount++;
        if (menuButtonFrameCount > (60 * 1) / 2)
        {
            DoMenuHome(true);
        }
    }
  }
#endif  
  else {
    if (!pressed && joystick.values[ODROID_INPUT_UP]) {     
      kb_blank(); kbpos-=10; if (kbpos<0) kbpos+=40;
      kb_set(); pressed=1;
    }
    if (!pressed && joystick.values[ODROID_INPUT_DOWN]) {     
      kb_blank(); kbpos+=10; if (kbpos>39) kbpos-=40;
      kb_set(); pressed=1;
    }
    if (!pressed && joystick.values[ODROID_INPUT_LEFT]) {     
      kb_blank(); kbpos--; if (kbpos%10==9 || kbpos==-1) kbpos+=10;
      kb_set(); pressed=1;
    }
    if (!pressed && joystick.values[ODROID_INPUT_RIGHT]) {     
      kb_blank(); kbpos++; if (kbpos%10==0) kbpos-=10;
      kb_set(); pressed=1;
    }
    
    if (joystick.values[ODROID_INPUT_A]) kbstate[map[kbpos]]=1;
    if (joystick.values[ODROID_INPUT_SELECT]) kbstate[map[30]]=1; // CAPS-shift
    if (joystick.values[ODROID_INPUT_START]) kbstate[map[38]]=1; // sym-shift
    
    if (joystick.values[ODROID_INPUT_B] ||
        joystick.values[ODROID_INPUT_MENU]) {keyboard=0; my_lastborder=100;}
    
        
  }
  if (pressed &&
       !joystick.values[ODROID_INPUT_UP] &&
       !joystick.values[ODROID_INPUT_DOWN] &&
       !joystick.values[ODROID_INPUT_LEFT] &&
       !joystick.values[ODROID_INPUT_RIGHT] &&
       !joystick.values[ODROID_INPUT_VOLUME]
       ) pressed=0;
  
}


static void spkb_init(void)
{
  keyboard_init();
  keyboard_translatekeys();
  // kbstate = keyboard_getstate(); // bjs not needed now
}

#define spkb_close()   keyboard_close()
#define spkb_raw()     keyboard_init()
#define spkb_normal()  keyboard_close()
#define spkb_update()  keyboard_update()

#define kbst(i) kbstate[i]

#define SDEMPTY {0, 0}
#define SDSPEC(x) {x, x}

struct spscan_keydef {
  unsigned norm;
  unsigned shifted;
};

static struct spscan_keydef sp_cp_keycode[] = {
  SDEMPTY,
  SDSPEC(SK_Escape),
  {'1', '!'},
  {'2', '@'},
  {'3', '#'},
  {'4', '$'},
  {'5', '%'},
  {'6', '^'},
  {'7', '&'},
  {'8', '*'},
  {'9', '('},
  {'0', ')'},
  {'-', '_'},
  {'=', '+'},
  SDSPEC(SK_BackSpace),
   
  SDSPEC(SK_Tab),
  {'q', 'Q'},
  {'w', 'W'},
  {'e', 'E'},
  {'r', 'R'},
  {'t', 'T'},
  {'y', 'Y'},
  {'u', 'U'},
  {'i', 'I'},
  {'o', 'O'},
  {'p', 'P'},
  {'[', '{'},
  {']', '}'},
  SDSPEC(SK_Return),
   
  SDSPEC(SK_Control_L),
  {'a', 'A'},
  {'s', 'S'},
  {'d', 'D'},
  {'f', 'F'},
  {'g', 'G'},
  {'h', 'H'},
  {'j', 'J'},
  {'k', 'K'},
  {'l', 'L'},
  {';', ':'},
  {'\'', '"'}, 
  {'`', '~'},
   
  SDSPEC(SK_Shift_L),
  {'\\', '|'},
  {'z', 'Z'},
  {'x', 'X'},
  {'c', 'C'},
  {'v', 'V'},
  {'b', 'B'},
  {'n', 'N'},
  {'m', 'M'},
  {',', '<'},
  {'.', '>'},
  {'/', '?'},
  SDSPEC(SK_Shift_R),
  SDSPEC(SK_KP_Multiply),
  SDSPEC(SK_Alt_L),
   
  {' ', ' '},
  SDSPEC(SK_Caps_Lock),
  SDSPEC(SK_F1),
  SDSPEC(SK_F2),
  SDSPEC(SK_F3),
  SDSPEC(SK_F4),
  SDSPEC(SK_F5),
  SDSPEC(SK_F6),
  SDSPEC(SK_F7),
  SDSPEC(SK_F8),
  SDSPEC(SK_F9),
  SDSPEC(SK_F10),

  SDSPEC(SK_Num_Lock),
  SDSPEC(SK_Scroll_Lock),
  SDSPEC(SK_KP_Home),
  SDSPEC(SK_KP_Up),
  SDSPEC(SK_KP_Page_Up),
  SDSPEC(SK_KP_Subtract),
  SDSPEC(SK_KP_Left),
  SDSPEC(SK_KP_5),
  SDSPEC(SK_KP_Right),
  SDSPEC(SK_KP_Add),
  SDSPEC(SK_KP_End),
  SDSPEC(SK_KP_Down),
  SDSPEC(SK_KP_Page_Down),
  SDSPEC(SK_KP_Insert),
  SDSPEC(SK_KP_Delete),

  SDEMPTY,
  SDEMPTY,
  {'<', '>'},
  SDSPEC(SK_F11),
  SDSPEC(SK_F12),
  SDEMPTY,
  SDEMPTY,
  SDEMPTY,
  SDEMPTY,
  SDEMPTY,
  SDEMPTY,
  SDEMPTY,
  SDSPEC(SK_KP_Enter),
  SDSPEC(SK_Control_R),
  SDSPEC(SK_KP_Divide),
  SDSPEC(SK_Print),
  SDSPEC(SK_Alt_R),
  SDEMPTY,
  SDSPEC(SK_Home),
  SDSPEC(SK_Up),
  SDSPEC(SK_Page_Up),
  SDSPEC(SK_Left),
  SDSPEC(SK_Right),
  SDSPEC(SK_End),
  SDSPEC(SK_Down),
  SDSPEC(SK_Page_Down),
  SDSPEC(SK_Insert),
  SDSPEC(SK_Delete),
};

spkeyboard kb_mkey;

void spkey_textmode(void)
{
  spkb_normal();
  restore_sptextmode();
}

void spkey_screenmode(void)
{
  set_vga_spmode();
  spkb_raw();
  
  sp_init_screen_mark();
}

//------------------------------------------------
void spkb_process_events(int evenframe)
{
  int i;
  int changed;
  
  if(evenframe) {
    spkb_update();
    changed = 0;
    for(i = 0; i <= LASTKEYCODE; i++) {
      int ki;
      int sh;
      int ks;
      
      ks = sp_cp_keycode[i].norm;
      ki = KS_TO_KEY(ks);
    
      if( kbst(i) && !spkb_kbstate[ki].press  ) { // press
	changed = 1;
	spkb_kbstate[ki].press = 1;
	
	sh = sp_cp_keycode[i].shifted;

	spkb_last.modif = 0;

	if(kbst(KC_L_SHIFT) || kbst(KC_R_SHIFT)) 
	  spkb_last.modif |= SKShiftMask;

	if(kbst(KC_L_CTRL) || kbst(KC_R_CTRL)) 
	  spkb_last.modif |= SKControlMask;

	if(kbst(KC_L_ALT) || kbst(KC_R_ALT)) 
	  spkb_last.modif |= SKMod1Mask;
	
	spkb_last.index = ki;
	spkb_last.shifted = sh;
	
	spkb_last.keysym = (spkb_last.modif &  SKShiftMask) ? sh : ks;
	if((!ISFKEY(ks) || !spvk_after_switch) && accept_keys &&
	   !spkey_keyfuncs()) {
	  spkb_kbstate[ki].state = 1;
	  spkb_kbstate[i].frame = sp_int_ctr;
	}
	else spkb_kbstate[ki].state = 0;
	
	spvk_after_switch = 0;
      }
      
      if( !kbst(i) && spkb_kbstate[ki].press ) {  // release
	changed = 1;
	spkb_kbstate[ki].press = 0;
	spkb_kbstate[ki].state = 0;
      }
      
    }
    if(changed) spkb_state_changed = 1;
  }
  process_keys();
}

//---------------------------------------------------

void init_spect_key(void)
{
  int i;
 
  for(i = 0; i < NR_SPKEYS; i++) spkb_kbstate[i].press = 0;
  clear_keystates();
  init_basekeys();
  spkb_init();
  // atexit(close_spect_key); // bjs unnecessary!
}

int display_keyboard(void)
{
  return 0;
}
