   ULONG col_addr=mLineCollisionAddress+(hoff/2);

   ULONG data=RAM_PEEK(col_addr);
   if(!(hoff&0x01))
   {
      // Upper nibble read
      data>>=4;
   }
   else
   {
      // Lower nibble read
      data&=0x0f;
   }

   // Increment cycle count for the read/modify/write
   cycles_used+=SPR_RDWR_CYC;

   return data;