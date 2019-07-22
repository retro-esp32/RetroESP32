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
// Suzy emulation class                                                     //
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// This class emulates the Suzy chip within the lynx. This provides math    //
// and sprite painting facilities. SpritePaint() is called from within      //
// the Mikey POKE functions when SPRGO is set and is called via the system  //
// object to keep the interface clean.                                      //
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

#define SUSIE_CPP

//#include <crtdbg.h>
//#define TRACE_SUSIE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "system.h"
#include "susie.h"
#include "lynxdef.h"

//
// As the Susie sprite engine only ever sees system RAM
// wa can access this directly without the hassle of
// going through the system object, much faster
//
//#define RAM_PEEK(m)			(mSystem.Peek_RAM((m)))
//#define RAM_POKE(m1,m2)		(mSystem.Poke_RAM((m1),(m2)))
//#define RAM_PEEKW(m)			(mSystem.PeekW_RAM((m)))

#define RAM_PEEK(m)				(mRamPointer[(m)])
#define RAM_PEEKW(m)			(mRamPointer[(m)]+(mRamPointer[(m)+1]<<8))
#define RAM_POKE(m1,m2)			{mRamPointer[(m1)]=(m2);}

// ULONG cycles_used=0;

CSusie::CSusie(CSystem& parent) :mSystem(parent)
{
   TRACE_SUSIE0("CSusie()");
   Reset();
   cycles_used = 0;
}

CSusie::~CSusie()
{
   TRACE_SUSIE0("~CSusie()");
}

void CSusie::Reset(void)
{
   TRACE_SUSIE0("Reset()");

   // Fetch pointer to system RAM, faster than object access
   // and seeing as Susie only ever sees RAM.

   mRamPointer=mSystem.GetRamPointer();

   // Reset ALL variables

   mTMPADR.Word=0;
   mTILTACUM.Word=0;
   mHOFF.Word=0;
   mVOFF.Word=0;
   mVIDBAS.Word=0;
   mCOLLBAS.Word=0;
   mVIDADR.Word=0;
   mCOLLADR.Word=0;
   mSCBNEXT.Word=0;
   mSPRDLINE.Word=0;
   mHPOSSTRT.Word=0;
   mVPOSSTRT.Word=0;
   mSPRHSIZ.Word=0;
   mSPRVSIZ.Word=0;
   mSTRETCH.Word=0;
   mTILT.Word=0;
   mSPRDOFF.Word=0;
   mSPRVPOS.Word=0;
   mCOLLOFF.Word=0;
   mVSIZACUM.Word=0;
   mHSIZACUM.Word=0;
   mHSIZOFF.Word=0x007f;
   mVSIZOFF.Word=0x007f;
   mSCBADR.Word=0;
   mPROCADR.Word=0;

   // Must be initialised to this due to
   // stun runner math initialisation bug
   // see whatsnew for 0.7
   mMATHABCD.Long=0xffffffff;
   mMATHEFGH.Long=0xffffffff;
   mMATHJKLM.Long=0xffffffff;
   mMATHNP.Long=0xffff;

   mMATHAB_sign=1;
   mMATHCD_sign=1;
   mMATHEFGH_sign=1;

   mSPRCTL0_Type=0;
   mSPRCTL0_Vflip=0;
   mSPRCTL0_Hflip=0;
   mSPRCTL0_PixelBits=0;

   mSPRCTL1_StartLeft=0;
   mSPRCTL1_StartUp=0;
   mSPRCTL1_SkipSprite=0;
   mSPRCTL1_ReloadPalette=0;
   mSPRCTL1_ReloadDepth=0;
   mSPRCTL1_Sizing=0;
   mSPRCTL1_Literal=0;

   mSPRCOLL_Number=0;
   mSPRCOLL_Collide=0;

   mSPRSYS_StopOnCurrent=0;
   mSPRSYS_LeftHand=0;
   mSPRSYS_VStretch=0;
   mSPRSYS_NoCollide=0;
   mSPRSYS_Accumulate=0;
   mSPRSYS_SignedMath=0;
   mSPRSYS_Status=0;
   mSPRSYS_UnsafeAccess=0;
   mSPRSYS_LastCarry=0;
   mSPRSYS_Mathbit=0;
   mSPRSYS_MathInProgress=0;

   mSUZYBUSEN=FALSE;

   mSPRINIT.Byte=0;

   mSPRGO=FALSE;
   mEVERON=FALSE;

   for(int loop=0;loop<16;loop++) mPenIndex[loop]=loop;

   mJOYSTICK.Byte=0;
   mSWITCHES.Byte=0;
#ifdef MY_NO_STATIC
  vquadoff = 0;
  hquadoff = 0;
#endif
}

