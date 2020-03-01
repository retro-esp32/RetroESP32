//
// Copyright (c) 2004 K. Wilkins
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from
// the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//

//////////////////////////////////////////////////////////////////////////////
//                       Handy - An Atari Lynx Emulator                     //
//                          Copyright (c) 1996,1997                         //
//                                 K. Wilkins                               //
//////////////////////////////////////////////////////////////////////////////
// System object header file                                                //
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// This header file provides the interface definition and inline code for   //
// the system object, this object if what binds together all of the Handy   //
// hardware enmulation objects, its the glue that holds the system together //
//                                                                          //
//    K. Wilkins                                                            //
// August 1997                                                              //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
// Revision History:                                                        //
// -----------------                                                        //
//                                                                          //
// 01Aug1997 KW Document header added & class documented.                   //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#ifndef SYSTEM_H
#define SYSTEM_H
/*
#pragma inline_depth (255)
#pragma inline_recursion (on)
*/
#ifdef _LYNXDBG

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
//#endif

#endif

#include "machine.h"
#include "myadd.h"

#define HANDY_SYSTEM_FREQ						16000000
#define HANDY_TIMER_FREQ						20
#define HANDY_AUDIO_SAMPLE_FREQ					22050
#define HANDY_AUDIO_SAMPLE_PERIOD				(HANDY_SYSTEM_FREQ/HANDY_AUDIO_SAMPLE_FREQ)
#define HANDY_AUDIO_WAVESHAPER_TABLE_LENGTH		0x200000

#ifdef SDL_PATCH
//#define HANDY_AUDIO_BUFFER_SIZE					4096	// Needed for SDL 8bit MONO
//#define HANDY_AUDIO_BUFFER_SIZE					8192	// Needed for SDL STEREO 8bit
#define HANDY_AUDIO_BUFFER_SIZE					16384	// Needed for SDL STEREO 16bit
#else
//#define HANDY_AUDIO_BUFFER_SIZE					(HANDY_AUDIO_SAMPLE_FREQ/4)
#define HANDY_AUDIO_BUFFER_SIZE					(HANDY_AUDIO_SAMPLE_FREQ)
#endif

#define HANDY_AUDIO_BUFFER_SIZE                 (2756)


#define HANDY_FILETYPE_LNX		0
#define HANDY_FILETYPE_HOMEBREW	1
#define HANDY_FILETYPE_SNAPSHOT	2
#define HANDY_FILETYPE_ILLEGAL	3
#define HANDY_FILETYPE_RAW	4

#define HANDY_SCREEN_WIDTH	160
#define HANDY_SCREEN_HEIGHT	102
//
// Define the global variable list
//

#ifdef SYSTEM_CPP
ULONG	gSystemCycleCount=0;
ULONG	gNextTimerEvent=0;
ULONG	gCPUWakeupTime=0;
ULONG	gIRQEntryCycle=0;
ULONG	gCPUBootAddress=0;
ULONG	gBreakpointHit=FALSE;
ULONG	gSingleStepMode=FALSE;
ULONG	gSingleStepModeSprites=FALSE;
ULONG	gSystemIRQ=FALSE;
ULONG	gSystemNMI=FALSE;
ULONG	gSystemCPUSleep=FALSE;
ULONG	gSystemCPUSleep_Saved=FALSE;
ULONG	gSystemHalt=FALSE;
ULONG	gThrottleMaxPercentage=100;
ULONG	gThrottleLastTimerCount=0;
ULONG	gThrottleNextCycleCheckpoint=0;

volatile ULONG gTimerCount=0;

ULONG	gAudioEnabled=FALSE;
#ifdef MY_AUDIO_MODE_V1
short   *gAudioBuffer;
ULONG   gAudioBufferPointer=0;
short   *gAudioBufferPointer2 = NULL;
#else
UBYTE   gAudioBuffer[HANDY_AUDIO_BUFFER_SIZE];
ULONG   gAudioBufferPointer=0;
#endif
ULONG	gAudioLastUpdateCycle=0;

CErrorInterface *gError=NULL;
#else

