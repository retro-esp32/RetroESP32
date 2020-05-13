//
// Collision code modified by KW 22/11/98
// Collision buffer cler added if there is no
// apparent collision, I have a gut feeling this
// is the wrong solution to the inv07.com bug but
// it seems to work OK.
//
// Shadow-------------------------------|
// Boundary-Shadow--------------------| |
// Normal---------------------------| | |
// Boundary-----------------------| | | |
// Background-Shadow------------| | | | |
// Background-No Collision----| | | | | |
// Non-Collideable----------| | | | | | |
// Exclusive-or-Shadow----| | | | | | | |
//                        | | | | | | | |
//                        1 1 1 1 0 1 0 1   F is opaque
//                        0 0 0 0 1 1 0 0   E is collideable
//                        0 0 1 1 0 0 0 0   0 is opaque and collideable
//                        1 0 0 0 1 1 1 1   allow collision detect
//                        1 0 0 1 1 1 1 1   allow coll. buffer access
//                        1 0 0 0 0 0 0 0   exclusive-or the data
//

   switch(mSPRCTL0_Type)
   {
      // BACKGROUND SHADOW
      // 1   F is opaque
      // 0   E is collideable
      // 1   0 is opaque and collideable
      // 0   allow collision detect
      // 1   allow coll. buffer access
      // 0   exclusive-or the data
      case sprite_background_shadow:
         WritePixel(hoff,pixel);
         if(!mSPRCOLL_Collide && !mSPRSYS_NoCollide && pixel!=0x0e)
         {
            WriteCollision(hoff,mSPRCOLL_Number);
         }
         break;

         // BACKGROUND NOCOLLIDE
         // 1   F is opaque
         // 0   E is collideable
         // 1   0 is opaque and collideable
         // 0   allow collision detect
         // 0   allow coll. buffer access
         // 0   exclusive-or the data
      case sprite_background_noncollide:
         WritePixel(hoff,pixel);
         break;

         // NOCOLLIDE
         // 1   F is opaque
         // 0   E is collideable
         // 0   0 is opaque and collideable
         // 0   allow collision detect
         // 0   allow coll. buffer access
         // 0   exclusive-or the data
      case sprite_noncollide:
         if(pixel!=0x00) WritePixel(hoff,pixel);
         break;

         // BOUNDARY
         // 0   F is opaque
         // 1   E is collideable
         // 0   0 is opaque and collideable
         // 1   allow collision detect
         // 1   allow coll. buffer access
         // 0   exclusive-or the data
      case sprite_boundary:
         if(pixel!=0x00 && pixel!=0x0f)
         {
            WritePixel(hoff,pixel);
         }
         if(pixel!=0x00)
         {
            if(!mSPRCOLL_Collide && !mSPRSYS_NoCollide)
            {
               int collision=ReadCollision(hoff);
               if(collision>mCollision)
               {
                  mCollision=collision;
               }
               // 01/05/00 V0.7 if(mSPRCOLL_Number>collision)
               {
                  WriteCollision(hoff,mSPRCOLL_Number);
               }
            }
         }
         break;

         // NORMAL
         // 1   F is opaque
         // 1   E is collideable
         // 0   0 is opaque and collideable
         // 1   allow collision detect
         // 1   allow coll. buffer access
         // 0   exclusive-or the data
      case sprite_normal:
         if(pixel!=0x00)
         {
            WritePixel(hoff,pixel);
            if(!mSPRCOLL_Collide && !mSPRSYS_NoCollide)
            {
               int collision=ReadCollision(hoff);
               if(collision>mCollision)
               {
                  mCollision=collision;
               }
               // 01/05/00 V0.7 if(mSPRCOLL_Number>collision)
               {
                  WriteCollision(hoff,mSPRCOLL_Number);
               }
            }
         }
         break;

         // BOUNDARY_SHADOW
         // 0   F is opaque
         // 0   E is collideable
         // 0   0 is opaque and collideable
         // 1   allow collision detect
         // 1   allow coll. buffer access
         // 0   exclusive-or the data
      case sprite_boundary_shadow:
         if(pixel!=0x00 && pixel!=0x0e && pixel!=0x0f)
         {
            WritePixel(hoff,pixel);
         }
         if(pixel!=0x00 && pixel!=0x0e)
         {
            if(!mSPRCOLL_Collide && !mSPRSYS_NoCollide)
            {
               int collision=ReadCollision(hoff);
               if(collision>mCollision)
               {
                  mCollision=collision;
               }
               // 01/05/00 V0.7 if(mSPRCOLL_Number>collision)
               {
                  WriteCollision(hoff,mSPRCOLL_Number);
               }
            }
         }
         break;

         // SHADOW
         // 1   F is opaque
         // 0   E is collideable
         // 0   0 is opaque and collideable
         // 1   allow collision detect
         // 1   allow coll. buffer access
         // 0   exclusive-or the data
      case sprite_shadow:
         if(pixel!=0x00)
         {
            WritePixel(hoff,pixel);
         }
         if(pixel!=0x00 && pixel!=0x0e)
         {
            if(!mSPRCOLL_Collide && !mSPRSYS_NoCollide)
            {
               int collision=ReadCollision(hoff);
               if(collision>mCollision)
               {
                  mCollision=collision;
               }
               // 01/05/00 V0.7 if(mSPRCOLL_Number>collision)
               {
                  WriteCollision(hoff,mSPRCOLL_Number);
               }
            }
         }
         break;

         // XOR SHADOW
         // 1   F is opaque
         // 0   E is collideable
         // 0   0 is opaque and collideable
         // 1   allow collision detect
         // 1   allow coll. buffer access
         // 1   exclusive-or the data
      case sprite_xor_shadow:
         if(pixel!=0x00)
         {
            WritePixel(hoff,ReadPixel(hoff)^pixel);
         }
         if(pixel!=0x00 && pixel!=0x0e)
         {
            if(!mSPRCOLL_Collide && !mSPRSYS_NoCollide && pixel!=0x0e)
            {
               int collision=ReadCollision(hoff);
               if(collision>mCollision)
               {
                  mCollision=collision;
               }
               // 01/05/00 V0.7 if(mSPRCOLL_Number>collision)
               {
                  WriteCollision(hoff,mSPRCOLL_Number);
               }
            }
         }
         break;
      default:
         //         _asm int 3;
         break;
   }