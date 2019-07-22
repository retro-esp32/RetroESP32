#ifndef MY_AUDIO_MODE
{
          int count = (gSystemCycleCount-gAudioLastUpdateCycle)/HANDY_AUDIO_SAMPLE_PERIOD;
          if (count == 0) return;
    
          gAudioLastUpdateCycle+=count*HANDY_AUDIO_SAMPLE_PERIOD;
          
          int cur_lsample = 0;
          int cur_rsample = 0;
          int x;
          
          AUDIO_CALC(0, mAUDIO_OUTPUT_0, mAUDIO_ATTEN_0)
          AUDIO_CALC(1, mAUDIO_OUTPUT_1, mAUDIO_ATTEN_1)
          AUDIO_CALC(2, mAUDIO_OUTPUT_2, mAUDIO_ATTEN_2)
          AUDIO_CALC(3, mAUDIO_OUTPUT_3, mAUDIO_ATTEN_3)
/*
          for(x = 0; x < 4; x++){
              /// Assumption (seems there is no documentation for the Attenuation registers)
              /// a) they are linear from $0 to $f - checked!
              /// b) an attenuation of $0 is equal to channel OFF (bits in mSTEREO not set) - checked!
              /// c) an attenuation of $f is NOT equal to no attenuation (bits in PAN not set), $10 would be - checked!
              /// These assumptions can only checked with an oszilloscope... - done
              /// the values stored in mSTEREO are bit-inverted ...
              /// mSTEREO was found to be set like that already (why?), but unused

              if(!(mSTEREO & (0x10 << x)))
              {
                if(mPAN & (0x10 << x))
                  cur_lsample += (mAUDIO_OUTPUT[x]*(mAUDIO_ATTEN[x]&0xF0))/(16*16); /// NOT /15*16 see remark above
                else
                  cur_lsample += mAUDIO_OUTPUT[x];
              }
              if(!(mSTEREO & (0x01 << x)))
              {
                if(mPAN & (0x01 << x))
                  cur_rsample += (mAUDIO_OUTPUT[x]*(mAUDIO_ATTEN[x]&0x0F))/16; /// NOT /15 see remark above
                else
                  cur_rsample += mAUDIO_OUTPUT[x];
              }
            }
            */

        // Upsample to 16 bit signed
        SWORD sample_l, sample_r;
        sample_l= (cur_lsample<<5); // koennte auch 6 sein
        sample_r= (cur_rsample<<5); // keep cool
        
        
                                
            for(;gAudioLastUpdateCycle+HANDY_AUDIO_SAMPLE_PERIOD<gSystemCycleCount;gAudioLastUpdateCycle+=HANDY_AUDIO_SAMPLE_PERIOD)
            {
               // Output audio sample
                // Stereo 16 bit signed
                    *(SWORD *) &(gAudioBuffer[gAudioBufferPointer])=sample_l;
                    *(SWORD *) &(gAudioBuffer[gAudioBufferPointer+2])=sample_r;
                    gAudioBufferPointer+=4;

                // Check buffer overflow condition, stick at the endpoint
               // teh audio output system will reset the input pointer
               // when it reads out the data.

               // We should NEVER overflow, this buffer holds 0.25 seconds
               // of data if this happens the the multimedia system above
               // has failed so the corruption of the buffer contents wont matter

               gAudioBufferPointer%=HANDY_AUDIO_BUFFER_SIZE;
            }
}
/*
inline void CMikie::UpdateSound(void)
{
          int cur_lsample = 0;
          int cur_rsample = 0;
                                int x;

          for(x = 0; x < 4; x++){
              /// Assumption (seems there is no documentation for the Attenuation registers)
              /// a) they are linear from $0 to $f - checked!
              /// b) an attenuation of $0 is equal to channel OFF (bits in mSTEREO not set) - checked!
              /// c) an attenuation of $f is NOT equal to no attenuation (bits in PAN not set), $10 would be - checked!
              /// These assumptions can only checked with an oszilloscope... - done
              /// the values stored in mSTEREO are bit-inverted ...
              /// mSTEREO was found to be set like that already (why?), but unused

              if(!(mSTEREO & (0x10 << x)))
              {
                if(mPAN & (0x10 << x))
                  cur_lsample += (mAUDIO_OUTPUT[x]*(mAUDIO_ATTEN[x]&0xF0))/(16*16); /// NOT /15*16 see remark above
                else
                  cur_lsample += mAUDIO_OUTPUT[x];
              }
              if(!(mSTEREO & (0x01 << x)))
              {
                if(mPAN & (0x01 << x))
                  cur_rsample += (mAUDIO_OUTPUT[x]*(mAUDIO_ATTEN[x]&0x0F))/16; /// NOT /15 see remark above
                else
                  cur_rsample += mAUDIO_OUTPUT[x];
              }
            }

        // Upsample to 16 bit signed
        SWORD sample_l, sample_r;
        sample_l= (cur_lsample<<5); // koennte auch 6 sein
        sample_r= (cur_rsample<<5); // keep cool
        
        
                                
            for(;gAudioLastUpdateCycle+HANDY_AUDIO_SAMPLE_PERIOD<gSystemCycleCount;gAudioLastUpdateCycle+=HANDY_AUDIO_SAMPLE_PERIOD)
            {
               // Output audio sample
                // Stereo 16 bit signed
                    *(SWORD *) &(gAudioBuffer[gAudioBufferPointer])=sample_l;
                    *(SWORD *) &(gAudioBuffer[gAudioBufferPointer+2])=sample_r;
                    gAudioBufferPointer+=4;

                // Check buffer overflow condition, stick at the endpoint
               // teh audio output system will reset the input pointer
               // when it reads out the data.

               // We should NEVER overflow, this buffer holds 0.25 seconds
               // of data if this happens the the multimedia system above
               // has failed so the corruption of the buffer contents wont matter

               gAudioBufferPointer%=HANDY_AUDIO_BUFFER_SIZE;
            }
}
*/
#else
#if MY_AUDIO_MODE==1