extern ULONG	gSystemCycleCount;
extern ULONG	gNextTimerEvent;
extern ULONG	gCPUWakeupTime;
extern ULONG	gIRQEntryCycle;
extern ULONG	gCPUBootAddress;
extern ULONG	gBreakpointHit;
extern ULONG	gSingleStepMode;
extern ULONG	gSingleStepModeSprites;
extern ULONG	gSystemIRQ;
extern ULONG	gSystemNMI;
extern ULONG	gSystemCPUSleep;
extern ULONG	gSystemCPUSleep_Saved;
extern ULONG	gSystemHalt;
extern ULONG	gThrottleMaxPercentage;
extern ULONG	gThrottleLastTimerCount;
extern ULONG	gThrottleNextCycleCheckpoint;

extern volatile ULONG gTimerCount;

extern ULONG	gAudioEnabled;
#ifdef MY_AUDIO_MODE_V1
extern short    *gAudioBuffer;
extern ULONG    gAudioBufferPointer;
extern short   *gAudioBufferPointer2;
#else
extern UBYTE 	gAudioBuffer[HANDY_AUDIO_BUFFER_SIZE];
extern ULONG	gAudioBufferPointer;
#endif
extern ULONG	gAudioLastUpdateCycle;

extern CErrorInterface *gError;
#endif

typedef struct lssfile
{
   UBYTE *memptr;
   ULONG index;
   ULONG index_limit;
} LSS_FILE;

int lss_read(void* dest,int varsize, int varcount,LSS_FILE *fp);

//
// Define the interfaces before we start pulling in the classes
// as many classes look for articles from the interfaces to
// allow compilation

#include "sysbase.h"

class CSystem;

//
// Now pull in the parts that build the system
//
#include "lynxbase.h"
//#include "memfault.h"
#include "ram.h"
#include "rom.h"
#include "memmap.h"
#include "cart.h"
#include "eeprom.h"
#include "susie.h"
#include "mikie.h"
#include "c65c02.h"

#include <stdint.h>

#define TOP_START	0xfc00
#define TOP_MASK	0x03ff
#define TOP_SIZE	0x400
#define SYSTEM_SIZE	65536

#define LSS_VERSION_OLD	"LSS2"
#define LSS_VERSION	"LSS3"

class CSystem : public CSystemBase
{
   public:
      CSystem(const char* gamefile, const char* romfile,bool useEmu);
      CSystem(bool aa);
      ~CSystem();
    void SaveEEPROM(void);

   public:
      void HLE_BIOS_FE00(void);
      void HLE_BIOS_FE19(void);
      void HLE_BIOS_FE4A(void);
      void HLE_BIOS_FF80(void);
      void	Reset(void);
      size_t	MemoryContextSave(const char* tmpfilename, char *context);
      bool	MemoryContextLoad(const char *context, size_t size);
      bool	ContextSave(const char *context);
      bool	ContextLoad(const char *context);
      bool  ContextSave(FILE *f);
      bool  ContextLoad(FILE *f);
      bool	IsZip(char *filename);

#ifndef MY_SYSTEM_LOOP
      inline void Update(void)
      {
         //		    fprintf(stderr, "sys update\n");
         //
         // Only update if there is a predicted timer event
         //
         if(gSystemCycleCount>=gNextTimerEvent)
         {
            mMikie->Update();
         }
         //
         // Step the processor through 1 instruction
         //
        mCpu->Update();
         //         fprintf(stderr, "end cpu update\n");

#ifdef _LYNXDBG
         // Check breakpoint
         static ULONG lastcycle=0;
         if(lastcycle<mCycleCountBreakpoint && gSystemCycleCount>=mCycleCountBreakpoint) gBreakpointHit=TRUE;
         lastcycle=gSystemCycleCount;

         // Check single step mode
         if(gSingleStepMode) gBreakpointHit=TRUE;
#endif

         //
         // If the CPU is asleep then skip to the next timer event
         //
         if(gSystemCPUSleep)
         {
            gSystemCycleCount=gNextTimerEvent;
         }

         //			fprintf(stderr, "end sys update\n"); 
      }
#else
#if MY_SYSTEM_LOOP==1
      inline void Update(void)
      {
        if(gSystemCycleCount>=gNextTimerEvent)
         {
            mMikie->Update();
         }
         while (gSystemCycleCount<gNextTimerEvent) {
            mCpu->Update();
            if(gSystemCPUSleep)
             {
                gSystemCycleCount=gNextTimerEvent;
                break;
             }
         }
      }
#endif
#if MY_SYSTEM_LOOP==2
      inline void Update(void)
      {
        if(gSystemCycleCount>=gNextTimerEvent)
         {
            mMikie->Update();
         }
         while (gSystemCycleCount<gNextTimerEvent) {
            mCpu->Update();
            mCpu->Update();
            mCpu->Update();
            if(gSystemCPUSleep)
             {
                gSystemCycleCount=gNextTimerEvent;
                break;
             }
         }
      }
#endif

#endif
      //
      // We MUST have separate CPU & RAM peek & poke handlers as all CPU accesses must
      // go thru the address generator at $FFF9
      //
      // BUT, Mikie video refresh & Susie see the whole system as RAM
      //
      // Complete and utter wankers, its taken me 1 week to find the 2 lines
      // in all the documentation that mention this fact, the mother of all
      // bugs has been found and FIXED.......