bool CSusie::ContextSave(FILE *fp)
{
   TRACE_SUSIE0("ContextSave()");

   if(!fprintf(fp,"CSusie::ContextSave")) return 0;

   if(!fwrite(&mTMPADR,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mTILTACUM,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mHOFF,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mVOFF,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mVIDBAS,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mCOLLBAS,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mVIDADR,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mCOLLADR,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mSCBNEXT,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mSPRDLINE,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mHPOSSTRT,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mVPOSSTRT,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mSPRHSIZ,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mSPRVSIZ,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mSTRETCH,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mTILT,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mSPRDOFF,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mSPRVPOS,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mCOLLOFF,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mVSIZACUM,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mHSIZACUM,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mHSIZOFF,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mVSIZOFF,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mSCBADR,sizeof(UUWORD),1,fp)) return 0;
   if(!fwrite(&mPROCADR,sizeof(UUWORD),1,fp)) return 0;

   if(!fwrite(&mMATHABCD,sizeof(TMATHABCD),1,fp)) return 0;
   if(!fwrite(&mMATHEFGH,sizeof(TMATHEFGH),1,fp)) return 0;
   if(!fwrite(&mMATHJKLM,sizeof(TMATHJKLM),1,fp)) return 0;
   if(!fwrite(&mMATHNP,sizeof(TMATHNP),1,fp)) return 0;

   if(!fwrite(&mSPRCTL0_Type,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRCTL0_Vflip,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRCTL0_Hflip,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRCTL0_PixelBits,sizeof(ULONG),1,fp)) return 0;

   if(!fwrite(&mSPRCTL1_StartLeft,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRCTL1_StartUp,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRCTL1_SkipSprite,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRCTL1_ReloadPalette,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRCTL1_ReloadDepth,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRCTL1_Sizing,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRCTL1_Literal,sizeof(ULONG),1,fp)) return 0;

   if(!fwrite(&mSPRCOLL_Number,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRCOLL_Collide,sizeof(ULONG),1,fp)) return 0;

   if(!fwrite(&mSPRSYS_StopOnCurrent,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRSYS_LeftHand,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRSYS_VStretch,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRSYS_NoCollide,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRSYS_Accumulate,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRSYS_SignedMath,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRSYS_Status,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRSYS_UnsafeAccess,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRSYS_LastCarry,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRSYS_Mathbit,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mSPRSYS_MathInProgress,sizeof(ULONG),1,fp)) return 0;

   if(!fwrite(&mSUZYBUSEN,sizeof(ULONG),1,fp)) return 0;

   if(!fwrite(&mSPRINIT,sizeof(TSPRINIT),1,fp)) return 0;

   if(!fwrite(&mSPRGO,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mEVERON,sizeof(ULONG),1,fp)) return 0;

   if(!fwrite(mPenIndex,sizeof(UBYTE),16,fp)) return 0;

   if(!fwrite(&mLineType,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mLineShiftRegCount,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mLineShiftReg,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mLineRepeatCount,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mLinePixel,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mLinePacketBitsLeft,sizeof(ULONG),1,fp)) return 0;

   if(!fwrite(&mCollision,sizeof(ULONG),1,fp)) return 0;

   if(!fwrite(&mLineBaseAddress,sizeof(ULONG),1,fp)) return 0;
   if(!fwrite(&mLineCollisionAddress,sizeof(ULONG),1,fp)) return 0;

   if(!fwrite(&mJOYSTICK,sizeof(TJOYSTICK),1,fp)) return 0;
   if(!fwrite(&mSWITCHES,sizeof(TSWITCHES),1,fp)) return 0;

   return 1;
}

bool CSusie::ContextLoad(LSS_FILE *fp)
{
   TRACE_SUSIE0("ContextLoad()");

   char teststr[100]="XXXXXXXXXXXXXXXXXXX";
   if(!lss_read(teststr,sizeof(char),19,fp)) return 0;
   if(strcmp(teststr,"CSusie::ContextSave")!=0) return 0;

   if(!lss_read(&mTMPADR,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mTILTACUM,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mHOFF,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mVOFF,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mVIDBAS,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mCOLLBAS,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mVIDADR,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mCOLLADR,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mSCBNEXT,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mSPRDLINE,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mHPOSSTRT,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mVPOSSTRT,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mSPRHSIZ,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mSPRVSIZ,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mSTRETCH,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mTILT,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mSPRDOFF,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mSPRVPOS,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mCOLLOFF,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mVSIZACUM,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mHSIZACUM,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mHSIZOFF,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mVSIZOFF,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mSCBADR,sizeof(UUWORD),1,fp)) return 0;
   if(!lss_read(&mPROCADR,sizeof(UUWORD),1,fp)) return 0;

   if(!lss_read(&mMATHABCD,sizeof(TMATHABCD),1,fp)) return 0;
   if(!lss_read(&mMATHEFGH,sizeof(TMATHEFGH),1,fp)) return 0;
   if(!lss_read(&mMATHJKLM,sizeof(TMATHJKLM),1,fp)) return 0;
   if(!lss_read(&mMATHNP,sizeof(TMATHNP),1,fp)) return 0;

   if(!lss_read(&mSPRCTL0_Type,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRCTL0_Vflip,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRCTL0_Hflip,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRCTL0_PixelBits,sizeof(ULONG),1,fp)) return 0;

   if(!lss_read(&mSPRCTL1_StartLeft,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRCTL1_StartUp,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRCTL1_SkipSprite,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRCTL1_ReloadPalette,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRCTL1_ReloadDepth,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRCTL1_Sizing,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRCTL1_Literal,sizeof(ULONG),1,fp)) return 0;

   if(!lss_read(&mSPRCOLL_Number,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRCOLL_Collide,sizeof(ULONG),1,fp)) return 0;

   if(!lss_read(&mSPRSYS_StopOnCurrent,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRSYS_LeftHand,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRSYS_VStretch,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRSYS_NoCollide,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRSYS_Accumulate,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRSYS_SignedMath,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRSYS_Status,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRSYS_UnsafeAccess,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRSYS_LastCarry,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRSYS_Mathbit,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mSPRSYS_MathInProgress,sizeof(ULONG),1,fp)) return 0;

   if(!lss_read(&mSUZYBUSEN,sizeof(ULONG),1,fp)) return 0;

   if(!lss_read(&mSPRINIT,sizeof(TSPRINIT),1,fp)) return 0;

   if(!lss_read(&mSPRGO,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mEVERON,sizeof(ULONG),1,fp)) return 0;

   if(!lss_read(mPenIndex,sizeof(UBYTE),16,fp)) return 0;

   if(!lss_read(&mLineType,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mLineShiftRegCount,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mLineShiftReg,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mLineRepeatCount,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mLinePixel,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mLinePacketBitsLeft,sizeof(ULONG),1,fp)) return 0;

   if(!lss_read(&mCollision,sizeof(ULONG),1,fp)) return 0;

   if(!lss_read(&mLineBaseAddress,sizeof(ULONG),1,fp)) return 0;
   if(!lss_read(&mLineCollisionAddress,sizeof(ULONG),1,fp)) return 0;

   if(!lss_read(&mJOYSTICK,sizeof(TJOYSTICK),1,fp)) return 0;
   if(!lss_read(&mSWITCHES,sizeof(TSWITCHES),1,fp)) return 0;

   return 1;
}

