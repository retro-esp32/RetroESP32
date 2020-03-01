   //   TRACE_SUSIE0("LineInit()");

   mLineShiftReg=0;
   mLineShiftRegCount=0;
   mLineRepeatCount=0;
   mLinePixel=0;
   mLineType=line_error;
   mLinePacketBitsLeft=0xffff;

   // Initialise the temporary pointer

   mTMPADR=mSPRDLINE;

   // First read the Offset to the next line

   ULONG offset=LineGetBits(8);
   //   TRACE_SUSIE1("LineInit() Offset=%04x",offset);

   // Specify the MAXIMUM number of bits in this packet, it
   // can terminate early but can never use more than this
   // without ending the current packet, we count down in LineGetBits()

   mLinePacketBitsLeft=(offset-1)*8;

   // Literals are a special case and get their count set on a line basis

   if(mSPRCTL1_Literal)
   {
      mLineType=line_abs_literal;
      mLineRepeatCount=((offset-1)*8)/mSPRCTL0_PixelBits;
      // Why is this necessary, is this compensating for the 1,1 offset bug
      //        mLineRepeatCount--;
   }
   //   TRACE_SUSIE1("LineInit() mLineRepeatCount=$%04x",mLineRepeatCount);

   // Set the line base address for use in the calls to pixel painting

   if(voff>101)
   {
      gError->Warning("CSusie::LineInit() Out of bounds (voff)");
      voff=0;
   }

   mLineBaseAddress=mVIDBAS.Word+(voff*(SCREEN_WIDTH/2));
   mLineCollisionAddress=mCOLLBAS.Word+(voff*(SCREEN_WIDTH/2));
   //   TRACE_SUSIE1("LineInit() mLineBaseAddress=$%04x",mLineBaseAddress);
   //   TRACE_SUSIE1("LineInit() mLineCollisionAddress=$%04x",mLineCollisionAddress);

   // Return the offset to the next line

   return offset;