{
    int count = (gSystemCycleCount-gAudioLastUpdateCycle)/HANDY_AUDIO_SAMPLE_PERIOD;
    if (count == 0) return;
    
    gAudioLastUpdateCycle+=count*HANDY_AUDIO_SAMPLE_PERIOD;
    
          int cur_lsample = 0;
          int cur_rsample = 0;
                                int x;

          for(x = 0; x < 4; x++){
              /// Assumption (seems there is no documentation for the Attenuation registers)
              /// a) they are linear from $0 to $f - checked!
              /// b) an attenuation of $0 is equal to channel OFF (bits in mSTEREO not set) - checked!
              /// c) an attenuation of $f is NOT equal to no attenuation (bits in PAN not set), $10 would be - checked!
              /// These assumptions can only checked with an oszilloscope... - done
              /// the values stored in mSTEREO are bit-inverted ...
              /// mSTEREO was found to be set like that already (why?), but unused

              if(!(mSTEREO & (0x10 << x)))
              {
                if(mPAN & (0x10 << x))
                  cur_lsample += (mAUDIO_OUTPUT[x]*(mAUDIO_ATTEN[x]&0xF0))/(16*16); /// NOT /15*16 see remark above
                else
                  cur_lsample += mAUDIO_OUTPUT[x];
              }
              if(!(mSTEREO & (0x01 << x)))
              {
                if(mPAN & (0x01 << x))
                  cur_rsample += (mAUDIO_OUTPUT[x]*(mAUDIO_ATTEN[x]&0x0F))/16; /// NOT /15 see remark above
                else
                  cur_rsample += mAUDIO_OUTPUT[x];
              }
            }

       
        // Upsample to 16 bit signed
        SWORD sample_l, sample_r;
        sample_l= (cur_lsample<<5); // koennte auch 6 sein
        sample_r= (cur_rsample<<5); // keep cool
        ULONG value = (((ULONG)sample_l) << 16) | ((ULONG)sample_r)&0x0000FFFF;
        
        if (gAudioBufferPointer + count * 4>=HANDY_AUDIO_BUFFER_SIZE) {
            if (count * 4>=HANDY_AUDIO_BUFFER_SIZE) {
               count = HANDY_AUDIO_BUFFER_SIZE/4 -1;
            }
            gAudioBufferPointer = 0;
            gAudioBufferPointer2 = gAudioBuffer;
       }
       gAudioBufferPointer+=4*count;
       
       // 16bit
       switch (count) {
       case 4:
        *(ULONG *) gAudioBufferPointer2=value;
        *(ULONG *) (gAudioBufferPointer2+2)=value;
        *(ULONG *) (gAudioBufferPointer2+4)=value;
        *(ULONG *) (gAudioBufferPointer2+6)=value;
        gAudioBufferPointer2+=8;
        break;
       case 3:
        *(ULONG *) gAudioBufferPointer2=value;
        *(ULONG *) (gAudioBufferPointer2+2)=value;
        *(ULONG *) (gAudioBufferPointer2+4)=value;
        gAudioBufferPointer2+=6;
        break;
       case 2:
        *(ULONG *) gAudioBufferPointer2=value;
        *(ULONG *) (gAudioBufferPointer2+2)=value;
        gAudioBufferPointer2+=4;
        break;
       case 1:
        *(ULONG *) gAudioBufferPointer2=value;
        gAudioBufferPointer2+=2;
        break;
       default:
         for(int i = 0;i < count; i++)
            {
                *(ULONG *) gAudioBufferPointer2=value; gAudioBufferPointer2+=2;
            }
       break;
       }
       
       
       /*
       // 8bit
       switch (count) {
       case 4:
        *(ULONG *) gAudioBufferPointer2=value;
        *(ULONG *) (gAudioBufferPointer2+4)=value;
        *(ULONG *) (gAudioBufferPointer2+8)=value;
        *(ULONG *) (gAudioBufferPointer2+12)=value;
        gAudioBufferPointer2+=16;
        break;
       case 3:
        *(ULONG *) gAudioBufferPointer2=value;
        *(ULONG *) (gAudioBufferPointer2+4)=value;
        *(ULONG *) (gAudioBufferPointer2+8)=value;
        gAudioBufferPointer2+=12;
        break;
       case 2:
        *(ULONG *) gAudioBufferPointer2=value;
        *(ULONG *) (gAudioBufferPointer2+4)=value;
        gAudioBufferPointer2+=8;
        break;
       case 1:
        *(ULONG *) gAudioBufferPointer2=value;
        gAudioBufferPointer2+=4;
        break;
       default:
         for(int i = 0;i < count; i++)
            {
                *(ULONG *) gAudioBufferPointer2=value; gAudioBufferPointer2+=4;
            }
       break;
       }
       */
       /*
       switch (count) {
       case 4:
        *(ULONG *) gAudioBufferPointer2=value; gAudioBufferPointer2+=4;
       case 3:
        *(ULONG *) gAudioBufferPointer2=value; gAudioBufferPointer2+=4;
       case 2:
        *(ULONG *) gAudioBufferPointer2=value; gAudioBufferPointer2+=4;
       case 1:
        *(ULONG *) gAudioBufferPointer2=value; gAudioBufferPointer2+=4;
        break;
       default:
         for(int i = 0;i < count; i++)
            {
                *(ULONG *) gAudioBufferPointer2=value; gAudioBufferPointer2+=4;
            }
       break;
       }
       */           
}
#endif
#if MY_AUDIO_MODE==2