#ifndef SUSIE_INLINE_DoMathMultiply
void CSusie::DoMathMultiply(void)
{
    #include "susie_DoMathMultiply.h"
}
#endif

#ifndef SUSIE_INLINE_DoMathDivide
inline void CSusie::DoMathDivide(void)
{
    #include "susie_DoMathDivide.h"
}
#endif

#ifndef SUSIE_INLINE_PaintSprites
ULONG CSusie::PaintSprites(void)
{
    #include "susie_paintsprites.h"
}
#endif

#ifndef SUSIE_INLINE_PROCESSPIXEL
void CSusie::ProcessPixel(ULONG hoff,ULONG pixel)
{
    #include "susie_processpixel.h"
}
#endif

#ifndef SUSIE_INLINE_WRITEPIXEL
 void CSusie::WritePixel(ULONG hoff,ULONG pixel)
{
   #include "susie_writepixel.h"
}
#endif

#ifndef SUSIE_INLINE_READPIXEL
 ULONG CSusie::ReadPixel(ULONG hoff)
{
   #include "susie_readpixel.h"
}
#endif


#ifndef SUSIE_INLINE_WRITECOLLISION
 void CSusie::WriteCollision(ULONG hoff,ULONG pixel)
{
    #include "susie_writecollision.h"
}
#endif

#ifndef SUSIE_INLINE_READCOLLISION
 ULONG CSusie::ReadCollision(ULONG hoff)
{
    #include "susie_readcollision.h"
}
#endif


#ifndef SUSIE_INLINE_LineInit
 ULONG CSusie::LineInit(ULONG voff)
{
    #include "susie_LineInit.h"
}
#endif

#ifndef SUSIE_INLINE_LineGetPixel
 ULONG CSusie::LineGetPixel()
{
    #include "susie_LineGetPixel.h"
}
#endif

#ifndef SUSIE_INLINE_LineGetBits
 ULONG CSusie::LineGetBits(ULONG bits)
{
    #include "susie_LineGetBits.h"
}
#endif