      //
      // CPU
      //
#ifdef MY_MEM_MODE_V2
        // mState
        // 0x01     SUSIE_START+SUSIE_SIZE    mSusieEnabled  0xfc00
        // 0x02     MIKIE_START+MIKIE_SIZE    mMikieEnabled  0xfd00
        // 0x04     BROM_START+(BROM_SIZE-8)  mRomEnabled    0xfe00 0x0200 - 8
        // 0x08     VECTOR_START,VECTOR_SIZE  mVectorsEnabled 0xfffa 0x6     
        // mSystem.mMemoryHandlers[0xFFF8 - MY_MEM_START]=mSystem.mRam;
        // mSystem.mMemoryHandlers[0xFFF9 - MY_MEM_START]=mSystem.mMemMap;

        inline CLynxBase *Get_Handler(ULONG addr)
        {
            if (addr < 0xfc00) return mRam;
            if (addr == 0xFFF9) return mMemMap;
            // if (mMemMap->mState == 0 || addr == 0xFFF8) return mRam;
            if (addr == 0xFFF8) return mRam;
            if ((addr&0xff00)==0xfc00) {
                return mMemMap->mSusieEnabled?(CLynxBase*)mSusie:(CLynxBase*)mRam;
            } else if ((addr&0xff00)==0xfd00) {
                return mMemMap->mMikieEnabled?(CLynxBase*)mMikie:(CLynxBase*)mRam;
            } else if (addr>=0xfffa) {
                return mMemMap->mVectorsEnabled?(CLynxBase*)mRom:(CLynxBase*)mRam;
            }
            return mMemMap->mRomEnabled?(CLynxBase*)mRom:(CLynxBase*)mRam;
        }
        inline void  Poke_CPU(ULONG addr, UBYTE data)
      {
        CLynxBase *p = Get_Handler(addr);
        p->Poke(addr,data);
      };
      inline UBYTE Peek_CPU(ULONG addr)
      {
        CLynxBase *p = Get_Handler(addr);
        return p->Peek(addr);
      };
      inline void  PokeW_CPU(ULONG addr,UWORD data)
      {
        CLynxBase *p = Get_Handler(addr);
        p->Poke(addr,data&0xff);
        addr++;
        p = Get_Handler(addr);
        p->Poke(addr,data>>8);
      };
      inline UWORD PeekW_CPU(ULONG addr)
      {
        //CLynxBase *p = Get_Handler(addr);
        //return ((p->Peek(addr))+(p->Peek(addr+1)<<8));
        return ((Get_Handler(addr)->Peek(addr))+(Get_Handler(addr+1)->Peek(addr+1)<<8));
      };

