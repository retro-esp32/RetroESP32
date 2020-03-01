   // Stop any further line rendering
   mLynxLineDMACounter=0;
   mLynxLine=mTIM_2_BKUP;

   // Set the timer status flag
   if(mTimerInterruptMask&0x04)
   {
      TRACE_MIKIE0("Update() - TIMER2 IRQ Triggered (Frame Timer)");
      mTimerStatusFlags|=0x04;
      gSystemIRQ=TRUE;  // Added 19/09/06 fix for IRQ issue
   }

   //   TRACE_MIKIE0("Update() - Frame end");
   // Trigger the callback to the display sub-system to render the
   // display and fetch the new pointer to be used for the lynx
   // display buffer for the forthcoming frame
   if(mpDisplayCallback) mpDisplayBits=(*mpDisplayCallback)(mDisplayCallbackObject);

   // Reinitialise the screen buffer pointer
   // Make any necessary adjustment for rotation
   switch(mDisplayRotate)
   {
      case MIKIE_ROTATE_L:
         mpDisplayCurrent=mpDisplayBits;
         switch(mDisplayFormat)
         {
            case MIKIE_PIXEL_FORMAT_8BPP:
               mpDisplayCurrent+=1*(HANDY_SCREEN_HEIGHT-1);
               break;
            case MIKIE_PIXEL_FORMAT_16BPP_555:
            case MIKIE_PIXEL_FORMAT_16BPP_565:
            case MIKIE_PIXEL_FORMAT_16BPP_565_INV:
               mpDisplayCurrent+=2*(HANDY_SCREEN_HEIGHT-1);
               break;
            case MIKIE_PIXEL_FORMAT_24BPP:
               mpDisplayCurrent+=3*(HANDY_SCREEN_HEIGHT-1);
               break;
            case MIKIE_PIXEL_FORMAT_32BPP:
               mpDisplayCurrent+=4*(HANDY_SCREEN_HEIGHT-1);
               break;
            case MIKIE_PIXEL_FORMAT_RAW:
               mpDisplayCurrent+=2*(HANDY_SCREEN_HEIGHT-1);
               break;
            default:
               break;
         }
         break;
      case MIKIE_ROTATE_R:
         mpDisplayCurrent=mpDisplayBits+(mDisplayPitch*(HANDY_SCREEN_WIDTH-1));
         break;
      case MIKIE_NO_ROTATE:
      default:
         mpDisplayCurrent=mpDisplayBits;
         break;
   }
   return 0;