{
          int count = (gSystemCycleCount-gAudioLastUpdateCycle)/HANDY_AUDIO_SAMPLE_PERIOD;
          if (count == 0) return;
    
          gAudioLastUpdateCycle+=count*HANDY_AUDIO_SAMPLE_PERIOD;
          
          int cur_lsample = 0;
          int cur_rsample = 0;
          int x;
          
          AUDIO_CALC(0, mAUDIO_OUTPUT_0, mAUDIO_ATTEN_0)
          AUDIO_CALC(1, mAUDIO_OUTPUT_1, mAUDIO_ATTEN_1)
          AUDIO_CALC(2, mAUDIO_OUTPUT_2, mAUDIO_ATTEN_2)
          AUDIO_CALC(3, mAUDIO_OUTPUT_3, mAUDIO_ATTEN_3)

        // Upsample to 16 bit signed
        SWORD sample_l, sample_r;
        sample_l= (cur_lsample<<5); // koennte auch 6 sein
        sample_r= (cur_rsample<<5); // keep cool
        ULONG value = (((ULONG)sample_l) << 16) | ((ULONG)sample_r)&0x0000FFFF;
        
            for(;count>0;count--)                    
            {
               // Output audio sample
                // Stereo 16 bit signed
                //*(SWORD *) &(gAudioBuffer[gAudioBufferPointer])=sample_l;
                //*(SWORD *) &(gAudioBuffer[gAudioBufferPointer+2])=sample_r;
                *(ULONG *) &(gAudioBuffer[gAudioBufferPointer])=value;
                gAudioBufferPointer+=4;

                // Check buffer overflow condition, stick at the endpoint
               // teh audio output system will reset the input pointer
               // when it reads out the data.

               // We should NEVER overflow, this buffer holds 0.25 seconds
               // of data if this happens the the multimedia system above
               // has failed so the corruption of the buffer contents wont matter

               gAudioBufferPointer%=HANDY_AUDIO_BUFFER_SIZE;
            }
}

#endif
#endif