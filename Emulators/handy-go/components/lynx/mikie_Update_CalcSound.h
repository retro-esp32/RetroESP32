SLONG divide;
SLONG decval;
ULONG tmp;
            //
            // Audio 0
            //
            //              if(mAUDIO_0_ENABLE_COUNT && !mAUDIO_0_TIMER_DONE && mAUDIO_0_VOLUME && mAUDIO_0_BKUP)
            if(mAUDIO_0_ENABLE_COUNT && (mAUDIO_0_ENABLE_RELOAD || !mAUDIO_0_TIMER_DONE) && mAUDIO_0_VOLUME && mAUDIO_0_BKUP)
            {
               decval=0;

               if(mAUDIO_0_LINKING==0x07)
               {
                  if(mTIM_7_BORROW_OUT) decval=1;
                  mAUDIO_0_LAST_LINK_CARRY=mTIM_7_BORROW_OUT;
               }
               else
               {
                  // Ordinary clocked mode as opposed to linked mode
                  // 16MHz clock downto 1us == cyclecount >> 4
                  divide=(4+mAUDIO_0_LINKING);
                  decval=(gSystemCycleCount-mAUDIO_0_LAST_COUNT)>>divide;
               }

               if(decval)
               {
                  mAUDIO_0_LAST_COUNT+=decval<<divide;
                  mAUDIO_0_CURRENT-=decval;
                  if(mAUDIO_0_CURRENT&0x80000000)
                  {
                     // Set carry out
                     mAUDIO_0_BORROW_OUT=TRUE;

                     // Reload if neccessary
                     if(mAUDIO_0_ENABLE_RELOAD)
                     {
                        mAUDIO_0_CURRENT+=mAUDIO_0_BKUP+1;
                        if(mAUDIO_0_CURRENT&0x80000000) mAUDIO_0_CURRENT=0;
                     }
                     else
                     {
                        // Set timer done
                        mAUDIO_0_TIMER_DONE=TRUE;
                        mAUDIO_0_CURRENT=0;
                     }

                     //
                     // Update audio circuitry
                     //
                     mAUDIO_0_WAVESHAPER=GetLfsrNext(mAUDIO_0_WAVESHAPER);

                     if(mAUDIO_0_INTEGRATE_ENABLE)
                     {
                                SLONG temp=mAUDIO_OUTPUT_0;
                        if(mAUDIO_0_WAVESHAPER&0x0001) temp+=mAUDIO_0_VOLUME; else temp-=mAUDIO_0_VOLUME;
                        if(temp>127) temp=127;
                        if(temp<-128) temp=-128;
                                mAUDIO_OUTPUT_0=(SBYTE)temp;
                     }
                     else
                     {
                                if(mAUDIO_0_WAVESHAPER&0x0001) mAUDIO_OUTPUT_0=mAUDIO_0_VOLUME; else mAUDIO_OUTPUT_0=-mAUDIO_0_VOLUME;
                     }
                  }
                  else
                  {
                     mAUDIO_0_BORROW_OUT=FALSE;
                  }
                  // Set carry in as we did a count
                  mAUDIO_0_BORROW_IN=TRUE;
               }
               else
               {
                  // Clear carry in as we didn't count
                  mAUDIO_0_BORROW_IN=FALSE;
                  // Clear carry out
                  mAUDIO_0_BORROW_OUT=FALSE;
               }

               // Prediction for next timer event cycle number

               if(mAUDIO_0_LINKING!=7)
               {
                  // Sometimes timeupdates can be >2x rollover in which case
                  // then CURRENT may still be negative and we can use it to
                  // calc the next timer value, we just want another update ASAP
                  tmp=(mAUDIO_0_CURRENT&0x80000000)?1:((mAUDIO_0_CURRENT+1)<<divide);
                  tmp+=gSystemCycleCount;
                  if(tmp<gNextTimerEvent)
                  {
                     gNextTimerEvent=tmp;
                     TRACE_MIKIE1("Update() - AUDIO 0 Set NextTimerEvent = %012d",gNextTimerEvent);
                  }
               }
               //                   TRACE_MIKIE1("Update() - mAUDIO_0_CURRENT = %012d",mAUDIO_0_CURRENT);
               //                   TRACE_MIKIE1("Update() - mAUDIO_0_BKUP    = %012d",mAUDIO_0_BKUP);
               //                   TRACE_MIKIE1("Update() - mAUDIO_0_LASTCNT = %012d",mAUDIO_0_LAST_COUNT);
               //                   TRACE_MIKIE1("Update() - mAUDIO_0_LINKING = %012d",mAUDIO_0_LINKING);
            }

            //
            // Audio 1
            //
            //              if(mAUDIO_1_ENABLE_COUNT && !mAUDIO_1_TIMER_DONE && mAUDIO_1_VOLUME && mAUDIO_1_BKUP)
            if(mAUDIO_1_ENABLE_COUNT && (mAUDIO_1_ENABLE_RELOAD || !mAUDIO_1_TIMER_DONE) && mAUDIO_1_VOLUME && mAUDIO_1_BKUP)
            {
               decval=0;

               if(mAUDIO_1_LINKING==0x07)
               {
                  if(mAUDIO_0_BORROW_OUT) decval=1;
                  mAUDIO_1_LAST_LINK_CARRY=mAUDIO_0_BORROW_OUT;
               }
               else
               {
                  // Ordinary clocked mode as opposed to linked mode
                  // 16MHz clock downto 1us == cyclecount >> 4
                  divide=(4+mAUDIO_1_LINKING);
                  decval=(gSystemCycleCount-mAUDIO_1_LAST_COUNT)>>divide;
               }

               if(decval)
               {
                  mAUDIO_1_LAST_COUNT+=decval<<divide;
                  mAUDIO_1_CURRENT-=decval;
                  if(mAUDIO_1_CURRENT&0x80000000)
                  {
                     // Set carry out
                     mAUDIO_1_BORROW_OUT=TRUE;

                     // Reload if neccessary
                     if(mAUDIO_1_ENABLE_RELOAD)
                     {
                        mAUDIO_1_CURRENT+=mAUDIO_1_BKUP+1;
                        if(mAUDIO_1_CURRENT&0x80000000) mAUDIO_1_CURRENT=0;
                     }
                     else
                     {
                        // Set timer done
                        mAUDIO_1_TIMER_DONE=TRUE;
                        mAUDIO_1_CURRENT=0;
                     }

                     //
                     // Update audio circuitry
                     //
                     mAUDIO_1_WAVESHAPER=GetLfsrNext(mAUDIO_1_WAVESHAPER);

                     if(mAUDIO_1_INTEGRATE_ENABLE)
                     {
                                SLONG temp=mAUDIO_OUTPUT_1;
                        if(mAUDIO_1_WAVESHAPER&0x0001) temp+=mAUDIO_1_VOLUME; else temp-=mAUDIO_1_VOLUME;
                        if(temp>127) temp=127;
                        if(temp<-128) temp=-128;
                                mAUDIO_OUTPUT_1=(SBYTE)temp;
                     }
                     else
                     {
                                if(mAUDIO_1_WAVESHAPER&0x0001) mAUDIO_OUTPUT_1=mAUDIO_1_VOLUME; else mAUDIO_OUTPUT_1=-mAUDIO_1_VOLUME;
                     }
                  }
                  else
                  {
                     mAUDIO_1_BORROW_OUT=FALSE;
                  }
                  // Set carry in as we did a count
                  mAUDIO_1_BORROW_IN=TRUE;
               }
               else
               {
                  // Clear carry in as we didn't count
                  mAUDIO_1_BORROW_IN=FALSE;
                  // Clear carry out
                  mAUDIO_1_BORROW_OUT=FALSE;
               }

               // Prediction for next timer event cycle number

               if(mAUDIO_1_LINKING!=7)
               {
                  // Sometimes timeupdates can be >2x rollover in which case
                  // then CURRENT may still be negative and we can use it to
                  // calc the next timer value, we just want another update ASAP
                  tmp=(mAUDIO_1_CURRENT&0x80000000)?1:((mAUDIO_1_CURRENT+1)<<divide);
                  tmp+=gSystemCycleCount;
                  if(tmp<gNextTimerEvent)
                  {
                     gNextTimerEvent=tmp;
                     TRACE_MIKIE1("Update() - AUDIO 1 Set NextTimerEvent = %012d",gNextTimerEvent);
                  }
               }
               //                   TRACE_MIKIE1("Update() - mAUDIO_1_CURRENT = %012d",mAUDIO_1_CURRENT);
               //                   TRACE_MIKIE1("Update() - mAUDIO_1_BKUP    = %012d",mAUDIO_1_BKUP);
               //                   TRACE_MIKIE1("Update() - mAUDIO_1_LASTCNT = %012d",mAUDIO_1_LAST_COUNT);
               //                   TRACE_MIKIE1("Update() - mAUDIO_1_LINKING = %012d",mAUDIO_1_LINKING);
            }

            //
            // Audio 2
            //
            //              if(mAUDIO_2_ENABLE_COUNT && !mAUDIO_2_TIMER_DONE && mAUDIO_2_VOLUME && mAUDIO_2_BKUP)
            if(mAUDIO_2_ENABLE_COUNT && (mAUDIO_2_ENABLE_RELOAD || !mAUDIO_2_TIMER_DONE) && mAUDIO_2_VOLUME && mAUDIO_2_BKUP)
            {
               decval=0;

               if(mAUDIO_2_LINKING==0x07)
               {
                  if(mAUDIO_1_BORROW_OUT) decval=1;
                  mAUDIO_2_LAST_LINK_CARRY=mAUDIO_1_BORROW_OUT;
               }
               else
               {
                  // Ordinary clocked mode as opposed to linked mode
                  // 16MHz clock downto 1us == cyclecount >> 4
                  divide=(4+mAUDIO_2_LINKING);
                  decval=(gSystemCycleCount-mAUDIO_2_LAST_COUNT)>>divide;
               }

               if(decval)
               {
                  mAUDIO_2_LAST_COUNT+=decval<<divide;
                  mAUDIO_2_CURRENT-=decval;
                  if(mAUDIO_2_CURRENT&0x80000000)
                  {
                     // Set carry out
                     mAUDIO_2_BORROW_OUT=TRUE;

                     // Reload if neccessary
                     if(mAUDIO_2_ENABLE_RELOAD)
                     {
                        mAUDIO_2_CURRENT+=mAUDIO_2_BKUP+1;
                        if(mAUDIO_2_CURRENT&0x80000000) mAUDIO_2_CURRENT=0;
                     }
                     else
                     {
                        // Set timer done
                        mAUDIO_2_TIMER_DONE=TRUE;
                        mAUDIO_2_CURRENT=0;
                     }

                     //
                     // Update audio circuitry
                     //
                     mAUDIO_2_WAVESHAPER=GetLfsrNext(mAUDIO_2_WAVESHAPER);

                     if(mAUDIO_2_INTEGRATE_ENABLE)
                     {
                                SLONG temp=mAUDIO_OUTPUT_2;
                        if(mAUDIO_2_WAVESHAPER&0x0001) temp+=mAUDIO_2_VOLUME; else temp-=mAUDIO_2_VOLUME;
                        if(temp>127) temp=127;
                        if(temp<-128) temp=-128;
                                mAUDIO_OUTPUT_2=(SBYTE)temp;
                     }
                     else
                     {
                                if(mAUDIO_2_WAVESHAPER&0x0001) mAUDIO_OUTPUT_2=mAUDIO_2_VOLUME; else mAUDIO_OUTPUT_2=-mAUDIO_2_VOLUME;
                     }
                  }
                  else
                  {
                     mAUDIO_2_BORROW_OUT=FALSE;
                  }
                  // Set carry in as we did a count
                  mAUDIO_2_BORROW_IN=TRUE;
               }
               else
               {
                  // Clear carry in as we didn't count
                  mAUDIO_2_BORROW_IN=FALSE;
                  // Clear carry out
                  mAUDIO_2_BORROW_OUT=FALSE;
               }

               // Prediction for next timer event cycle number

               if(mAUDIO_2_LINKING!=7)
               {
                  // Sometimes timeupdates can be >2x rollover in which case
                  // then CURRENT may still be negative and we can use it to
                  // calc the next timer value, we just want another update ASAP
                  tmp=(mAUDIO_2_CURRENT&0x80000000)?1:((mAUDIO_2_CURRENT+1)<<divide);
                  tmp+=gSystemCycleCount;
                  if(tmp<gNextTimerEvent)
                  {
                     gNextTimerEvent=tmp;
                     TRACE_MIKIE1("Update() - AUDIO 2 Set NextTimerEvent = %012d",gNextTimerEvent);
                  }
               }
               //                   TRACE_MIKIE1("Update() - mAUDIO_2_CURRENT = %012d",mAUDIO_2_CURRENT);
               //                   TRACE_MIKIE1("Update() - mAUDIO_2_BKUP    = %012d",mAUDIO_2_BKUP);
               //                   TRACE_MIKIE1("Update() - mAUDIO_2_LASTCNT = %012d",mAUDIO_2_LAST_COUNT);
               //                   TRACE_MIKIE1("Update() - mAUDIO_2_LINKING = %012d",mAUDIO_2_LINKING);
            }

            //
            // Audio 3
            //
            //              if(mAUDIO_3_ENABLE_COUNT && !mAUDIO_3_TIMER_DONE && mAUDIO_3_VOLUME && mAUDIO_3_BKUP)
            if(mAUDIO_3_ENABLE_COUNT && (mAUDIO_3_ENABLE_RELOAD || !mAUDIO_3_TIMER_DONE) && mAUDIO_3_VOLUME && mAUDIO_3_BKUP)
            {
               decval=0;

               if(mAUDIO_3_LINKING==0x07)
               {
                  if(mAUDIO_2_BORROW_OUT) decval=1;
                  mAUDIO_3_LAST_LINK_CARRY=mAUDIO_2_BORROW_OUT;
               }
               else
               {
                  // Ordinary clocked mode as opposed to linked mode
                  // 16MHz clock downto 1us == cyclecount >> 4
                  divide=(4+mAUDIO_3_LINKING);
                  decval=(gSystemCycleCount-mAUDIO_3_LAST_COUNT)>>divide;
               }

               if(decval)
               {
                  mAUDIO_3_LAST_COUNT+=decval<<divide;
                  mAUDIO_3_CURRENT-=decval;
                  if(mAUDIO_3_CURRENT&0x80000000)
                  {
                     // Set carry out
                     mAUDIO_3_BORROW_OUT=TRUE;

                     // Reload if neccessary
                     if(mAUDIO_3_ENABLE_RELOAD)
                     {
                        mAUDIO_3_CURRENT+=mAUDIO_3_BKUP+1;
                        if(mAUDIO_3_CURRENT&0x80000000) mAUDIO_3_CURRENT=0;
                     }
                     else
                     {
                        // Set timer done
                        mAUDIO_3_TIMER_DONE=TRUE;
                        mAUDIO_3_CURRENT=0;
                     }

                     //
                     // Update audio circuitry
                     //
                     mAUDIO_3_WAVESHAPER=GetLfsrNext(mAUDIO_3_WAVESHAPER);

                     if(mAUDIO_3_INTEGRATE_ENABLE)
                     {
                                SLONG temp=mAUDIO_OUTPUT_3;
                        if(mAUDIO_3_WAVESHAPER&0x0001) temp+=mAUDIO_3_VOLUME; else temp-=mAUDIO_3_VOLUME;
                        if(temp>127) temp=127;
                        if(temp<-128) temp=-128;
                                mAUDIO_OUTPUT_3=(SBYTE)temp;
                     }
                     else
                     {
                                if(mAUDIO_3_WAVESHAPER&0x0001) mAUDIO_OUTPUT_3=mAUDIO_3_VOLUME; else mAUDIO_OUTPUT_3=-mAUDIO_3_VOLUME;
                     }
                  }
                  else
                  {
                     mAUDIO_3_BORROW_OUT=FALSE;
                  }
                  // Set carry in as we did a count
                  mAUDIO_3_BORROW_IN=TRUE;
               }
               else
               {
                  // Clear carry in as we didn't count
                  mAUDIO_3_BORROW_IN=FALSE;
                  // Clear carry out
                  mAUDIO_3_BORROW_OUT=FALSE;
               }

               // Prediction for next timer event cycle number

               if(mAUDIO_3_LINKING!=7)
               {
                  // Sometimes timeupdates can be >2x rollover in which case
                  // then CURRENT may still be negative and we can use it to
                  // calc the next timer value, we just want another update ASAP
                  tmp=(mAUDIO_3_CURRENT&0x80000000)?1:((mAUDIO_3_CURRENT+1)<<divide);
                  tmp+=gSystemCycleCount;
                  if(tmp<gNextTimerEvent)
                  {
                     gNextTimerEvent=tmp;
                     TRACE_MIKIE1("Update() - AUDIO 3 Set NextTimerEvent = %012d",gNextTimerEvent);
                  }
               }
               //                   TRACE_MIKIE1("Update() - mAUDIO_3_CURRENT = %012d",mAUDIO_3_CURRENT);
               //                   TRACE_MIKIE1("Update() - mAUDIO_3_BKUP    = %012d",mAUDIO_3_BKUP);
               //                   TRACE_MIKIE1("Update() - mAUDIO_3_LASTCNT = %012d",mAUDIO_3_LAST_COUNT);
               //                   TRACE_MIKIE1("Update() - mAUDIO_3_LINKING = %012d",mAUDIO_3_LINKING);
            }