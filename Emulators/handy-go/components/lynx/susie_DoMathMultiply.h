   mSPRSYS_Mathbit=FALSE;

   // Multiplies with out sign or accumulate take 44 ticks to complete.
   // Multiplies with sign and accumulate take 54 ticks to complete.
   //
   //    AB                                    EFGH
   //  * CD                                  /   NP
   // -------                            -----------
   //  EFGH                                    ABCD
   // Accumulate in JKLM         Remainder in (JK)LM
   //

   // Basic multiply is ALWAYS unsigned, sign conversion is done later
   ULONG result=(ULONG)mMATHABCD.Words.AB*(ULONG)mMATHABCD.Words.CD;
   mMATHEFGH.Long=result;

   if(mSPRSYS_SignedMath)
   {
      TRACE_SUSIE0("DoMathMultiply() - SIGNED");
      // Add the sign bits, only >0 is +ve result
      mMATHEFGH_sign=mMATHAB_sign+mMATHCD_sign;
      if(!mMATHEFGH_sign)
      {
         mMATHEFGH.Long^=0xffffffff;
         mMATHEFGH.Long++;
      }
   }
   else
   {
      TRACE_SUSIE0("DoMathMultiply() - UNSIGNED");
   }

   TRACE_SUSIE2("DoMathMultiply() AB=$%04x * CD=$%04x",mMATHABCD.Words.AB,mMATHABCD.Words.CD);

   // Check overflow, if B31 has changed from 1->0 then its overflow time
   if(mSPRSYS_Accumulate)
   {
      TRACE_SUSIE0("DoMathMultiply() - ACCUMULATED JKLM+=EFGH");
      ULONG tmp=mMATHJKLM.Long+mMATHEFGH.Long;
      // Let sign change indicate overflow
      if((tmp&0x80000000)!=(mMATHJKLM.Long&0x80000000))
      {
         TRACE_SUSIE0("DoMathMultiply() - OVERFLOW DETECTED");
         //         mSPRSYS_Mathbit=TRUE;
      }
      else
      {
         //         mSPRSYS_Mathbit=FALSE;
      }
      // Save accumulated result
      mMATHJKLM.Long=tmp;
   }

   TRACE_SUSIE1("DoMathMultiply() Results (raw - no sign) Result=$%08x",result);
   TRACE_SUSIE1("DoMathMultiply() Results (Multi) EFGH=$%08x",mMATHEFGH.Long);
   TRACE_SUSIE1("DoMathMultiply() Results (Accum) JKLM=$%08x",mMATHJKLM.Long);