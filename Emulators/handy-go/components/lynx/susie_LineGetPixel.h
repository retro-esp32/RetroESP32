   if(!mLineRepeatCount)
   {
      // Normal sprites fetch their counts on a packet basis
      if(mLineType!=line_abs_literal)
      {
         ULONG literal=LineGetBits(1);
         if(literal) mLineType=line_literal; else mLineType=line_packed;
      }

      // Pixel store is empty what should we do
      switch(mLineType)
      {
         case line_abs_literal:
            // This means end of line for us
            mLinePixel=LINE_END;
            return mLinePixel;      // SPEEDUP
         case line_literal:
            mLineRepeatCount=LineGetBits(4);
            mLineRepeatCount++;
            break;
         case line_packed:
            //
            // From reading in between the lines only a packed line with
            // a zero size i.e 0b00000 as a header is allowable as a packet end
            //
            mLineRepeatCount=LineGetBits(4);
            if(!mLineRepeatCount)
               mLinePixel=LINE_END;
            else
               mLinePixel=mPenIndex[LineGetBits(mSPRCTL0_PixelBits)];
            mLineRepeatCount++;
            break;
         default:
            return 0;
      }

   }

   if(mLinePixel!=LINE_END)
   {
      mLineRepeatCount--;

      switch(mLineType)
      {
         case line_abs_literal:
            mLinePixel=LineGetBits(mSPRCTL0_PixelBits);
            // Check the special case of a zero in the last pixel
            if(!mLineRepeatCount && !mLinePixel)
               mLinePixel=LINE_END;
            else
               mLinePixel=mPenIndex[mLinePixel];
            break;
         case line_literal:
            mLinePixel=mPenIndex[LineGetBits(mSPRCTL0_PixelBits)];
            break;
         case line_packed:
            break;
         default:
            return 0;
      }
   }

   return mLinePixel;