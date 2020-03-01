         SLONG divide = 0;
         SLONG decval;
         ULONG tmp;
         ULONG mikie_work_done=0;

         //
         // To stop problems with cycle count wrap we will check and then correct the
         // cycle counter.
         //

         //         TRACE_MIKIE0("Update()");

         if(gSystemCycleCount>0xf0000000)
         {
            gSystemCycleCount-=0x80000000;
            gThrottleNextCycleCheckpoint-=0x80000000;
            gAudioLastUpdateCycle-=0x80000000;
            mTIM_0_LAST_COUNT-=0x80000000;
            mTIM_1_LAST_COUNT-=0x80000000;
            mTIM_2_LAST_COUNT-=0x80000000;
            mTIM_3_LAST_COUNT-=0x80000000;
            mTIM_4_LAST_COUNT-=0x80000000;
            mTIM_5_LAST_COUNT-=0x80000000;
            mTIM_6_LAST_COUNT-=0x80000000;
            mTIM_7_LAST_COUNT-=0x80000000;
            mAUDIO_0_LAST_COUNT-=0x80000000;
            mAUDIO_1_LAST_COUNT-=0x80000000;
            mAUDIO_2_LAST_COUNT-=0x80000000;
            mAUDIO_3_LAST_COUNT-=0x80000000;
            // Only correct if sleep is active
            if(gCPUWakeupTime)
            {
               gCPUWakeupTime-=0x80000000;
               gIRQEntryCycle-=0x80000000;
            }
         }

         gNextTimerEvent=0xffffffff;

         //
         // Check if the CPU needs to be woken up from sleep mode
         //
         if(gCPUWakeupTime)
         {
            if(gSystemCycleCount>=gCPUWakeupTime)
            {
               TRACE_MIKIE0("*********************************************************");
               TRACE_MIKIE0("****              CPU SLEEP COMPLETED                ****");
               TRACE_MIKIE0("*********************************************************");
               ClearCPUSleep();
               gCPUWakeupTime=0;
            }
            else
            {
               if(gCPUWakeupTime>gSystemCycleCount) gNextTimerEvent=gCPUWakeupTime;
            }
         }

         // Timer updates, rolled out flat in group order
         //
         // Group A:
         // Timer 0 -> Timer 2 -> Timer 4.
         //
         // Group B:
         // Timer 1 -> Timer 3 -> Timer 5 -> Timer 7 -> Audio 0 -> Audio 1-> Audio 2 -> Audio 3 -> Timer 1.
         //

         //
         // Within each timer code block we will predict the cycle count number of
         // the next timer event
         //
         // We don't need to count linked timers as the timer they are linked
         // from will always generate earlier events.
         //
         // As Timer 4 (UART) will generate many events we will ignore it
         //
         // We set the next event to the end of time at first and let the timers
         // overload it. Any writes to timer controls will force next event to
         // be immediate and hence a new preidction will be done. The prediction
         // causes overflow as opposed to zero i.e. current+1
         // (In reality T0 line counter should always be running.)
         //


         //
         // Timer 0 of Group A
         //

         //
         // Optimisation, assume T0 (Line timer) is never in one-shot,
         // never placed in link mode
         //

         // KW bugfix 13/4/99 added (mTIM_x_ENABLE_RELOAD ||  ..)
         //         if(mTIM_0_ENABLE_COUNT && (mTIM_0_ENABLE_RELOAD || !mTIM_0_TIMER_DONE))
         if(mTIM_0_ENABLE_COUNT)
         {
            // Timer 0 has no linking
            //              if(mTIM_0_LINKING!=0x07)
            {
               // Ordinary clocked mode as opposed to linked mode
               // 16MHz clock downto 1us == cyclecount >> 4
               divide=(4+mTIM_0_LINKING);
               decval=(gSystemCycleCount-mTIM_0_LAST_COUNT)>>divide;

               if(decval)
               {
                  mTIM_0_LAST_COUNT+=decval<<divide;
                  mTIM_0_CURRENT-=decval;

                  if(mTIM_0_CURRENT&0x80000000)
                  {
                     // Set carry out
                     mTIM_0_BORROW_OUT=TRUE;

                     //                         // Reload if neccessary
                     //                         if(mTIM_0_ENABLE_RELOAD)
                     //                         {
                     mTIM_0_CURRENT+=mTIM_0_BKUP+1;
                     //                         }
                     //                         else
                     //                         {
                     //                             mTIM_0_CURRENT=0;
                     //                         }

                     mTIM_0_TIMER_DONE=TRUE;

                     // Interupt flag setting code moved into DisplayRenderLine()

                     // Line timer has expired, render a line, we cannot incrememnt
                     // the global counter at this point as it will screw the other timers
                     // so we save under work done and inc at the end.
                     mikie_work_done+=DisplayRenderLine();

                  }
                  else
                  {
                     mTIM_0_BORROW_OUT=FALSE;
                  }
                  // Set carry in as we did a count
                  mTIM_0_BORROW_IN=TRUE;
               }
               else
               {
                  // Clear carry in as we didn't count
                  mTIM_0_BORROW_IN=FALSE;
                  // Clear carry out
                  mTIM_0_BORROW_OUT=FALSE;
               }
            }

            // Prediction for next timer event cycle number

            //              if(mTIM_0_LINKING!=7)
            {
               // Sometimes timeupdates can be >2x rollover in which case
               // then CURRENT may still be negative and we can use it to
               // calc the next timer value, we just want another update ASAP
               tmp=(mTIM_0_CURRENT&0x80000000)?1:((mTIM_0_CURRENT+1)<<divide);
               tmp+=gSystemCycleCount;
               if(tmp<gNextTimerEvent)
               {
                  gNextTimerEvent=tmp;
                  //                        TRACE_MIKIE1("Update() - TIMER 0 Set NextTimerEvent = %012d",gNextTimerEvent);
               }
            }
            //              TRACE_MIKIE1("Update() - mTIM_0_CURRENT = %012d",mTIM_0_CURRENT);
            //              TRACE_MIKIE1("Update() - mTIM_0_BKUP    = %012d",mTIM_0_BKUP);
            //              TRACE_MIKIE1("Update() - mTIM_0_LASTCNT = %012d",mTIM_0_LAST_COUNT);
            //              TRACE_MIKIE1("Update() - mTIM_0_LINKING = %012d",mTIM_0_LINKING);
         }

         //
         // Timer 2 of Group A
         //

         //
         // Optimisation, assume T2 (Frame timer) is never in one-shot
         // always in linked mode i.e clocked by Line Timer
         //

         // KW bugfix 13/4/99 added (mTIM_x_ENABLE_RELOAD ||  ..)
         //         if(mTIM_2_ENABLE_COUNT && (mTIM_2_ENABLE_RELOAD || !mTIM_2_TIMER_DONE))
         if(mTIM_2_ENABLE_COUNT)
         {
            decval=0;

            //              if(mTIM_2_LINKING==0x07)
            {
               if(mTIM_0_BORROW_OUT) decval=1;
               mTIM_2_LAST_LINK_CARRY=mTIM_0_BORROW_OUT;
            }
            //              else
            //              {
            //                  // Ordinary clocked mode as opposed to linked mode
            //                  // 16MHz clock downto 1us == cyclecount >> 4
            //                  divide=(4+mTIM_2_LINKING);
            //                  decval=(gSystemCycleCount-mTIM_2_LAST_COUNT)>>divide;
            //              }

            if(decval)
            {
               //                   mTIM_2_LAST_COUNT+=decval<<divide;
               mTIM_2_CURRENT-=decval;
               if(mTIM_2_CURRENT&0x80000000)
               {
                  // Set carry out
                  mTIM_2_BORROW_OUT=TRUE;

                  //                        // Reload if neccessary
                  //                        if(mTIM_2_ENABLE_RELOAD)
                  //                        {
                  mTIM_2_CURRENT+=mTIM_2_BKUP+1;
                  //                        }
                  //                        else
                  //                        {
                  //                            mTIM_2_CURRENT=0;
                  //                        }
                  mTIM_2_TIMER_DONE=TRUE;

                  // Interupt flag setting code moved into DisplayEndOfFrame(), also
                  // park any CPU cycles lost for later inclusion
                  mikie_work_done+=DisplayEndOfFrame();
               }
               else
               {
                  mTIM_2_BORROW_OUT=FALSE;
               }
               // Set carry in as we did a count
               mTIM_2_BORROW_IN=TRUE;
            }
            else
            {
               // Clear carry in as we didn't count
               mTIM_2_BORROW_IN=FALSE;
               // Clear carry out
               mTIM_2_BORROW_OUT=FALSE;
            }

            // Prediction for next timer event cycle number
            // We dont need to predict this as its the frame timer and will always
            // be beaten by the line timer on Timer 0
            //              if(mTIM_2_LINKING!=7)
            //              {
            //                  tmp=gSystemCycleCount+((mTIM_2_CURRENT+1)<<divide);
            //                  if(tmp<gNextTimerEvent) gNextTimerEvent=tmp;
            //              }
            //              TRACE_MIKIE1("Update() - mTIM_2_CURRENT = %012d",mTIM_2_CURRENT);
            //              TRACE_MIKIE1("Update() - mTIM_2_BKUP    = %012d",mTIM_2_BKUP);
            //              TRACE_MIKIE1("Update() - mTIM_2_LASTCNT = %012d",mTIM_2_LAST_COUNT);
            //              TRACE_MIKIE1("Update() - mTIM_2_LINKING = %012d",mTIM_2_LINKING);
         }

         //
         // Timer 4 of Group A
         //
         // For the sake of speed it is assumed that Timer 4 (UART timer)
         // never uses one-shot mode, never uses linking, hence the code
         // is commented out. Timer 4 is at the end of a chain and seems
         // no reason to update its carry in-out variables
         //

         // KW bugfix 13/4/99 added (mTIM_x_ENABLE_RELOAD ||  ..)
         //         if(mTIM_4_ENABLE_COUNT && (mTIM_4_ENABLE_RELOAD || !mTIM_4_TIMER_DONE))
         if(mTIM_4_ENABLE_COUNT)
         {
            decval=0;

            //              if(mTIM_4_LINKING==0x07)
            //              {
            ////                if(mTIM_2_BORROW_OUT && !mTIM_4_LAST_LINK_CARRY) decval=1;
            //                  if(mTIM_2_BORROW_OUT) decval=1;
            //                  mTIM_4_LAST_LINK_CARRY=mTIM_2_BORROW_OUT;
            //              }
            //              else
            {
               // Ordinary clocked mode as opposed to linked mode
               // 16MHz clock downto 1us == cyclecount >> 4
               // Additional /8 (+3) for 8 clocks per bit transmit
               divide=4+3+mTIM_4_LINKING;
               decval=(gSystemCycleCount-mTIM_4_LAST_COUNT)>>divide;
            }

            if(decval)
            {
               mTIM_4_LAST_COUNT+=decval<<divide;
               mTIM_4_CURRENT-=decval;
               if(mTIM_4_CURRENT&0x80000000)
               {
                  // Set carry out
                  mTIM_4_BORROW_OUT=TRUE;

                  //
                  // Update the UART counter models for Rx & Tx
                  //

                  //
                  // According to the docs IRQ's are level triggered and hence will always assert
                  // what a pain in the arse
                  //
                  // Rx & Tx are loopedback due to comlynx structure

                  //
                  // Receive
                  //
                  if(!mUART_RX_COUNTDOWN)
                  {
                     // Fetch a byte from the input queue
                     if(mUART_Rx_waiting>0)
                     {
                        mUART_RX_DATA=mUART_Rx_input_queue[mUART_Rx_output_ptr];
                        mUART_Rx_output_ptr=(++mUART_Rx_output_ptr)%UART_MAX_RX_QUEUE;
                        mUART_Rx_waiting--;
                        TRACE_MIKIE2("Update() - RX Byte output ptr=%02d waiting=%02d",mUART_Rx_output_ptr,mUART_Rx_waiting);
                     }
                     else
                     {
                        TRACE_MIKIE0("Update() - RX Byte but no data waiting ????");
                     }

                     // Retrigger input if more bytes waiting
                     if(mUART_Rx_waiting>0)
                     {
                        mUART_RX_COUNTDOWN=UART_RX_TIME_PERIOD+UART_RX_NEXT_DELAY;
                        TRACE_MIKIE1("Update() - RX Byte retriggered, %d waiting",mUART_Rx_waiting);
                     }
                     else
                     {
                        mUART_RX_COUNTDOWN=UART_RX_INACTIVE;
                        TRACE_MIKIE0("Update() - RX Byte nothing waiting, deactivated");
                     }

                     // If RX_READY already set then we have an overrun
                     // as previous byte hasnt been read
                     if(mUART_RX_READY) mUART_Rx_overun_error=1;

                     // Flag byte as being recvd
                     mUART_RX_READY=1;
                  }
                  else if(!(mUART_RX_COUNTDOWN&UART_RX_INACTIVE))
                  {
                     mUART_RX_COUNTDOWN--;
                  }

                  if(!mUART_TX_COUNTDOWN)
                  {
                     if(mUART_SENDBREAK)
                     {
                        mUART_TX_DATA=UART_BREAK_CODE;
                        // Auto-Respawn new transmit
                        mUART_TX_COUNTDOWN=UART_TX_TIME_PERIOD;
                        // Loop back what we transmitted
                        ComLynxTxLoopback(mUART_TX_DATA);
                     }
                     else
                     {
                        // Serial activity finished
                        mUART_TX_COUNTDOWN=UART_TX_INACTIVE;
                     }

                     // If a networking object is attached then use its callback to send the data byte.
                     if(mpUART_TX_CALLBACK)
                     {
                        TRACE_MIKIE0("Update() - UART_TX_CALLBACK");
                        (*mpUART_TX_CALLBACK)(mUART_TX_DATA,mUART_TX_CALLBACK_OBJECT);
                     }

                  }
                  else if(!(mUART_TX_COUNTDOWN&UART_TX_INACTIVE))
                  {
                     mUART_TX_COUNTDOWN--;
                  }

                  // Set the timer status flag
                  // Timer 4 is the uart timer and doesn't generate IRQ's using this method

                  // 16 Clocks = 1 bit transmission. Hold separate Rx & Tx counters

                  // Reload if neccessary
                  //                        if(mTIM_4_ENABLE_RELOAD)
                  //                        {
                  mTIM_4_CURRENT+=mTIM_4_BKUP+1;
                  // The low reload values on TIM4 coupled with a longer
                  // timer service delay can sometimes cause
                  // an underun, check and fix
                  if(mTIM_4_CURRENT&0x80000000)
                  {
                     mTIM_4_CURRENT=mTIM_4_BKUP;
                     mTIM_4_LAST_COUNT=gSystemCycleCount;
                  }
                  //                        }
                  //                        else
                  //                        {
                  //                            mTIM_4_CURRENT=0;
                  //                        }
                  //                        mTIM_4_TIMER_DONE=TRUE;
               }
               //                   else
               //                   {
               //                       mTIM_4_BORROW_OUT=FALSE;
               //                   }
               //                   // Set carry in as we did a count
               //                   mTIM_4_BORROW_IN=TRUE;
            }
            //              else
            //              {
            //                  // Clear carry in as we didn't count
            //                  mTIM_4_BORROW_IN=FALSE;
            //                  // Clear carry out
            //                  mTIM_4_BORROW_OUT=FALSE;
            //              }
            //
            //              // Prediction for next timer event cycle number
            //
            //              if(mTIM_4_LINKING!=7)
            //              {
            // Sometimes timeupdates can be >2x rollover in which case
            // then CURRENT may still be negative and we can use it to
            // calc the next timer value, we just want another update ASAP
            tmp=(mTIM_4_CURRENT&0x80000000)?1:((mTIM_4_CURRENT+1)<<divide);
            tmp+=gSystemCycleCount;
            if(tmp<gNextTimerEvent)
            {
               gNextTimerEvent=tmp;
               TRACE_MIKIE1("Update() - TIMER 4 Set NextTimerEvent = %012d",gNextTimerEvent);
            }
            //              }
            //              TRACE_MIKIE1("Update() - mTIM_4_CURRENT = %012d",mTIM_4_CURRENT);
            //              TRACE_MIKIE1("Update() - mTIM_4_BKUP    = %012d",mTIM_4_BKUP);
            //              TRACE_MIKIE1("Update() - mTIM_4_LASTCNT = %012d",mTIM_4_LAST_COUNT);
            //              TRACE_MIKIE1("Update() - mTIM_4_LINKING = %012d",mTIM_4_LINKING);
         }

         // Emulate the UART bug where UART IRQ is level sensitive
         // in that it will continue to generate interrupts as long
         // as they are enabled and the interrupt condition is true

         // If Tx is inactive i.e ready for a byte to eat and the
         // IRQ is enabled then generate it always
         if((mUART_TX_COUNTDOWN&UART_TX_INACTIVE) && mUART_TX_IRQ_ENABLE)
         {
            TRACE_MIKIE0("Update() - UART TX IRQ Triggered");
            mTimerStatusFlags|=0x10;
            gSystemIRQ=TRUE;    // Added 19/09/06 fix for IRQ issue
         }
         // Is data waiting and the interrupt enabled, if so then
         // what are we waiting for....
         if(mUART_RX_READY && mUART_RX_IRQ_ENABLE)
         {
            TRACE_MIKIE0("Update() - UART RX IRQ Triggered");
            mTimerStatusFlags|=0x10;
            gSystemIRQ=TRUE;    // Added 19/09/06 fix for IRQ issue
         }

         //
         // Timer 1 of Group B
         //
         // KW bugfix 13/4/99 added (mTIM_x_ENABLE_RELOAD ||  ..)
         if(mTIM_1_ENABLE_COUNT && (mTIM_1_ENABLE_RELOAD || !mTIM_1_TIMER_DONE))
         {
            if(mTIM_1_LINKING!=0x07)
            {
               // Ordinary clocked mode as opposed to linked mode
               // 16MHz clock downto 1us == cyclecount >> 4
               divide=(4+mTIM_1_LINKING);
               decval=(gSystemCycleCount-mTIM_1_LAST_COUNT)>>divide;

               if(decval)
               {
                  mTIM_1_LAST_COUNT+=decval<<divide;
                  mTIM_1_CURRENT-=decval;
                  if(mTIM_1_CURRENT&0x80000000)
                  {
                     // Set carry out
                     mTIM_1_BORROW_OUT=TRUE;

                     // Set the timer status flag
                     if(mTimerInterruptMask&0x02)
                     {
                        TRACE_MIKIE0("Update() - TIMER1 IRQ Triggered");
                        mTimerStatusFlags|=0x02;
                        gSystemIRQ=TRUE;    // Added 19/09/06 fix for IRQ issue
                     }

                     // Reload if neccessary
                     if(mTIM_1_ENABLE_RELOAD)
                     {
                        mTIM_1_CURRENT+=mTIM_1_BKUP+1;
                     }
                     else
                     {
                        mTIM_1_CURRENT=0;
                     }
                     mTIM_1_TIMER_DONE=TRUE;
                  }
                  else
                  {
                     mTIM_1_BORROW_OUT=FALSE;
                  }
                  // Set carry in as we did a count
                  mTIM_1_BORROW_IN=TRUE;
               }
               else
               {
                  // Clear carry in as we didn't count
                  mTIM_1_BORROW_IN=FALSE;
                  // Clear carry out
                  mTIM_1_BORROW_OUT=FALSE;
               }
            }

            // Prediction for next timer event cycle number

            if(mTIM_1_LINKING!=7)
            {
               // Sometimes timeupdates can be >2x rollover in which case
               // then CURRENT may still be negative and we can use it to
               // calc the next timer value, we just want another update ASAP
               tmp=(mTIM_1_CURRENT&0x80000000)?1:((mTIM_1_CURRENT+1)<<divide);
               tmp+=gSystemCycleCount;
               if(tmp<gNextTimerEvent)
               {
                  gNextTimerEvent=tmp;
                  TRACE_MIKIE1("Update() - TIMER 1 Set NextTimerEvent = %012d",gNextTimerEvent);
               }
            }
            //              TRACE_MIKIE1("Update() - mTIM_1_CURRENT = %012d",mTIM_1_CURRENT);
            //              TRACE_MIKIE1("Update() - mTIM_1_BKUP    = %012d",mTIM_1_BKUP);
            //              TRACE_MIKIE1("Update() - mTIM_1_LASTCNT = %012d",mTIM_1_LAST_COUNT);
            //              TRACE_MIKIE1("Update() - mTIM_1_LINKING = %012d",mTIM_1_LINKING);
         }

         //
         // Timer 3 of Group A
         //
         // KW bugfix 13/4/99 added (mTIM_x_ENABLE_RELOAD ||  ..)
         if(mTIM_3_ENABLE_COUNT && (mTIM_3_ENABLE_RELOAD || !mTIM_3_TIMER_DONE))
         {
            decval=0;

            if(mTIM_3_LINKING==0x07)
            {
               if(mTIM_1_BORROW_OUT) decval=1;
               mTIM_3_LAST_LINK_CARRY=mTIM_1_BORROW_OUT;
            }
            else
            {
               // Ordinary clocked mode as opposed to linked mode
               // 16MHz clock downto 1us == cyclecount >> 4
               divide=(4+mTIM_3_LINKING);
               decval=(gSystemCycleCount-mTIM_3_LAST_COUNT)>>divide;
            }

            if(decval)
            {
               mTIM_3_LAST_COUNT+=decval<<divide;
               mTIM_3_CURRENT-=decval;
               if(mTIM_3_CURRENT&0x80000000)
               {
                  // Set carry out
                  mTIM_3_BORROW_OUT=TRUE;

                  // Set the timer status flag
                  if(mTimerInterruptMask&0x08)
                  {
                     TRACE_MIKIE0("Update() - TIMER3 IRQ Triggered");
                     mTimerStatusFlags|=0x08;
                     gSystemIRQ=TRUE;   // Added 19/09/06 fix for IRQ issue
                  }

                  // Reload if neccessary
                  if(mTIM_3_ENABLE_RELOAD)
                  {
                     mTIM_3_CURRENT+=mTIM_3_BKUP+1;
                  }
                  else
                  {
                     mTIM_3_CURRENT=0;
                  }
                  mTIM_3_TIMER_DONE=TRUE;
               }
               else
               {
                  mTIM_3_BORROW_OUT=FALSE;
               }
               // Set carry in as we did a count
               mTIM_3_BORROW_IN=TRUE;
            }
            else
            {
               // Clear carry in as we didn't count
               mTIM_3_BORROW_IN=FALSE;
               // Clear carry out
               mTIM_3_BORROW_OUT=FALSE;
            }

            // Prediction for next timer event cycle number

            if(mTIM_3_LINKING!=7)
            {
               // Sometimes timeupdates can be >2x rollover in which case
               // then CURRENT may still be negative and we can use it to
               // calc the next timer value, we just want another update ASAP
               tmp=(mTIM_3_CURRENT&0x80000000)?1:((mTIM_3_CURRENT+1)<<divide);
               tmp+=gSystemCycleCount;
               if(tmp<gNextTimerEvent)
               {
                  gNextTimerEvent=tmp;
                  TRACE_MIKIE1("Update() - TIMER 3 Set NextTimerEvent = %012d",gNextTimerEvent);
               }
            }
            //              TRACE_MIKIE1("Update() - mTIM_3_CURRENT = %012d",mTIM_3_CURRENT);
            //              TRACE_MIKIE1("Update() - mTIM_3_BKUP    = %012d",mTIM_3_BKUP);
            //              TRACE_MIKIE1("Update() - mTIM_3_LASTCNT = %012d",mTIM_3_LAST_COUNT);
            //              TRACE_MIKIE1("Update() - mTIM_3_LINKING = %012d",mTIM_3_LINKING);
         }

         //
         // Timer 5 of Group A
         //
         // KW bugfix 13/4/99 added (mTIM_x_ENABLE_RELOAD ||  ..)
         if(mTIM_5_ENABLE_COUNT && (mTIM_5_ENABLE_RELOAD || !mTIM_5_TIMER_DONE))
         {
            decval=0;

            if(mTIM_5_LINKING==0x07)
            {
               if(mTIM_3_BORROW_OUT) decval=1;
               mTIM_5_LAST_LINK_CARRY=mTIM_3_BORROW_OUT;
            }
            else
            {
               // Ordinary clocked mode as opposed to linked mode
               // 16MHz clock downto 1us == cyclecount >> 4
               divide=(4+mTIM_5_LINKING);
               decval=(gSystemCycleCount-mTIM_5_LAST_COUNT)>>divide;
            }

            if(decval)
            {
               mTIM_5_LAST_COUNT+=decval<<divide;
               mTIM_5_CURRENT-=decval;
               if(mTIM_5_CURRENT&0x80000000)
               {
                  // Set carry out
                  mTIM_5_BORROW_OUT=TRUE;

                  // Set the timer status flag
                  if(mTimerInterruptMask&0x20)
                  {
                     TRACE_MIKIE0("Update() - TIMER5 IRQ Triggered");
                     mTimerStatusFlags|=0x20;
                     gSystemIRQ=TRUE;   // Added 19/09/06 fix for IRQ issue
                  }

                  // Reload if neccessary
                  if(mTIM_5_ENABLE_RELOAD)
                  {
                     mTIM_5_CURRENT+=mTIM_5_BKUP+1;
                  }
                  else
                  {
                     mTIM_5_CURRENT=0;
                  }
                  mTIM_5_TIMER_DONE=TRUE;
               }
               else
               {
                  mTIM_5_BORROW_OUT=FALSE;
               }
               // Set carry in as we did a count
               mTIM_5_BORROW_IN=TRUE;
            }
            else
            {
               // Clear carry in as we didn't count
               mTIM_5_BORROW_IN=FALSE;
               // Clear carry out
               mTIM_5_BORROW_OUT=FALSE;
            }

            // Prediction for next timer event cycle number

            if(mTIM_5_LINKING!=7)
            {
               // Sometimes timeupdates can be >2x rollover in which case
               // then CURRENT may still be negative and we can use it to
               // calc the next timer value, we just want another update ASAP
               tmp=(mTIM_5_CURRENT&0x80000000)?1:((mTIM_5_CURRENT+1)<<divide);
               tmp+=gSystemCycleCount;
               if(tmp<gNextTimerEvent)
               {
                  gNextTimerEvent=tmp;
                  TRACE_MIKIE1("Update() - TIMER 5 Set NextTimerEvent = %012d",gNextTimerEvent);
               }
            }
            //              TRACE_MIKIE1("Update() - mTIM_5_CURRENT = %012d",mTIM_5_CURRENT);
            //              TRACE_MIKIE1("Update() - mTIM_5_BKUP    = %012d",mTIM_5_BKUP);
            //              TRACE_MIKIE1("Update() - mTIM_5_LASTCNT = %012d",mTIM_5_LAST_COUNT);
            //              TRACE_MIKIE1("Update() - mTIM_5_LINKING = %012d",mTIM_5_LINKING);
         }

         //
         // Timer 7 of Group A
         //
         // KW bugfix 13/4/99 added (mTIM_x_ENABLE_RELOAD ||  ..)
         if(mTIM_7_ENABLE_COUNT && (mTIM_7_ENABLE_RELOAD || !mTIM_7_TIMER_DONE))
         {
            decval=0;

            if(mTIM_7_LINKING==0x07)
            {
               if(mTIM_5_BORROW_OUT) decval=1;
               mTIM_7_LAST_LINK_CARRY=mTIM_5_BORROW_OUT;
            }
            else
            {
               // Ordinary clocked mode as opposed to linked mode
               // 16MHz clock downto 1us == cyclecount >> 4
               divide=(4+mTIM_7_LINKING);
               decval=(gSystemCycleCount-mTIM_7_LAST_COUNT)>>divide;
            }

            if(decval)
            {
               mTIM_7_LAST_COUNT+=decval<<divide;
               mTIM_7_CURRENT-=decval;
               if(mTIM_7_CURRENT&0x80000000)
               {
                  // Set carry out
                  mTIM_7_BORROW_OUT=TRUE;

                  // Set the timer status flag
                  if(mTimerInterruptMask&0x80)
                  {
                     TRACE_MIKIE0("Update() - TIMER7 IRQ Triggered");
                     mTimerStatusFlags|=0x80;
                     gSystemIRQ=TRUE;   // Added 19/09/06 fix for IRQ issue
                  }

                  // Reload if neccessary
                  if(mTIM_7_ENABLE_RELOAD)
                  {
                     mTIM_7_CURRENT+=mTIM_7_BKUP+1;
                  }
                  else
                  {
                     mTIM_7_CURRENT=0;
                  }
                  mTIM_7_TIMER_DONE=TRUE;

               }
               else
               {
                  mTIM_7_BORROW_OUT=FALSE;
               }
               // Set carry in as we did a count
               mTIM_7_BORROW_IN=TRUE;
            }
            else
            {
               // Clear carry in as we didn't count
               mTIM_7_BORROW_IN=FALSE;
               // Clear carry out
               mTIM_7_BORROW_OUT=FALSE;
            }

            // Prediction for next timer event cycle number

            if(mTIM_7_LINKING!=7)
            {
               // Sometimes timeupdates can be >2x rollover in which case
               // then CURRENT may still be negative and we can use it to
               // calc the next timer value, we just want another update ASAP
               tmp=(mTIM_7_CURRENT&0x80000000)?1:((mTIM_7_CURRENT+1)<<divide);
               tmp+=gSystemCycleCount;
               if(tmp<gNextTimerEvent)
               {
                  gNextTimerEvent=tmp;
                  TRACE_MIKIE1("Update() - TIMER 7 Set NextTimerEvent = %012d",gNextTimerEvent);
               }
            }
            //              TRACE_MIKIE1("Update() - mTIM_7_CURRENT = %012d",mTIM_7_CURRENT);
            //              TRACE_MIKIE1("Update() - mTIM_7_BKUP    = %012d",mTIM_7_BKUP);
            //              TRACE_MIKIE1("Update() - mTIM_7_LASTCNT = %012d",mTIM_7_LAST_COUNT);
            //              TRACE_MIKIE1("Update() - mTIM_7_LINKING = %012d",mTIM_7_LINKING);
         }

         //
         // Timer 6 has no group
         //
         // KW bugfix 13/4/99 added (mTIM_x_ENABLE_RELOAD ||  ..)
         if(mTIM_6_ENABLE_COUNT && (mTIM_6_ENABLE_RELOAD || !mTIM_6_TIMER_DONE))
         {
            //              if(mTIM_6_LINKING!=0x07)
            {
               // Ordinary clocked mode as opposed to linked mode
               // 16MHz clock downto 1us == cyclecount >> 4
               divide=(4+mTIM_6_LINKING);
               decval=(gSystemCycleCount-mTIM_6_LAST_COUNT)>>divide;

               if(decval)
               {
                  mTIM_6_LAST_COUNT+=decval<<divide;
                  mTIM_6_CURRENT-=decval;
                  if(mTIM_6_CURRENT&0x80000000)
                  {
                     // Set carry out
                     mTIM_6_BORROW_OUT=TRUE;

                     // Set the timer status flag
                     if(mTimerInterruptMask&0x40)
                     {
                        TRACE_MIKIE0("Update() - TIMER6 IRQ Triggered");
                        mTimerStatusFlags|=0x40;
                        gSystemIRQ=TRUE;    // Added 19/09/06 fix for IRQ issue
                     }

                     // Reload if neccessary
                     if(mTIM_6_ENABLE_RELOAD)
                     {
                        mTIM_6_CURRENT+=mTIM_6_BKUP+1;
                     }
                     else
                     {
                        mTIM_6_CURRENT=0;
                     }
                     mTIM_6_TIMER_DONE=TRUE;
                  }
                  else
                  {
                     mTIM_6_BORROW_OUT=FALSE;
                  }
                  // Set carry in as we did a count
                  mTIM_6_BORROW_IN=TRUE;
               }
               else
               {
                  // Clear carry in as we didn't count
                  mTIM_6_BORROW_IN=FALSE;
                  // Clear carry out
                  mTIM_6_BORROW_OUT=FALSE;
               }
            }

            // Prediction for next timer event cycle number
            // (Timer 6 doesn't support linking)

            //              if(mTIM_6_LINKING!=7)
            {
               // Sometimes timeupdates can be >2x rollover in which case
               // then CURRENT may still be negative and we can use it to
               // calc the next timer value, we just want another update ASAP
               tmp=(mTIM_6_CURRENT&0x80000000)?1:((mTIM_6_CURRENT+1)<<divide);
               tmp+=gSystemCycleCount;
               if(tmp<gNextTimerEvent)
               {
                  gNextTimerEvent=tmp;
                  TRACE_MIKIE1("Update() - TIMER 6 Set NextTimerEvent = %012d",gNextTimerEvent);
               }
            }
            //              TRACE_MIKIE1("Update() - mTIM_6_CURRENT = %012d",mTIM_6_CURRENT);
            //              TRACE_MIKIE1("Update() - mTIM_6_BKUP    = %012d",mTIM_6_BKUP);
            //              TRACE_MIKIE1("Update() - mTIM_6_LASTCNT = %012d",mTIM_6_LAST_COUNT);
            //              TRACE_MIKIE1("Update() - mTIM_6_LINKING = %012d",mTIM_6_LINKING);
         }

         //
         // If sound is enabled then update the sound subsystem
         //
         if(gAudioEnabled)
         {
            UpdateSound();
            UpdateCalcSound();
         }

         //         if(gSystemCycleCount==gNextTimerEvent) gError->Warning("CMikie::Update() - gSystemCycleCount==gNextTimerEvent, system lock likely");
         //         TRACE_MIKIE1("Update() - NextTimerEvent = %012d",gNextTimerEvent);

         // Now all the timer updates are done we can increment the system
         // counter for any work done within the Update() function, gSystemCycleCounter
         // cannot be updated until this point otherwise it screws up the counters.
         gSystemCycleCount+=mikie_work_done;