void CSusie::Poke(ULONG addr,UBYTE data)
{
   switch(addr&0xff)
   {
      case (TMPADRL&0xff):
         mTMPADR.Byte.Low=data;
         mTMPADR.Byte.High=0;
         TRACE_SUSIE2("Poke(TMPADRL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (TMPADRH&0xff):
         mTMPADR.Byte.High=data;
         TRACE_SUSIE2("Poke(TMPADRH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (TILTACUML&0xff):
         mTILTACUM.Byte.Low=data;
         mTILTACUM.Byte.High=0;
         TRACE_SUSIE2("Poke(TILTACUML,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (TILTACUMH&0xff):
         mTILTACUM.Byte.High=data;
         TRACE_SUSIE2("Poke(TILTACUMH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (HOFFL&0xff):
         mHOFF.Byte.Low=data;
         mHOFF.Byte.High=0;
         TRACE_SUSIE2("Poke(HOFFL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (HOFFH&0xff):
         mHOFF.Byte.High=data;
         TRACE_SUSIE2("Poke(HOFFH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (VOFFL&0xff):
         mVOFF.Byte.Low=data;
         mVOFF.Byte.High=0;
         TRACE_SUSIE2("Poke(VOFFL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (VOFFH&0xff):
         mVOFF.Byte.High=data;
         TRACE_SUSIE2("Poke(VOFFH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (VIDBASL&0xff):
         mVIDBAS.Byte.Low=data;
         mVIDBAS.Byte.High=0;
         TRACE_SUSIE2("Poke(VIDBASL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (VIDBASH&0xff):
         mVIDBAS.Byte.High=data;
         TRACE_SUSIE2("Poke(VIDBASH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (COLLBASL&0xff):
         mCOLLBAS.Byte.Low=data;
         mCOLLBAS.Byte.High=0;
         TRACE_SUSIE2("Poke(COLLBASL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (COLLBASH&0xff):
         mCOLLBAS.Byte.High=data;
         TRACE_SUSIE2("Poke(COLLBASH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (VIDADRL&0xff):
         mVIDADR.Byte.Low=data;
         mVIDADR.Byte.High=0;
         TRACE_SUSIE2("Poke(VIDADRL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (VIDADRH&0xff):
         mVIDADR.Byte.High=data;
         TRACE_SUSIE2("Poke(VIDADRH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (COLLADRL&0xff):
         mCOLLADR.Byte.Low=data;
         mCOLLADR.Byte.High=0;
         TRACE_SUSIE2("Poke(COLLADRL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (COLLADRH&0xff):
         mCOLLADR.Byte.High=data;
         TRACE_SUSIE2("Poke(COLLADRH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (SCBNEXTL&0xff):
         mSCBNEXT.Byte.Low=data;
         mSCBNEXT.Byte.High=0;
         TRACE_SUSIE2("Poke(SCBNEXTL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (SCBNEXTH&0xff):
         mSCBNEXT.Byte.High=data;
         TRACE_SUSIE2("Poke(SCBNEXTH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (SPRDLINEL&0xff):
         mSPRDLINE.Byte.Low=data;
         mSPRDLINE.Byte.High=0;
         TRACE_SUSIE2("Poke(SPRDLINEL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (SPRDLINEH&0xff):
         mSPRDLINE.Byte.High=data;
         TRACE_SUSIE2("Poke(SPRDLINEH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (HPOSSTRTL&0xff):
         mHPOSSTRT.Byte.Low=data;
         mHPOSSTRT.Byte.High=0;
         TRACE_SUSIE2("Poke(HPOSSTRTL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (HPOSSTRTH&0xff):
         mHPOSSTRT.Byte.High=data;
         TRACE_SUSIE2("Poke(HPOSSTRTH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (VPOSSTRTL&0xff):
         mVPOSSTRT.Byte.Low=data;
         mVPOSSTRT.Byte.High=0;
         TRACE_SUSIE2("Poke(VPOSSTRTL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (VPOSSTRTH&0xff):
         mVPOSSTRT.Byte.High=data;
         TRACE_SUSIE2("Poke(VPOSSTRTH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (SPRHSIZL&0xff):
         mSPRHSIZ.Byte.Low=data;
         mSPRHSIZ.Byte.High=0;
         TRACE_SUSIE2("Poke(SPRHSIZL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (SPRHSIZH&0xff):
         mSPRHSIZ.Byte.High=data;
         TRACE_SUSIE2("Poke(SPRHSIZH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (SPRVSIZL&0xff):
         mSPRVSIZ.Byte.Low=data;
         mSPRVSIZ.Byte.High=0;
         TRACE_SUSIE2("Poke(SPRVSIZL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (SPRVSIZH&0xff):
         mSPRVSIZ.Byte.High=data;
         TRACE_SUSIE2("Poke(SPRVSIZH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (STRETCHL&0xff):
         mSTRETCH.Byte.Low=data;
         mSTRETCH.Byte.High=0;
         TRACE_SUSIE2("Poke(STRETCHL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (STRETCHH&0xff):
         TRACE_SUSIE2("Poke(STRETCHH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         mSTRETCH.Byte.High=data;
         break;
      case (TILTL&0xff):
         mTILT.Byte.Low=data;
         mTILT.Byte.High=0;
         TRACE_SUSIE2("Poke(TILTL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (TILTH&0xff):
         mTILT.Byte.High=data;
         TRACE_SUSIE2("Poke(TILTH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (SPRDOFFL&0xff):
         TRACE_SUSIE2("Poke(SPRDOFFL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         mSPRDOFF.Byte.Low=data;
         mSPRDOFF.Byte.High=0;
         break;
      case (SPRDOFFH&0xff):
         TRACE_SUSIE2("Poke(SPRDOFFH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         mSPRDOFF.Byte.High=data;
         break;
      case (SPRVPOSL&0xff):
         TRACE_SUSIE2("Poke(SPRVPOSL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         mSPRVPOS.Byte.Low=data;
         mSPRVPOS.Byte.High=0;
         break;
      case (SPRVPOSH&0xff):
         mSPRVPOS.Byte.High=data;
         TRACE_SUSIE2("Poke(SPRVPOSH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (COLLOFFL&0xff):
         mCOLLOFF.Byte.Low=data;
         mCOLLOFF.Byte.High=0;
         TRACE_SUSIE2("Poke(COLLOFFL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (COLLOFFH&0xff):
         mCOLLOFF.Byte.High=data;
         TRACE_SUSIE2("Poke(COLLOFFH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (VSIZACUML&0xff):
         mVSIZACUM.Byte.Low=data;
         mVSIZACUM.Byte.High=0;
         TRACE_SUSIE2("Poke(VSIZACUML,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (VSIZACUMH&0xff):
         mVSIZACUM.Byte.High=data;
         TRACE_SUSIE2("Poke(VSIZACUMH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (HSIZOFFL&0xff):
         mHSIZOFF.Byte.Low=data;
         mHSIZOFF.Byte.High=0;
         TRACE_SUSIE2("Poke(HSIZOFFL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (HSIZOFFH&0xff):
         mHSIZOFF.Byte.High=data;
         TRACE_SUSIE2("Poke(HSIZOFFH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (VSIZOFFL&0xff):
         mVSIZOFF.Byte.Low=data;
         mVSIZOFF.Byte.High=0;
         TRACE_SUSIE2("Poke(VSIZOFFL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (VSIZOFFH&0xff):
         mVSIZOFF.Byte.High=data;
         TRACE_SUSIE2("Poke(VSIZOFFH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (SCBADRL&0xff):
         mSCBADR.Byte.Low=data;
         mSCBADR.Byte.High=0;
         TRACE_SUSIE2("Poke(SCBADRL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (SCBADRH&0xff):
         mSCBADR.Byte.High=data;
         TRACE_SUSIE2("Poke(SCBADRH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (PROCADRL&0xff):
         mPROCADR.Byte.Low=data;
         mPROCADR.Byte.High=0;
         TRACE_SUSIE2("Poke(PROCADRL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (PROCADRH&0xff):
         mPROCADR.Byte.High=data;
         TRACE_SUSIE2("Poke(PROCADRH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;

      case (MATHD&0xff):
         TRACE_SUSIE2("Poke(MATHD,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         mMATHABCD.Bytes.D=data;
         //			mMATHABCD.Bytes.C=0;
         // The hardware manual says that the sign shouldnt change
         // but if I dont do this then stun runner will hang as it
         // does the init in the wrong order and if the previous
         // calc left a zero there then we'll get a sign error
         Poke(MATHC,0);
         break;
      case (MATHC&0xff):
         TRACE_SUSIE2("Poke(MATHC,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         mMATHABCD.Bytes.C=data;
         // Perform sign conversion if required
         if(mSPRSYS_SignedMath)
         {
            // Account for the math bug that 0x8000 is +ve & 0x0000 is -ve by subracting 1
            if((mMATHABCD.Words.CD-1)&0x8000)
            {
               UWORD conv;
               conv=mMATHABCD.Words.CD^0xffff;
               conv++;
               mMATHCD_sign=-1;
               TRACE_SUSIE2("MATH CD signed conversion complete %04x to %04x",mMATHABCD.Words.CD,conv);
               mMATHABCD.Words.CD=conv;
            }
            else
            {
               mMATHCD_sign=1;
            }
         }
         break;
      case (MATHB&0xff):
         TRACE_SUSIE2("Poke(MATHB,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         mMATHABCD.Bytes.B=data;
         mMATHABCD.Bytes.A=0;
         break;
      case (MATHA&0xff):
         TRACE_SUSIE2("Poke(MATHA,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         mMATHABCD.Bytes.A=data;
         // Perform sign conversion if required
         if(mSPRSYS_SignedMath)
         {
            // Account for the math bug that 0x8000 is +ve & 0x0000 is -ve by subracting 1
            if((mMATHABCD.Words.AB-1)&0x8000)
            {
               UWORD conv;
               conv=mMATHABCD.Words.AB^0xffff;
               conv++;
               mMATHAB_sign=-1;
               TRACE_SUSIE2("MATH AB signed conversion complete %04x to %04x",mMATHABCD.Words.AB,conv);
               mMATHABCD.Words.AB=conv;
            }
            else
            {
               mMATHAB_sign=1;
            }
         }
         DoMathMultiply();
         break;

      case (MATHP&0xff):
         mMATHNP.Bytes.P=data;
         mMATHNP.Bytes.N=0;
         TRACE_SUSIE2("Poke(MATHP,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (MATHN&0xff):
         mMATHNP.Bytes.N=data;
         TRACE_SUSIE2("Poke(MATHN,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;

      case (MATHH&0xff):
         mMATHEFGH.Bytes.H=data;
         mMATHEFGH.Bytes.G=0;
         TRACE_SUSIE2("Poke(MATHH,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (MATHG&0xff):
         mMATHEFGH.Bytes.G=data;
         TRACE_SUSIE2("Poke(MATHG,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (MATHF&0xff):
         mMATHEFGH.Bytes.F=data;
         mMATHEFGH.Bytes.E=0;
         TRACE_SUSIE2("Poke(MATHF,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (MATHE&0xff):
         mMATHEFGH.Bytes.E=data;
         TRACE_SUSIE2("Poke(MATHE,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         DoMathDivide();
         break;

      case (MATHM&0xff):
         mMATHJKLM.Bytes.M=data;
         mMATHJKLM.Bytes.L=0;
         mSPRSYS_Mathbit=FALSE;
         TRACE_SUSIE2("Poke(MATHM,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (MATHL&0xff):
         mMATHJKLM.Bytes.L=data;
         TRACE_SUSIE2("Poke(MATHL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (MATHK&0xff):
         mMATHJKLM.Bytes.K=data;
         mMATHJKLM.Bytes.J=0;
         TRACE_SUSIE2("Poke(MATHK,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (MATHJ&0xff):
         mMATHJKLM.Bytes.J=data;
         TRACE_SUSIE2("Poke(MATHJ,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;

      case (SPRCTL0&0xff):
         mSPRCTL0_Type=data&0x0007;
         mSPRCTL0_Vflip=data&0x0010;
         mSPRCTL0_Hflip=data&0x0020;
         mSPRCTL0_PixelBits=((data&0x00c0)>>6)+1;
         TRACE_SUSIE2("Poke(SPRCTL0,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (SPRCTL1&0xff):
         mSPRCTL1_StartLeft=data&0x0001;
         mSPRCTL1_StartUp=data&0x0002;
         mSPRCTL1_SkipSprite=data&0x0004;
         mSPRCTL1_ReloadPalette=data&0x0008;
         mSPRCTL1_ReloadDepth=(data&0x0030)>>4;
         mSPRCTL1_Sizing=data&0x0040;
         mSPRCTL1_Literal=data&0x0080;
         TRACE_SUSIE2("Poke(SPRCTL1,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (SPRCOLL&0xff):
         mSPRCOLL_Number=data&0x000f;
         mSPRCOLL_Collide=data&0x0020;
         TRACE_SUSIE2("Poke(SPRCOLL,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (SPRINIT&0xff):
         mSPRINIT.Byte=data;
         TRACE_SUSIE2("Poke(SPRINIT,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (SUZYBUSEN&0xff):
         mSUZYBUSEN=data&0x01;
         TRACE_SUSIE2("Poke(SUZYBUSEN,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (SPRGO&0xff):
         mSPRGO=data&0x01;
         mEVERON=data&0x04;
         TRACE_SUSIE2("Poke(SPRGO,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (SPRSYS&0xff):
         mSPRSYS_StopOnCurrent=data&0x0002;
         if(data&0x0004) mSPRSYS_UnsafeAccess=0;
         mSPRSYS_LeftHand=data&0x0008;
         mSPRSYS_VStretch=data&0x0010;
         mSPRSYS_NoCollide=data&0x0020;
         mSPRSYS_Accumulate=data&0x0040;
         mSPRSYS_SignedMath=data&0x0080;
         TRACE_SUSIE2("Poke(SPRSYS,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;

         // Cartridge writing ports

      case (RCART0&0xff):
         if(mSystem.mCart->CartGetAudin() && mSystem.mMikie->SwitchAudInValue()){
           mSystem.Poke_CARTB0A(data);
         }else{
           mSystem.Poke_CARTB0(data);
         }
         mSystem.mEEPROM->ProcessEepromCounter(mSystem.mCart->GetCounterValue());
         TRACE_SUSIE2("Poke(RCART0,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;
      case (RCART1&0xff):
        if(mSystem.mCart->CartGetAudin() && mSystem.mMikie->SwitchAudInValue()){
           mSystem.Poke_CARTB1A(data);
        }else{
            mSystem.Poke_CARTB1(data);
         }
         mSystem.mEEPROM->ProcessEepromCounter(mSystem.mCart->GetCounterValue());
         TRACE_SUSIE2("Poke(RCART1,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;

         // These are not so important, so lets ignore them for the moment

      case (LEDS&0xff):
      case (PPORTSTAT&0xff):
      case (PPORTDATA&0xff):
      case (HOWIE&0xff):
         TRACE_SUSIE2("Poke(LEDS/PPORTSTST/PPORTDATA/HOWIE,%02x) at PC=$%04x",data,mSystem.mCpu->GetPC());
         break;

         // Errors on read only register accesses

      case (SUZYHREV&0xff):
      case (JOYSTICK&0xff):
      case (SWITCHES&0xff):
         TRACE_SUSIE3("Poke(%04x,%02x) - Poke to read only register location at PC=%04x",addr,data,mSystem.mCpu->GetPC());
         break;

         // Errors on illegal location accesses

      default:
         TRACE_SUSIE3("Poke(%04x,%02x) - Poke to illegal location at PC=%04x",addr,data,mSystem.mCpu->GetPC());
         break;
   }
}

UBYTE CSusie::Peek(ULONG addr)
{
   UBYTE	retval=0;

   switch(addr&0xff)
   {
      case (TMPADRL&0xff):
         retval=mTMPADR.Byte.Low;
         TRACE_SUSIE2("Peek(TMPADRL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (TMPADRH&0xff):
         retval=mTMPADR.Byte.High;
         TRACE_SUSIE2("Peek(TMPADRH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (TILTACUML&0xff):
         retval=mTILTACUM.Byte.Low;
         TRACE_SUSIE2("Peek(TILTACUML)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (TILTACUMH&0xff):
         retval=mTILTACUM.Byte.High;
         TRACE_SUSIE2("Peek(TILTACUMH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (HOFFL&0xff):
         retval=mHOFF.Byte.Low;
         TRACE_SUSIE2("Peek(HOFFL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (HOFFH&0xff):
         retval=mHOFF.Byte.High;
         TRACE_SUSIE2("Peek(HOFFH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (VOFFL&0xff):
         retval=mVOFF.Byte.Low;
         TRACE_SUSIE2("Peek(VOFFL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (VOFFH&0xff):
         retval=mVOFF.Byte.High;
         TRACE_SUSIE2("Peek(VOFFH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (VIDBASL&0xff):
         retval=mVIDBAS.Byte.Low;
         TRACE_SUSIE2("Peek(VIDBASL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (VIDBASH&0xff):
         retval=mVIDBAS.Byte.High;
         TRACE_SUSIE2("Peek(VIDBASH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (COLLBASL&0xff):
         retval=mCOLLBAS.Byte.Low;
         TRACE_SUSIE2("Peek(COLLBASL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (COLLBASH&0xff):
         retval=mCOLLBAS.Byte.High;
         TRACE_SUSIE2("Peek(COLLBASH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (VIDADRL&0xff):
         retval=mVIDADR.Byte.Low;
         TRACE_SUSIE2("Peek(VIDADRL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (VIDADRH&0xff):
         retval=mVIDADR.Byte.High;
         TRACE_SUSIE2("Peek(VIDADRH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (COLLADRL&0xff):
         retval=mCOLLADR.Byte.Low;
         TRACE_SUSIE2("Peek(COLLADRL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (COLLADRH&0xff):
         retval=mCOLLADR.Byte.High;
         TRACE_SUSIE2("Peek(COLLADRH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (SCBNEXTL&0xff):
         retval=mSCBNEXT.Byte.Low;
         TRACE_SUSIE2("Peek(SCBNEXTL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (SCBNEXTH&0xff):
         retval=mSCBNEXT.Byte.High;
         TRACE_SUSIE2("Peek(SCBNEXTH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (SPRDLINEL&0xff):
         retval=mSPRDLINE.Byte.Low;
         TRACE_SUSIE2("Peek(SPRDLINEL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (SPRDLINEH&0xff):
         retval=mSPRDLINE.Byte.High;
         TRACE_SUSIE2("Peek(SPRDLINEH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (HPOSSTRTL&0xff):
         retval=mHPOSSTRT.Byte.Low;
         TRACE_SUSIE2("Peek(HPOSSTRTL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (HPOSSTRTH&0xff):
         retval=mHPOSSTRT.Byte.High;
         TRACE_SUSIE2("Peek(HPOSSTRTH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (VPOSSTRTL&0xff):
         retval=mVPOSSTRT.Byte.Low;
         TRACE_SUSIE2("Peek(VPOSSTRTL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (VPOSSTRTH&0xff):
         retval=mVPOSSTRT.Byte.High;
         TRACE_SUSIE2("Peek(VPOSSTRTH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (SPRHSIZL&0xff):
         retval=mSPRHSIZ.Byte.Low;
         TRACE_SUSIE2("Peek(SPRHSIZL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (SPRHSIZH&0xff):
         retval=mSPRHSIZ.Byte.High;
         TRACE_SUSIE2("Peek(SPRHSIZH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (SPRVSIZL&0xff):
         retval=mSPRVSIZ.Byte.Low;
         TRACE_SUSIE2("Peek(SPRVSIZL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (SPRVSIZH&0xff):
         retval=mSPRVSIZ.Byte.High;
         TRACE_SUSIE2("Peek(SPRVSIZH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (STRETCHL&0xff):
         retval=mSTRETCH.Byte.Low;
         TRACE_SUSIE2("Peek(STRETCHL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (STRETCHH&0xff):
         retval=mSTRETCH.Byte.High;
         TRACE_SUSIE2("Peek(STRETCHH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (TILTL&0xff):
         retval=mTILT.Byte.Low;
         TRACE_SUSIE2("Peek(TILTL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (TILTH&0xff):
         retval=mTILT.Byte.High;
         TRACE_SUSIE2("Peek(TILTH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (SPRDOFFL&0xff):
         retval=mSPRDOFF.Byte.Low;
         TRACE_SUSIE2("Peek(SPRDOFFL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (SPRDOFFH&0xff):
         retval=mSPRDOFF.Byte.High;
         TRACE_SUSIE2("Peek(SPRDOFFH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (SPRVPOSL&0xff):
         retval=mSPRVPOS.Byte.Low;
         TRACE_SUSIE2("Peek(SPRVPOSL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (SPRVPOSH&0xff):
         retval=mSPRVPOS.Byte.High;
         TRACE_SUSIE2("Peek(SPRVPOSH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (COLLOFFL&0xff):
         retval=mCOLLOFF.Byte.Low;
         TRACE_SUSIE2("Peek(COLLOFFL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (COLLOFFH&0xff):
         retval=mCOLLOFF.Byte.High;
         TRACE_SUSIE2("Peek(COLLOFFH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (VSIZACUML&0xff):
         retval=mVSIZACUM.Byte.Low;
         TRACE_SUSIE2("Peek(VSIZACUML)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (VSIZACUMH&0xff):
         retval=mVSIZACUM.Byte.High;
         TRACE_SUSIE2("Peek(VSIZACUMH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (HSIZOFFL&0xff):
         retval=mHSIZOFF.Byte.Low;
         TRACE_SUSIE2("Peek(HSIZOFFL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (HSIZOFFH&0xff):
         retval=mHSIZOFF.Byte.High;
         TRACE_SUSIE2("Peek(HSIZOFFH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (VSIZOFFL&0xff):
         retval=mVSIZOFF.Byte.Low;
         TRACE_SUSIE2("Peek(VSIZOFFL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (VSIZOFFH&0xff):
         retval=mVSIZOFF.Byte.High;
         TRACE_SUSIE2("Peek(VSIZOFFH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (SCBADRL&0xff):
         retval=mSCBADR.Byte.Low;
         TRACE_SUSIE2("Peek(SCBADRL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (SCBADRH&0xff):
         retval=mSCBADR.Byte.High;
         TRACE_SUSIE2("Peek(SCBADRH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (PROCADRL&0xff):
         retval=mPROCADR.Byte.Low;
         TRACE_SUSIE2("Peek(PROCADRL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (PROCADRH&0xff):
         retval=mPROCADR.Byte.High;
         TRACE_SUSIE2("Peek(PROCADRH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (MATHD&0xff):
         retval=mMATHABCD.Bytes.D;
         TRACE_SUSIE2("Peek(MATHD)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (MATHC&0xff):
         retval=mMATHABCD.Bytes.C;
         TRACE_SUSIE2("Peek(MATHC)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (MATHB&0xff):
         retval=mMATHABCD.Bytes.B;
         TRACE_SUSIE2("Peek(MATHB)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (MATHA&0xff):
         retval=mMATHABCD.Bytes.A;
         TRACE_SUSIE2("Peek(MATHA)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;

      case (MATHP&0xff):
         retval=mMATHNP.Bytes.P;
         TRACE_SUSIE2("Peek(MATHP)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (MATHN&0xff):
         retval=mMATHNP.Bytes.N;
         TRACE_SUSIE2("Peek(MATHN)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;

      case (MATHH&0xff):
         retval=mMATHEFGH.Bytes.H;
         TRACE_SUSIE2("Peek(MATHH)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (MATHG&0xff):
         retval=mMATHEFGH.Bytes.G;
         TRACE_SUSIE2("Peek(MATHG)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (MATHF&0xff):
         retval=mMATHEFGH.Bytes.F;
         TRACE_SUSIE2("Peek(MATHF)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (MATHE&0xff):
         retval=mMATHEFGH.Bytes.E;
         TRACE_SUSIE2("Peek(MATHE)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (MATHM&0xff):
         retval=mMATHJKLM.Bytes.M;
         TRACE_SUSIE2("Peek(MATHM)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (MATHL&0xff):
         retval=mMATHJKLM.Bytes.L;
         TRACE_SUSIE2("Peek(MATHL)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (MATHK&0xff):
         retval=mMATHJKLM.Bytes.K;
         TRACE_SUSIE2("Peek(MATHK)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (MATHJ&0xff):
         retval=mMATHJKLM.Bytes.J;
         TRACE_SUSIE2("Peek(MATHJ)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (SUZYHREV&0xff):
         retval=0x01;
         TRACE_SUSIE2("Peek(SUZYHREV)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;

      case (SPRSYS&0xff):
         retval=0x0000;
         //	retval+=(mSPRSYS_Status)?0x0001:0x0000;
         // Use gSystemCPUSleep to signal the status instead, if we are asleep then
         // we must be rendering sprites
         retval+=(gSystemCPUSleep)?0x0001:0x0000;
         retval+=(mSPRSYS_StopOnCurrent)?0x0002:0x0000;
         retval+=(mSPRSYS_UnsafeAccess)?0x0004:0x0000;
         retval+=(mSPRSYS_LeftHand)?0x0008:0x0000;
         retval+=(mSPRSYS_VStretch)?0x0010:0x0000;
         retval+=(mSPRSYS_LastCarry)?0x0020:0x0000;
         retval+=(mSPRSYS_Mathbit)?0x0040:0x0000;
         retval+=(mSPRSYS_MathInProgress)?0x0080:0x0000;
         TRACE_SUSIE2("Peek(SPRSYS)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;

      case (JOYSTICK&0xff):
         if(mSPRSYS_LeftHand)
         retval= mJOYSTICK.Byte;
         else
         {
            TJOYSTICK Modified=mJOYSTICK;
            Modified.Bits.Left=mJOYSTICK.Bits.Right;
            Modified.Bits.Right=mJOYSTICK.Bits.Left;
            Modified.Bits.Down=mJOYSTICK.Bits.Up;
            Modified.Bits.Up=mJOYSTICK.Bits.Down;
            retval= Modified.Byte;
         }
         //			TRACE_SUSIE2("Peek(JOYSTICK)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;

      case (SWITCHES&0xff):
         retval=mSWITCHES.Byte;
         //			TRACE_SUSIE2("Peek(SWITCHES)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;

         // Cartridge reading ports

      case (RCART0&0xff):
         if(mSystem.mCart->CartGetAudin() && mSystem.mMikie->SwitchAudInValue()){
            retval=mSystem.Peek_CARTB0A();
         }else{
            retval=mSystem.Peek_CARTB0();
         }
         mSystem.mEEPROM->ProcessEepromCounter(mSystem.mCart->GetCounterValue());
         //			TRACE_SUSIE2("Peek(RCART0)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;
      case (RCART1&0xff):
         if(mSystem.mCart->CartGetAudin() && mSystem.mMikie->SwitchAudInValue()){
            retval=mSystem.Peek_CARTB1A();
         }else{
           retval=mSystem.Peek_CARTB1();
         }
         mSystem.mEEPROM->ProcessEepromCounter(mSystem.mCart->GetCounterValue());
         //			TRACE_SUSIE2("Peek(RCART1)=$%02x at PC=$%04x",retval,mSystem.mCpu->GetPC());
         return retval;

         // These are no so important so lets ignore them for the moment

      case (LEDS&0xff):
      case (PPORTSTAT&0xff):
      case (PPORTDATA&0xff):
      case (HOWIE&0xff):
         TRACE_SUSIE1("Peek(LEDS/PPORTSTAT/PPORTDATA) at PC=$%04x",mSystem.mCpu->GetPC());
         break;

         // Errors on write only register accesses

      case (SPRCTL0&0xff):
      case (SPRCTL1&0xff):
      case (SPRCOLL&0xff):
      case (SPRINIT&0xff):
      case (SUZYBUSEN&0xff):
      case (SPRGO&0xff):
         TRACE_SUSIE2("Peek(%04x) - Peek from write only register location at PC=$%04x",addr,mSystem.mCpu->GetPC());
         break;

         // Errors on illegal location accesses

      default:
         TRACE_SUSIE2("Peek(%04x) - Peek from illegal location at PC=$%04x",addr,mSystem.mCpu->GetPC());
         break;
   }

   return 0xff;
}
