   UBYTE *bitmap_tmp=NULL;
   UBYTE source;
   ULONG loop;
   ULONG work_done=0;

   if(!mpDisplayBits) return 0;
   if(!mpDisplayCurrent) return 0;
   if(!mDISPCTL_DMAEnable) return 0;

   //   if(mLynxLine&0x80000000) return 0;

   // Set the timer interrupt flag
   if(mTimerInterruptMask&0x01)
   {
      TRACE_MIKIE0("Update() - TIMER0 IRQ Triggered (Line Timer)");
      mTimerStatusFlags|=0x01;
      gSystemIRQ=TRUE;  // Added 19/09/06 fix for IRQ issue
   }

   // Logic says it should be 101 but testing on an actual lynx shows the rest
   // persiod is between lines 102,101,100 with the new line being latched at
   // the beginning of count==99 hence the code below !!

   // Emulate REST signal
   if(mLynxLine==mTIM_2_BKUP-2 || mLynxLine==mTIM_2_BKUP-3 || mLynxLine==mTIM_2_BKUP-4) mIODAT_REST_SIGNAL=TRUE; else mIODAT_REST_SIGNAL=FALSE;

   if(mLynxLine==(mTIM_2_BKUP-3))
   {
      if(mDISPCTL_Flip)
      {
         mLynxAddr=mDisplayAddress&0xfffc;
         mLynxAddr+=3;
      }
      else
      {
         mLynxAddr=mDisplayAddress&0xfffc;
      }
      // Trigger line rending to start
      mLynxLineDMACounter=102;
   }

   // Decrement line counter logic
   if(mLynxLine) mLynxLine--;

   // Do 102 lines, nothing more, less is OK.
   if(mLynxLineDMACounter)
   {
      //        TRACE_MIKIE1("Update() - Screen DMA, line %03d",line_count);
      mLynxLineDMACounter--;

      // Cycle hit for a 80 RAM access in rendering a line
      work_done+=80*DMA_RDWR_CYC;

      // Mikie screen DMA can only see the system RAM....
      // (Step through bitmap, line at a time)

      // Assign the temporary pointer;
      bitmap_tmp=mpDisplayCurrent;

      switch(mDisplayRotate)
      {
         case MIKIE_NO_ROTATE:
            if (skipNextFrame) {
                    int cnt = SCREEN_WIDTH/2;
                  if(mDISPCTL_Flip)
                  {
                     mLynxAddr-=cnt;
                  }
                  else
                  {
                     mLynxAddr+=cnt;
                  }
                    mpDisplayCurrent+=mDisplayPitch;
                    break;
            }
            if(mDisplayFormat==MIKIE_PIXEL_FORMAT_8BPP)
            {
               for(loop=0;loop<SCREEN_WIDTH/2;loop++)
               {
                  source=mpRamPointer[mLynxAddr];
                  if(mDISPCTL_Flip)
                  {
                     mLynxAddr--;
                     *(bitmap_tmp)=(UBYTE)mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp+=sizeof(UBYTE);
                     *(bitmap_tmp)=(UBYTE)mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp+=sizeof(UBYTE);
                  }
                  else
                  {
                     mLynxAddr++;
                     *(bitmap_tmp)=(UBYTE)mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp+=sizeof(UBYTE);
                     *(bitmap_tmp)=(UBYTE)mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp+=sizeof(UBYTE);
                  }
               }
            }
            else if (mDisplayFormat==MIKIE_PIXEL_FORMAT_RAW) {
                UBYTE *r;
                if(mDISPCTL_Flip)
                {
                   mLynxAddr-=SCREEN_WIDTH/2;
                   r = &mpRamPointer[mLynxAddr];
                }
                else
                {
                   r = &mpRamPointer[mLynxAddr];
                   mLynxAddr+=SCREEN_WIDTH/2;
                }
                // mDISPCTL_Flip
                memcpy(bitmap_tmp, mPalette, 64);
                memcpy(bitmap_tmp+64, r, SCREEN_WIDTH/2);
                bitmap_tmp+=SCREEN_WIDTH/2+64;
                //memcpy(bitmap_tmp, r, SCREEN_WIDTH/2);
                //bitmap_tmp+=SCREEN_WIDTH/2;
            }
            else if(mDisplayFormat==MIKIE_PIXEL_FORMAT_16BPP_555 || mDisplayFormat==MIKIE_PIXEL_FORMAT_16BPP_565 || mDisplayFormat==MIKIE_PIXEL_FORMAT_16BPP_565_INV)
            {
               UBYTE *r = &mpRamPointer[mLynxAddr];
               for(loop=0;loop<SCREEN_WIDTH/2;loop++)
               {
                  //source=mpRamPointer[mLynxAddr];
                  source=*r;
                  if(mDISPCTL_Flip)
                  {
                     //mLynxAddr--;
                     r--;
                     *((UWORD*)(bitmap_tmp))=(UWORD)mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp+=sizeof(UWORD);
                     *((UWORD*)(bitmap_tmp))=(UWORD)mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp+=sizeof(UWORD);
                  }
                  else
                  {
                     //mLynxAddr++;
                     r++;
                     *((UWORD*)(bitmap_tmp))=(UWORD)mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp+=sizeof(UWORD);
                     *((UWORD*)(bitmap_tmp))=(UWORD)mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp+=sizeof(UWORD);
                  }
               }
               if(mDISPCTL_Flip)
               mLynxAddr-=SCREEN_WIDTH/2;
               else
               mLynxAddr+=SCREEN_WIDTH/2;
            }
            else if(mDisplayFormat==MIKIE_PIXEL_FORMAT_24BPP)
            {
               ULONG pixel;
               for(loop=0;loop<SCREEN_WIDTH/2;loop++)
               {
                  source=mpRamPointer[mLynxAddr];
                  if(mDISPCTL_Flip)
                  {
                     mLynxAddr--;
                     pixel=mColourMap[mPalette[source&0x0f].Index];
                     *bitmap_tmp++=(UBYTE)pixel;    pixel>>=8;
                     *bitmap_tmp++=(UBYTE)pixel;    pixel>>=8;
                     *bitmap_tmp++=(UBYTE)pixel;
                     pixel=mColourMap[mPalette[source>>4].Index];
                     *bitmap_tmp++=(UBYTE)pixel;    pixel>>=8;
                     *bitmap_tmp++=(UBYTE)pixel;    pixel>>=8;
                     *bitmap_tmp++=(UBYTE)pixel;
                  }
                  else
                  {
                     mLynxAddr++;
                     pixel=mColourMap[mPalette[source>>4].Index];
                     *bitmap_tmp++=(UBYTE)pixel;    pixel>>=8;
                     *bitmap_tmp++=(UBYTE)pixel;    pixel>>=8;
                     *bitmap_tmp++=(UBYTE)pixel;
                     pixel=mColourMap[mPalette[source&0x0f].Index];
                     *bitmap_tmp++=(UBYTE)pixel;    pixel>>=8;
                     *bitmap_tmp++=(UBYTE)pixel;    pixel>>=8;
                     *bitmap_tmp++=(UBYTE)pixel;
                  }
               }
            }
            else if(mDisplayFormat==MIKIE_PIXEL_FORMAT_32BPP)
            {
               for(loop=0;loop<SCREEN_WIDTH/2;loop++)
               {
                  source=mpRamPointer[mLynxAddr];
                  if(mDISPCTL_Flip)
                  {
                     mLynxAddr--;
                     *((ULONG*)(bitmap_tmp))=mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp+=sizeof(ULONG);
                     *((ULONG*)(bitmap_tmp))=mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp+=sizeof(ULONG);
                  }
                  else
                  {
                     mLynxAddr++;
                     *((ULONG*)(bitmap_tmp))=mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp+=sizeof(ULONG);
                     *((ULONG*)(bitmap_tmp))=mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp+=sizeof(ULONG);
                  }
               }
            }
            mpDisplayCurrent+=mDisplayPitch;
            break;
         case MIKIE_ROTATE_L:
            if(mDisplayFormat==MIKIE_PIXEL_FORMAT_8BPP)
            {
               for(loop=0;loop<SCREEN_WIDTH/2;loop++)
               {
                  source=mpRamPointer[mLynxAddr];
                  if(mDISPCTL_Flip)
                  {
                     mLynxAddr--;
                     *(bitmap_tmp)=(UBYTE)mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp+=mDisplayPitch;
                     *(bitmap_tmp)=(UBYTE)mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp+=mDisplayPitch;
                  }
                  else
                  {
                     mLynxAddr++;
                     *(bitmap_tmp)=(UBYTE)mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp+=mDisplayPitch;
                     *(bitmap_tmp)=(UBYTE)mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp+=mDisplayPitch;
                  }
               }
               mpDisplayCurrent-=sizeof(UBYTE);
            }
            else if(mDisplayFormat==MIKIE_PIXEL_FORMAT_16BPP_555 || mDisplayFormat==MIKIE_PIXEL_FORMAT_16BPP_565 || mDisplayFormat==MIKIE_PIXEL_FORMAT_16BPP_565_INV)
            {
               for(loop=0;loop<SCREEN_WIDTH/2;loop++)
               {
                  source=mpRamPointer[mLynxAddr];
                  if(mDISPCTL_Flip)
                  {
                     mLynxAddr--;
                     *((UWORD*)(bitmap_tmp))=(UWORD)mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp+=mDisplayPitch;
                     *((UWORD*)(bitmap_tmp))=(UWORD)mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp+=mDisplayPitch;
                  }
                  else
                  {
                     mLynxAddr++;
                     *((UWORD*)(bitmap_tmp))=(UWORD)mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp+=mDisplayPitch;
                     *((UWORD*)(bitmap_tmp))=(UWORD)mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp+=mDisplayPitch;
                  }
               }
               mpDisplayCurrent-=sizeof(UWORD);
            }
            else if(mDisplayFormat==MIKIE_PIXEL_FORMAT_24BPP)
            {
               ULONG pixel;
               for(loop=0;loop<SCREEN_WIDTH/2;loop++)
               {
                  source=mpRamPointer[mLynxAddr];
                  if(mDISPCTL_Flip)
                  {
                     mLynxAddr--;
                     pixel=mColourMap[mPalette[source&0x0f].Index];
                     *(bitmap_tmp)=(UBYTE)pixel; pixel>>=8;
                     *(bitmap_tmp+1)=(UBYTE)pixel; pixel>>=8;
                     *(bitmap_tmp+2)=(UBYTE)pixel;
                     bitmap_tmp+=mDisplayPitch;
                     pixel=mColourMap[mPalette[source>>4].Index];
                     *(bitmap_tmp)=(UBYTE)pixel; pixel>>=8;
                     *(bitmap_tmp+1)=(UBYTE)pixel; pixel>>=8;
                     *(bitmap_tmp+2)=(UBYTE)pixel;
                     bitmap_tmp+=mDisplayPitch;
                  }
                  else
                  {
                     mLynxAddr++;
                     pixel=mColourMap[mPalette[source>>4].Index];
                     *(bitmap_tmp)=(UBYTE)pixel;    pixel>>=8;
                     *(bitmap_tmp+1)=(UBYTE)pixel; pixel>>=8;
                     *(bitmap_tmp+2)=(UBYTE)pixel;
                     bitmap_tmp+=mDisplayPitch;
                     pixel=mColourMap[mPalette[source&0x0f].Index];
                     *(bitmap_tmp)=(UBYTE)pixel; pixel>>=8;
                     *(bitmap_tmp+1)=(UBYTE)pixel; pixel>>=8;
                     *(bitmap_tmp+2)=(UBYTE)pixel;
                     bitmap_tmp+=mDisplayPitch;
                  }
               }
               mpDisplayCurrent-=3;
            }
            else if(mDisplayFormat==MIKIE_PIXEL_FORMAT_32BPP)
            {
               for(loop=0;loop<SCREEN_WIDTH/2;loop++)
               {
                  source=mpRamPointer[mLynxAddr];
                  if(mDISPCTL_Flip)
                  {
                     mLynxAddr--;
                     *((ULONG*)(bitmap_tmp))=mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp+=mDisplayPitch;
                     *((ULONG*)(bitmap_tmp))=mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp+=mDisplayPitch;
                  }
                  else
                  {
                     mLynxAddr++;
                     *((ULONG*)(bitmap_tmp))=mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp+=mDisplayPitch;
                     *((ULONG*)(bitmap_tmp))=mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp+=mDisplayPitch;
                  }
               }
               mpDisplayCurrent-=sizeof(ULONG);
            }
            break;
         case MIKIE_ROTATE_R:
            if(mDisplayFormat==MIKIE_PIXEL_FORMAT_8BPP)
            {
               for(loop=0;loop<SCREEN_WIDTH/2;loop++)
               {
                  source=mpRamPointer[mLynxAddr];
                  if(mDISPCTL_Flip)
                  {
                     mLynxAddr--;
                     *(bitmap_tmp)=(UBYTE)mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp-=mDisplayPitch;
                     *(bitmap_tmp)=(UBYTE)mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp-=mDisplayPitch;
                  }
                  else
                  {
                     mLynxAddr++;
                     *(bitmap_tmp)=(UBYTE)mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp-=mDisplayPitch;
                     *(bitmap_tmp)=(UBYTE)mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp-=mDisplayPitch;
                  }
               }
               mpDisplayCurrent+=sizeof(UBYTE);
            }
            else if(mDisplayFormat==MIKIE_PIXEL_FORMAT_16BPP_555 || mDisplayFormat==MIKIE_PIXEL_FORMAT_16BPP_565 || mDisplayFormat==MIKIE_PIXEL_FORMAT_16BPP_565_INV)
            {
               for(loop=0;loop<SCREEN_WIDTH/2;loop++)
               {
                  source=mpRamPointer[mLynxAddr];
                  if(mDISPCTL_Flip)
                  {
                     mLynxAddr--;
                     *((UWORD*)(bitmap_tmp))=(UWORD)mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp-=mDisplayPitch;
                     *((UWORD*)(bitmap_tmp))=(UWORD)mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp-=mDisplayPitch;
                  }
                  else
                  {
                     mLynxAddr++;
                     *((UWORD*)(bitmap_tmp))=(UWORD)mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp-=mDisplayPitch;
                     *((UWORD*)(bitmap_tmp))=(UWORD)mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp-=mDisplayPitch;
                  }
               }
               mpDisplayCurrent+=sizeof(UWORD);
            }
            else if(mDisplayFormat==MIKIE_PIXEL_FORMAT_24BPP)
            {
               ULONG pixel;
               for(loop=0;loop<SCREEN_WIDTH/2;loop++)
               {
                  source=mpRamPointer[mLynxAddr];
                  if(mDISPCTL_Flip)
                  {
                     mLynxAddr--;
                     pixel=mColourMap[mPalette[source&0x0f].Index];
                     *(bitmap_tmp)=(UBYTE)pixel; pixel>>=8;
                     *(bitmap_tmp+1)=(UBYTE)pixel; pixel>>=8;
                     *(bitmap_tmp+2)=(UBYTE)pixel;
                     bitmap_tmp-=mDisplayPitch;
                     pixel=mColourMap[mPalette[source>>4].Index];
                     *(bitmap_tmp)=(UBYTE)pixel; pixel>>=8;
                     *(bitmap_tmp+1)=(UBYTE)pixel; pixel>>=8;
                     *(bitmap_tmp+2)=(UBYTE)pixel;
                     bitmap_tmp-=mDisplayPitch;
                  }
                  else
                  {
                     mLynxAddr++;
                     pixel=mColourMap[mPalette[source>>4].Index];
                     *(bitmap_tmp)=(UBYTE)pixel;    pixel>>=8;
                     *(bitmap_tmp+1)=(UBYTE)pixel; pixel>>=8;
                     *(bitmap_tmp+2)=(UBYTE)pixel;
                     bitmap_tmp-=mDisplayPitch;
                     pixel=mColourMap[mPalette[source&0x0f].Index];
                     *(bitmap_tmp)=(UBYTE)pixel; pixel>>=8;
                     *(bitmap_tmp+1)=(UBYTE)pixel; pixel>>=8;
                     *(bitmap_tmp+2)=(UBYTE)pixel;
                     bitmap_tmp-=mDisplayPitch;
                  }
               }
               mpDisplayCurrent+=3;
            }
            else if(mDisplayFormat==MIKIE_PIXEL_FORMAT_32BPP)
            {
               for(loop=0;loop<SCREEN_WIDTH/2;loop++)
               {
                  source=mpRamPointer[mLynxAddr];
                  if(mDISPCTL_Flip)
                  {
                     mLynxAddr--;
                     *((ULONG*)(bitmap_tmp))=mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp-=mDisplayPitch;
                     *((ULONG*)(bitmap_tmp))=mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp-=mDisplayPitch;
                  }
                  else
                  {
                     mLynxAddr++;
                     *((ULONG*)(bitmap_tmp))=mColourMap[mPalette[source>>4].Index];
                     bitmap_tmp-=mDisplayPitch;
                     *((ULONG*)(bitmap_tmp))=mColourMap[mPalette[source&0x0f].Index];
                     bitmap_tmp-=mDisplayPitch;
                  }
               }
               mpDisplayCurrent+=sizeof(ULONG);
            }
            break;
         default:
            break;
      }
   }
   return work_done;