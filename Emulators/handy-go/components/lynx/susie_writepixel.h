   ULONG scr_addr=mLineBaseAddress+(hoff/2);

   UBYTE dest=RAM_PEEK(scr_addr);
   if(!(hoff&0x01))
   {
      // Upper nibble screen write
      dest&=0x0f;
      dest|=pixel<<4;
   }
   else
   {
      // Lower nibble screen write
      dest&=0xf0;
      dest|=pixel;
   }
   RAM_POKE(scr_addr,dest);

   // Increment cycle count for the read/modify/write
   cycles_used+=2*SPR_RDWR_CYC;