      /*
      inline CLynxBase *Get_Handler(ULONG addr)
        {
            if (addr < 0xfc00) return mRam;
            if (addr == 0xFFF9) return mMemMap;
            // if (mMemMap->mState == 0 || addr == 0xFFF8) return mRam;
            if (addr == 0xFFF8) return mRam;
            if ((addr&0xff00)==0xfc00) {
                return mMemMap->mSusieEnabled?(CLynxBase*)mSusie:(CLynxBase*)mRam;
            } else if ((addr&0xff00)==0xfd00) {
                return mMemMap->mMikieEnabled?(CLynxBase*)mMikie:(CLynxBase*)mRam;
            } else if (addr>=0xfffa) {
                return mMemMap->mVectorsEnabled?(CLynxBase*)mRom:(CLynxBase*)mRam;
            }
            return mMemMap->mRomEnabled?(CLynxBase*)mRom:(CLynxBase*)mRam;
        }
      
      #define MY_POKE(addr, data) \
      {\
            if (addr < 0xfc00) mRam->Poke(addr, data);\
            else\
            if (addr == 0xFFF9) mMemMap->Poke(addr, data);\
            else\
            if (addr == 0xFFF8) mRam->Poke(addr, data);\
            else\
            if ((addr&0xff00)==0xfc00) {\
                if (mMemMap->mSusieEnabled) mSusie->Poke(addr, data);\
                else mRam->Poke(addr, data);\
            } else if ((addr&0xff00)==0xfd00) {\
                if (mMemMap->mMikieEnabled) mMikie->Poke(addr, data);\
                else mRam->Poke(addr, data);\
            } else if (addr>=0xfffa) {\
                if (mMemMap->mVectorsEnabled) mRom->Poke(addr, data);\
                else mRam->Poke(addr, data);\
            }\
            if (mMemMap->mRomEnabled) mRom->Poke(addr, data);\
            else mRam->Poke(addr, data);\
      }
      
        inline void  Poke_CPU(ULONG addr, UBYTE data)
      {
        MY_POKE(addr, data);
      };
      inline UBYTE Peek_CPU(ULONG addr)
      {
        CLynxBase *p = Get_Handler(addr);
        return p->Peek(addr);
      };
      inline void  PokeW_CPU(ULONG addr,UWORD data)
      {
        MY_POKE(addr, (data&0xff));
        addr++;
        MY_POKE(addr, (data>>8));
      };
      inline UWORD PeekW_CPU(ULONG addr)
      {
        return ((Get_Handler(addr)->Peek(addr))+(Get_Handler(addr+1)->Peek(addr+1)<<8));
      };
      */
#else
#ifdef MY_MEM_MODE
      inline void  Poke_CPU(ULONG addr, UBYTE data)
      {
        if (addr < MY_MEM_START) mRam->Poke(addr,data);
        else mMemoryHandlers[addr-MY_MEM_START]->Poke(addr,data);
      };
      inline UBYTE Peek_CPU(ULONG addr)
      {
        if (addr < MY_MEM_START) return mRam->Peek(addr);
        else return mMemoryHandlers[addr-MY_MEM_START]->Peek(addr);
      };
      inline void  PokeW_CPU(ULONG addr,UWORD data)
      {
        if (addr < MY_MEM_START)
        {
        mRam->Poke(addr,data&0xff);
        addr++;
        mRam->Poke(addr,data>>8);
        } else
        {
        mMemoryHandlers[addr-MY_MEM_START]->Poke(addr,data&0xff);
        addr++;
        mMemoryHandlers[addr-MY_MEM_START]->Poke(addr,data>>8);
        }
      };
      inline UWORD PeekW_CPU(ULONG addr)
      {
        if (addr < MY_MEM_START) return ((mRam->Peek(addr))+(mRam->Peek(addr+1)<<8));
        else return ((mMemoryHandlers[addr-MY_MEM_START]->Peek(addr))+(mMemoryHandlers[addr-MY_MEM_START]->Peek(addr+1)<<8));
      };
#else
      inline void  Poke_CPU(ULONG addr, UBYTE data) { mMemoryHandlers[addr]->Poke(addr,data);};
      inline UBYTE Peek_CPU(ULONG addr) { return mMemoryHandlers[addr]->Peek(addr);};
      inline void  PokeW_CPU(ULONG addr,UWORD data) { mMemoryHandlers[addr]->Poke(addr,data&0xff);addr++;mMemoryHandlers[addr]->Poke(addr,data>>8);};
      inline UWORD PeekW_CPU(ULONG addr) {return ((mMemoryHandlers[addr]->Peek(addr))+(mMemoryHandlers[addr]->Peek(addr+1)<<8));};
#endif
#endif
      //
      // RAM
      //
      inline void  Poke_RAM(ULONG addr, UBYTE data) { mRam->Poke(addr,data);};
      inline UBYTE Peek_RAM(ULONG addr) { return mRam->Peek(addr);};
      inline void  PokeW_RAM(ULONG addr,UWORD data) { mRam->Poke(addr,data&0xff);addr++;mRam->Poke(addr,data>>8);};
      inline UWORD PeekW_RAM(ULONG addr) {return ((mRam->Peek(addr))+(mRam->Peek(addr+1)<<8));};

