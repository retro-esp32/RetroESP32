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

#include "spperif.h"
#include "z80.h"

#include "spmain.h"
#include "sptiming.h"
#include "spscr.h"
#include "spkey.h"
#include "sptape.h"
#include "spsound.h"
#include "snapshot.h"
#include "spver.h"

#include "spconf.h"

#include "interf.h"
#include "misc.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "freertos/FreeRTOS.h"
#include "esp_event.h"
#include "../odroid/odroid_sdcard.h"
#include "../odroid/odroid_display.h"
#include "../odroid/odroid_audio.h"
#include "../odroid/odroid_ui.h"

#ifdef CONFIG_IN_GAME_MENU_YES
  #include <stdarg.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include "../odroid/odroid_hud.h"
  int ACTION;
#endif

int endofsingle;

int sp_nosync = 0;

int showframe = 2;
int load_immed = 0;

qbyte sp_int_ctr = 0;


#ifdef USE_DJGPP
#define DOS
#endif

#ifndef DATADIR
#ifdef DOS
#define DATADIR "."
#else
#define DATADIR "/usr/local/share/spectemu"
#endif
#endif

#ifndef MULTIUSER
#ifdef DOS
#define MULTIUSER 0
#else
#define MULTIUSER 1
#endif
#endif

#define GLOBALCFG "spectemu.cfg"
#define LOCALCFG  ".spectemurc"

const char *sp_datadir = DATADIR;

extern unsigned char loadim[];
extern const unsigned long loadim_size;

extern void snsh_sna_load(SNFILE *fp); // bjs

//Use second core for video
QueueHandle_t vidQueue;
TaskHandle_t videoTaskHandle;
volatile bool videoTaskIsRunning = false;

#define SHOW_OFFS 1

static void update(void)
{
  update_screen();
  sp_border_update >>= 1;
  sp_imag_vert = sp_imag_horiz = 0;
}

#define TASK_BREAK (uint16_t*)1

//code from OtherCrashOverride Go-Play
void videoTask(void)
{

  esp_err_t ret;
  videoTaskIsRunning = true;

  uint16_t* param;
  while(1)
  {
    xQueuePeek(vidQueue, &param, portMAX_DELAY);

    if (param == TASK_BREAK)
      break;
    odroid_display_lock();
    translate_screen();
    odroid_display_unlock();

    xQueueReceive(vidQueue, &param, portMAX_DELAY);
  }

  videoTaskIsRunning = false;
  vTaskDelete(NULL);

  while (1) {}
}

void DoMenuHome(bool save)
{
  esp_err_t err;
  uint16_t* param = TASK_BREAK;

  #ifdef CONFIG_IN_GAME_MENU_YES 
    hud_menu();
    printf("\nACTION:%d\n", ACTION);     
    odroid_display_unlock();
    switch(ACTION) {
        case 3:
        case 4:
            hud_progress("Saving...", true);
            hud_deinit();
            DoReboot(true);
            //odroid_display_unlock();
            //ili9341_clear(0);
            //esp_restart();
        break;
        case 5:
            printf("\nDELETE ROM\n");
        break;   
    }             
  #else

    // Clear audio to prevent studdering
    printf("PowerDown: stopping audio.\n");
    odroid_audio_terminate();

    // Stop tasks
    printf("PowerDown: stopping tasks.\n");

    xQueueSend(vidQueue, &param, portMAX_DELAY);
    while (videoTaskIsRunning) {}

    DoReboot(save);
  #endif
}

