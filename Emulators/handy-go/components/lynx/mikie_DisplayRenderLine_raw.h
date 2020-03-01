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
            //mDisplayFormat==MIKIE_PIXEL_FORMAT_RAW)
            {
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
            mpDisplayCurrent+=mDisplayPitch;
            break;
         case MIKIE_ROTATE_L:
            break;
         case MIKIE_ROTATE_R:
            break;
         default:
            break;
      }
   }
   return work_done;