      // High level cart access for debug etc

      inline void  Poke_CART(ULONG addr, UBYTE data) {mCart->Poke(addr,data);};
      inline UBYTE Peek_CART(ULONG addr) {return mCart->Peek(addr);};
      inline void  CartBank(EMMODE bank) {mCart->BankSelect(bank);};
      inline ULONG CartSize(void) {return mCart->ObjectSize();};
      inline const char* CartGetName(void) { return mCart->CartGetName();};
      inline const char* CartGetManufacturer(void) { return mCart->CartGetManufacturer();};
      inline ULONG CartGetRotate(void) {return mCart->CartGetRotate();};

      // Low level cart access for Suzy, Mikey

      inline void  Poke_CARTB0(UBYTE data) {mCart->Poke0(data);};
      inline void  Poke_CARTB1(UBYTE data) {mCart->Poke1(data);};
      inline void  Poke_CARTB0A(UBYTE data) {mCart->Poke0A(data);};
      inline void  Poke_CARTB1A(UBYTE data) {mCart->Poke1A(data);};
      inline UBYTE Peek_CARTB0(void) {return mCart->Peek0();}
      inline UBYTE Peek_CARTB1(void) {return mCart->Peek1();}
      inline UBYTE Peek_CARTB0A(void) {return mCart->Peek0A();}
      inline UBYTE Peek_CARTB1A(void) {return mCart->Peek1A();}
      inline void  CartAddressStrobe(bool strobe) {mCart->CartAddressStrobe(strobe);};
      inline void  CartAddressData(bool data) {mCart->CartAddressData(data);};

      // Low level CPU access

      void	SetRegs(C6502_REGS &regs) {mCpu->SetRegs(regs);};
      void	GetRegs(C6502_REGS &regs) {mCpu->GetRegs(regs);};

      // Mikey system interfacing

      void	DisplaySetAttributes(ULONG Rotate,ULONG Format,ULONG Pitch,UBYTE* (*DisplayCallback)(ULONG objref),ULONG objref) { mMikie->DisplaySetAttributes(Rotate,Format,Pitch,DisplayCallback,objref); };

      void	ComLynxCable(int status) { mMikie->ComLynxCable(status); };
      void	ComLynxRxData(int data)  { mMikie->ComLynxRxData(data); };
      void	ComLynxTxCallback(void (*function)(int data,ULONG objref),ULONG objref) { mMikie->ComLynxTxCallback(function,objref); };

      // Suzy system interfacing

      inline ULONG	PaintSprites(void) {return mSusie->PaintSprites();};

      // Miscellaneous

      inline void	SetButtonData(ULONG data) {mSusie->SetButtonData(data);};
      inline ULONG	GetButtonData(void) {return mSusie->GetButtonData();};
      inline void	SetCycleBreakpoint(ULONG breakpoint) {mCycleCountBreakpoint=breakpoint;};
      inline UBYTE*	GetRamPointer(void) {return mRam->GetRamPointer();};
#ifdef _LYNXDBG
      void	DebugTrace(int address);

      void	DebugSetCallback(void (*function)(ULONG objref, char *message),ULONG objref);

      void	(*mpDebugCallback)(ULONG objref, char *message);
      ULONG	mDebugCallbackObject;
#endif

   public:
      ULONG			mCycleCountBreakpoint;
#ifndef MY_MEM_MODE_V2
#ifdef MY_MEM_MODE
    CLynxBase       *mMemoryHandlers[SYSTEM_SIZE - MY_MEM_START];
#else
      CLynxBase		*mMemoryHandlers[SYSTEM_SIZE];
      // CLynxBase		**mMemoryHandlers;
#endif
#endif
      CCart			*mCart;
      CRom			*mRom;
      CMemMap			*mMemMap;
      CRam			*mRam;
      C65C02			*mCpu;
      CMikie			*mMikie;
      CSusie			*mSusie;
      CEEPROM			*mEEPROM;

      ULONG			mFileType;
};

#endif