static void run_singlemode(void)
{
    uint startTime;
    uint stopTime;
    uint totalElapsedTime = 0;
    uint actualFrameCount = 0;
  int t = 0;
  int evenframe, halfsec, updateframe;

  sp_int_ctr = 0;
  endofsingle = 0;

  spti_reset();
  while(!endofsingle) {
    startTime = xthal_get_ccount();
    if(sp_paused) {
      autoclose_sound();
      while(sp_paused) {
	spkb_process_events(1);
	spti_sleep(SKIPTIME);

        //dk Use second core for video
        uint16_t* param = 1;
        xQueueSend(vidQueue,&param,portMAX_DELAY);
        while(videoTaskIsRunning) {}

	update();

      }
      spti_reset();
    }

    halfsec = !(sp_int_ctr % 25);
    evenframe = !(sp_int_ctr & 1);
    if(screen_visible) updateframe = sp_nosync ? halfsec : 
      !((sp_int_ctr+SHOW_OFFS) % showframe);
    else updateframe = 0;

    if(halfsec) {
      sp_flash_state = ~sp_flash_state;
      flash_change();
    }
    if(evenframe) {
      play_tape();
      sp_scline = 0;
    }
    spkb_process_events(evenframe);

    sp_updating = updateframe;

    t += CHKTICK;
    t = sp_halfframe(t, evenframe ? EVENHF : ODDHF);
    if(SPNM(load_trapped)) {
      SPNM(load_trapped) = 0;
      DANM(haltstate) = 0;
      qload();
    }
    z80_interrupt(0xFF);
    sp_int_ctr++;

    if(!evenframe) rec_tape();
    if(!sp_nosync) {    
      if(updateframe) update();
      if(!sound_avail) spti_wait();
      play_sound(evenframe);
    }
    else if(updateframe) update();
    
    stopTime = xthal_get_ccount();
    
        uint elapsedTime;
        if (stopTime > startTime)
          elapsedTime = (stopTime - startTime);
        else
          elapsedTime = ((uint64_t)stopTime + (uint64_t)0xffffffff) - (startTime);

        totalElapsedTime += elapsedTime;
        ++actualFrameCount;

        if (actualFrameCount == 60)
        {
          float seconds = totalElapsedTime / (CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ * 1000000.0f); // 240000000.0f; // (240Mhz)
          float fps = actualFrameCount / seconds;

          //printf("FPS:%f\n", fps);

          actualFrameCount = 0;
          totalElapsedTime = 0;
        }
  }
}

void check_params(int argc, char *argv[])
{
#if MULTIUSER
  char *homedir;

  strncpy(filenamebuf, DATADIR, MAXFILENAME-128);
  strcat(filenamebuf, "/");
  strcat(filenamebuf, GLOBALCFG);

  spcf_read_conf_file(filenamebuf);

  homedir = getenv("HOME");
  if(homedir == NULL) {
    sprintf(msgbuf, 
	    "Warning: Can't open '%s': HOME environment variable not set",
	    LOCALCFG);
    put_msg(msgbuf);
  }
  else {
    strncpy(filenamebuf, homedir, MAXFILENAME-128);
    strcat(filenamebuf, "/");
    strcat(filenamebuf, LOCALCFG);

    if(!file_exist(filenamebuf)) {
      FILE *fp;
      fp = fopen(filenamebuf, "wt");
      if(fp == NULL) {
	sprintf(msgbuf, "Note: Failed to create '%s': %s", filenamebuf, 
		strerror(errno));
	put_msg(msgbuf);
      }
      else {
	fprintf(fp, "# This is the config file for spectemu.\n\n");
	fclose(fp);
	
	sprintf(msgbuf, "Created '%s'", filenamebuf);
	put_msg(msgbuf);
      }
    }

    spcf_read_conf_file(filenamebuf);
  }
#else
  spcf_read_conf_file(GLOBALCFG);
#endif

  spcf_read_xresources();
  spcf_read_command_line(argc, argv);
}

static void init_load()
{
  if(load_immed) snsh_z80_load_intern(loadim, loadim_size);

  if(spcf_init_snapshot != NULL) {
    sprintf(msgbuf, "Loading snapshot '%s'", spcf_init_snapshot);
    put_msg(msgbuf);
    load_snapshot_file_type(spcf_init_snapshot, spcf_init_snapshot_type);
    free_string(spcf_init_snapshot);
  }
  
  if(spcf_init_tapefile != NULL) {
    sprintf(msgbuf, "Loading tape '%s'", spcf_init_tapefile);
    put_msg(msgbuf);
    start_play_file_type(spcf_init_tapefile, 0, spcf_init_tapefile_type);
    if(!load_immed) pause_play();
    free_string(spcf_init_tapefile);
  }
}

static void print_copyright(void)
{
  sprintf(msgbuf, 
	  "Welcome to SPECTEMU version %s/%s (C) Szeredi Miklos 1996-1998", 
	  SPECTEMU_VERSION, SPECTEMU_TYPE);
  put_msg(msgbuf);

  put_msg("This program comes with NO WARRANTY, see the file COPYING "
	  "for details");
  put_msg("Press Ctrl-h for help");
}

//dk

void start_spectemu()
{
  spti_init(); 
  init_spect_scr();
//  init_spect_sound();
  init_spect_key();
  
  print_copyright(); // bjs irrelevant dumping to message buffer
  init_load();

  //dk use second core for video
  vidQueue = xQueueCreate(1, sizeof(uint16_t*));
  xTaskCreatePinnedToCore(&videoTask, "videoTask", 1024*4, NULL, 5, &videoTaskHandle, 1);//these numbers are probably wrong

  run_singlemode();
}
