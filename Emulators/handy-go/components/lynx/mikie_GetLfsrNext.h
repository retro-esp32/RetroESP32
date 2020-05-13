   // The table is built thus:
   //   Bits 0-11  LFSR                 (12 Bits)
   //  Bits 12-20 Feedback switches (9 Bits)
   //     (Order = 7,0,1,2,3,4,5,10,11)
   //  Order is mangled to make peek/poke easier as
   //  bit 7 is in a seperate register
   //
   // Total 21 bits = 2MWords @ 4 Bytes/Word = 8MB !!!!!
   //
   // If the index is a combination of Current LFSR+Feedback the
   // table will give the next value.
#ifdef MY_NO_STATIC
   ULONG switches,lfsr,next,swloop,result;
   ULONG switchbits[9]={7,0,1,2,3,4,5,10,11};
#else
   static ULONG switches,lfsr,next,swloop,result;
   static ULONG switchbits[9]={7,0,1,2,3,4,5,10,11};
#endif

   switches=current>>12;
   lfsr=current&0xfff;
   result=0;
   for(swloop=0;swloop<9;swloop++)
   {
      if((switches>>swloop)&0x001) result^=(lfsr>>switchbits[swloop])&0x001;
   }
   result=(result)?0:1;
   next=(switches<<12)|((lfsr<<1)&0xffe)|result;
   return next;
