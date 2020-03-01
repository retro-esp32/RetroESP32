   mSPRSYS_Mathbit=FALSE;

   //
   // Divides take 176 + 14*N ticks
   // (N is the number of most significant zeros in the divisor.)
   //
   //    AB                                    EFGH
   //  * CD                                  /   NP
   // -------                            -----------
   //  EFGH                                    ABCD
   // Accumulate in JKLM         Remainder in (JK)LM
   //

   // Divide is ALWAYS unsigned arithmetic...
   if(mMATHNP.Long)
   {
      TRACE_SUSIE0("DoMathDivide() - UNSIGNED");
      mMATHABCD.Long=mMATHEFGH.Long/mMATHNP.Long;
      mMATHJKLM.Long=mMATHEFGH.Long%mMATHNP.Long;
   }
   else
   {
      TRACE_SUSIE0("DoMathDivide() - DIVIDE BY ZERO ERROR");
      mMATHABCD.Long=0xffffffff;
      mMATHJKLM.Long=0;
      mSPRSYS_Mathbit=TRUE;
   }
   TRACE_SUSIE2("DoMathDivide() EFGH=$%08x / NP=%04x",mMATHEFGH.Long,mMATHNP.Long);
   TRACE_SUSIE1("DoMathDivide() Results (div) ABCD=$%08x",mMATHABCD.Long);
   TRACE_SUSIE1("DoMathDivide() Results (mod) JKLM=$%08x",mMATHJKLM.Long);