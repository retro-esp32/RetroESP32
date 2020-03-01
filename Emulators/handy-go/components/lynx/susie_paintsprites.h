   int  sprcount=0;
   int data=0;
   int everonscreen=0;

   TRACE_SUSIE0("                                                              ");
   TRACE_SUSIE0("                                                              ");
   TRACE_SUSIE0("                                                              ");
   TRACE_SUSIE0("**************************************************************");
   TRACE_SUSIE0("********************** PaintSprites **************************");
   TRACE_SUSIE0("**************************************************************");
   TRACE_SUSIE0("                                                              ");

   TRACE_SUSIE1("PaintSprites() VIDBAS  $%04x",mVIDBAS.Word);
   TRACE_SUSIE1("PaintSprites() COLLBAS $%04x",mCOLLBAS.Word);
   TRACE_SUSIE1("PaintSprites() SPRSYS  $%02x",Peek(SPRSYS));

   if(!mSUZYBUSEN || !mSPRGO)
   {
      TRACE_SUSIE0("PaintSprites() Returned !mSUZYBUSEN || !mSPRGO");
      return 0;
   }

   cycles_used=0;

   do
   {
      TRACE_SUSIE1("PaintSprites() ************ Rendering Sprite %03d ************",sprcount);

      everonscreen=0;// everon has to be reset for every sprite, thus line was moved inside this loop

      // Step 1 load up the SCB params into Susie

      // And thus it is documented that only the top byte of SCBNEXT is used.
      // Its mentioned under the bits that are broke section in the bluebook
      if(!(mSCBNEXT.Word&0xff00))
      {
         TRACE_SUSIE0("PaintSprites() mSCBNEXT==0 - FINISHED");
         mSPRSYS_Status=0;  // Engine has finished
         mSPRGO=FALSE;
         break;
      }
      else
      {
         mSPRSYS_Status=1;
      }

      mTMPADR.Word=mSCBNEXT.Word;   // Copy SCB pointer
      mSCBADR.Word=mSCBNEXT.Word;   // Copy SCB pointer
      TRACE_SUSIE1("PaintSprites() SCBADDR $%04x",mSCBADR.Word);

      data=RAM_PEEK(mTMPADR.Word);          // Fetch control 0
      TRACE_SUSIE1("PaintSprites() SPRCTL0 $%02x",data);
      mSPRCTL0_Type=data&0x0007;
      mSPRCTL0_Vflip=data&0x0010;
      mSPRCTL0_Hflip=data&0x0020;
      mSPRCTL0_PixelBits=((data&0x00c0)>>6)+1;
      mTMPADR.Word+=1;

      data=RAM_PEEK(mTMPADR.Word);          // Fetch control 1
      TRACE_SUSIE1("PaintSprites() SPRCTL1 $%02x",data);
      mSPRCTL1_StartLeft=data&0x0001;
      mSPRCTL1_StartUp=data&0x0002;
      mSPRCTL1_SkipSprite=data&0x0004;
      mSPRCTL1_ReloadPalette=data&0x0008;
      mSPRCTL1_ReloadDepth=(data&0x0030)>>4;
      mSPRCTL1_Sizing=data&0x0040;
      mSPRCTL1_Literal=data&0x0080;
      mTMPADR.Word+=1;

      data=RAM_PEEK(mTMPADR.Word);          // Collision num
      TRACE_SUSIE1("PaintSprites() SPRCOLL $%02x",data);
      mSPRCOLL_Number=data&0x000f;
      mSPRCOLL_Collide=data&0x0020;
      mTMPADR.Word+=1;

      mSCBNEXT.Word=RAM_PEEKW(mTMPADR.Word);    // Next SCB
      TRACE_SUSIE1("PaintSprites() SCBNEXT $%04x",mSCBNEXT.Word);
      mTMPADR.Word+=2;

      cycles_used+=5*SPR_RDWR_CYC;

      // Initialise the collision depositary

      // Although Tom Schenck says this is correct, it doesnt appear to be
      //        if(!mSPRCOLL_Collide && !mSPRSYS_NoCollide)
      //        {
      //            mCollision=RAM_PEEK((mSCBADR.Word+mCOLLOFF.Word)&0xffff);
      //            mCollision&=0x0f;
      //        }
      mCollision=0;

      // Check if this is a skip sprite

      if(!mSPRCTL1_SkipSprite)
      {
         bool enable_sizing  = FALSE;
         bool enable_stretch = FALSE;
         bool enable_tilt    = FALSE;

         mSPRDLINE.Word=RAM_PEEKW(mTMPADR.Word);    // Sprite pack data
         TRACE_SUSIE1("PaintSprites() SPRDLINE $%04x",mSPRDLINE.Word);
         mTMPADR.Word+=2;

         mHPOSSTRT.Word=RAM_PEEKW(mTMPADR.Word);    // Sprite horizontal start position
         TRACE_SUSIE1("PaintSprites() HPOSSTRT $%04x",mHPOSSTRT.Word);
         mTMPADR.Word+=2;

         mVPOSSTRT.Word=RAM_PEEKW(mTMPADR.Word);    // Sprite vertical start position
         TRACE_SUSIE1("PaintSprites() VPOSSTRT $%04x",mVPOSSTRT.Word);
         mTMPADR.Word+=2;

         cycles_used+=6*SPR_RDWR_CYC;

         /* Optional section defined by reload type in Control 1 */

         TRACE_SUSIE1("PaintSprites() mSPRCTL1.Bits.ReloadDepth=%d",mSPRCTL1_ReloadDepth);
         switch(mSPRCTL1_ReloadDepth)
         {
            case 1:
               TRACE_SUSIE0("PaintSprites() Sizing Enabled");
               enable_sizing=TRUE;

               mSPRHSIZ.Word=RAM_PEEKW(mTMPADR.Word);   // Sprite Horizontal size
               mTMPADR.Word+=2;

               mSPRVSIZ.Word=RAM_PEEKW(mTMPADR.Word);   // Sprite Verticalal size
               mTMPADR.Word+=2;

               cycles_used+=4*SPR_RDWR_CYC;
               break;

            case 2:
               TRACE_SUSIE0("PaintSprites() Sizing Enabled");
               TRACE_SUSIE0("PaintSprites() Stretch Enabled");
               enable_sizing=TRUE;
               enable_stretch=TRUE;

               mSPRHSIZ.Word=RAM_PEEKW(mTMPADR.Word);   // Sprite Horizontal size
               mTMPADR.Word+=2;

               mSPRVSIZ.Word=RAM_PEEKW(mTMPADR.Word);   // Sprite Verticalal size
               mTMPADR.Word+=2;

               mSTRETCH.Word=RAM_PEEKW(mTMPADR.Word);   // Sprite stretch
               mTMPADR.Word+=2;

               cycles_used+=6*SPR_RDWR_CYC;
               break;

            case 3:
               TRACE_SUSIE0("PaintSprites() Sizing Enabled");
               TRACE_SUSIE0("PaintSprites() Stretch Enabled");
               TRACE_SUSIE0("PaintSprites() Tilt Enabled");
               enable_sizing=TRUE;
               enable_stretch=TRUE;
               enable_tilt=TRUE;

               mSPRHSIZ.Word=RAM_PEEKW(mTMPADR.Word);   // Sprite Horizontal size
               mTMPADR.Word+=2;

               mSPRVSIZ.Word=RAM_PEEKW(mTMPADR.Word);   // Sprite Verticalal size
               mTMPADR.Word+=2;

               mSTRETCH.Word=RAM_PEEKW(mTMPADR.Word);   // Sprite stretch
               mTMPADR.Word+=2;

               mTILT.Word=RAM_PEEKW(mTMPADR.Word);      // Sprite tilt
               mTMPADR.Word+=2;

               cycles_used+=8*SPR_RDWR_CYC;
               break;

            default:
               break;
         }

         TRACE_SUSIE1("PaintSprites() SPRHSIZ $%04x",mSPRHSIZ.Word);
         TRACE_SUSIE1("PaintSprites() SPRVSIZ $%04x",mSPRVSIZ.Word);
         TRACE_SUSIE1("PaintSprites() STRETCH $%04x",mSTRETCH.Word);
         TRACE_SUSIE1("PaintSprites() TILT    $%04x",mTILT.Word);


         // Optional Palette reload

         if(!mSPRCTL1_ReloadPalette)
         {
            TRACE_SUSIE0("PaintSprites() Palette reloaded");
            for(int loop=0;loop<8;loop++)
            {
               UBYTE data_tmp=RAM_PEEK(mTMPADR.Word++);
               mPenIndex[loop*2]=(data_tmp>>4)&0x0f;
               mPenIndex[(loop*2)+1]=data_tmp&0x0f;
            }
            // Increment cycle count for the reads
            cycles_used+=8*SPR_RDWR_CYC;
         }

         // Now we can start painting

         // Quadrant drawing order is: SE,NE,NW,SW
         // start quadrant is given by sprite_control1:0 & 1

         // Setup screen start end variables

         int screen_h_start=(SWORD)mHOFF.Word;
         int screen_h_end=(SWORD)mHOFF.Word+SCREEN_WIDTH;
         int screen_v_start=(SWORD)mVOFF.Word;
         int screen_v_end=(SWORD)mVOFF.Word+SCREEN_HEIGHT;

         int world_h_mid=screen_h_start+0x8000+(SCREEN_WIDTH/2);
         int world_v_mid=screen_v_start+0x8000+(SCREEN_HEIGHT/2);

         TRACE_SUSIE2("PaintSprites() screen_h_start $%04x screen_h_end $%04x",screen_h_start,screen_h_end);
         TRACE_SUSIE2("PaintSprites() screen_v_start $%04x screen_v_end $%04x",screen_v_start,screen_v_end);
         TRACE_SUSIE2("PaintSprites() world_h_mid    $%04x world_v_mid  $%04x",world_h_mid,world_v_mid);

         bool superclip=FALSE;
         int quadrant=0;
         int hsign,vsign;
#ifdef MY_NO_STATIC
         int vquadflip[4]={1,0,3,2};
         int hquadflip[4]={3,2,1,0};
#endif
         if(mSPRCTL1_StartLeft)
         {
            if(mSPRCTL1_StartUp) quadrant=2; else quadrant=3;
         }
         else
         {
            if(mSPRCTL1_StartUp) quadrant=1; else quadrant=0;
         }
         TRACE_SUSIE1("PaintSprites() Quadrant=%d",quadrant);

         // Check ref is inside screen area

         if((SWORD)mHPOSSTRT.Word<screen_h_start || (SWORD)mHPOSSTRT.Word>=screen_h_end ||
               (SWORD)mVPOSSTRT.Word<screen_v_start || (SWORD)mVPOSSTRT.Word>=screen_v_end) superclip=TRUE;

         TRACE_SUSIE1("PaintSprites() Superclip=%d",superclip);


         // Quadrant mapping is:    SE  NE  NW  SW
         //                     0   1   2   3
         // hsign               +1  +1  -1  -1
         // vsign               +1  -1  -1  +1
         //
         //
         //     2 | 1
         //     -------
         //      3 | 0
         //

         // Loop for 4 quadrants

         for(int loop=0;loop<4;loop++)
         {
            TRACE_SUSIE1("PaintSprites() -------- Rendering Quadrant %03d --------",quadrant);

            int sprite_v=mVPOSSTRT.Word;
            int sprite_h=mHPOSSTRT.Word;

            bool render=FALSE;

            // Set quadrand multipliers
            hsign=(quadrant==0 || quadrant==1)?1:-1;
            vsign=(quadrant==0 || quadrant==3)?1:-1;

            // Preflip      TRACE_SUSIE2("PaintSprites() hsign=%d vsign=%d",hsign,vsign);

            //Use h/v flip to invert v/hsign

            if(mSPRCTL0_Vflip) vsign=-vsign;
            if(mSPRCTL0_Hflip) hsign=-hsign;

            TRACE_SUSIE2("PaintSprites() Hflip=%d Vflip=%d",mSPRCTL0_Hflip,mSPRCTL0_Vflip);
            TRACE_SUSIE2("PaintSprites() Hsign=%d   Vsign=%d",hsign,vsign);
            TRACE_SUSIE2("PaintSprites() Hpos =%04x Vpos =%04x",mHPOSSTRT.Word,mVPOSSTRT.Word);
            TRACE_SUSIE2("PaintSprites() Hsizoff =%04x Vsizoff =%04x",mHSIZOFF.Word,mVSIZOFF.Word);

            // Two different rendering algorithms used, on-screen & superclip
            // when on screen we draw in x until off screen then skip to next
            // line, BUT on superclip we draw all the way to the end of any
            // given line checking each pixel is on screen.

            if(superclip)
            {
               // Check on the basis of each quad, we only render the quad
               // IF the screen is in the quad, relative to the centre of
               // the screen which is calculated below.

               // Quadrant mapping is:  SE  NE  NW  SW
               //                       0   1   2   3
               // hsign             +1  +1  -1  -1
               // vsign             +1  -1  -1  +1
               //
               //
               //       2 | 1
               //     -------
               //      3 | 0
               //
               // Quadrant mapping for superclipping must also take into account
               // the hflip, vflip bits & negative tilt to be able to work correctly
               //
               int  modquad=quadrant;
#ifndef MY_NO_STATIC
               static int vquadflip[4]={1,0,3,2};
               static int hquadflip[4]={3,2,1,0};
#endif
               
               if(mSPRCTL0_Vflip) modquad=vquadflip[modquad];
               if(mSPRCTL0_Hflip) modquad=hquadflip[modquad];

               // This is causing Eurosoccer to fail!!
               //                   if(enable_tilt && mTILT.Word&0x8000) modquad=hquadflip[modquad];

               switch(modquad)
               {
                  case 3:
                     if((sprite_h>=screen_h_start || sprite_h<world_h_mid) && (sprite_v<screen_v_end   || sprite_v>world_v_mid)) render=TRUE;
                     break;
                  case 2:
                     if((sprite_h>=screen_h_start || sprite_h<world_h_mid) && (sprite_v>=screen_v_start || sprite_v<world_v_mid)) render=TRUE;
                     break;
                  case 1:
                     if((sprite_h<screen_h_end   || sprite_h>world_h_mid) && (sprite_v>=screen_v_start || sprite_v<world_v_mid)) render=TRUE;
                     break;
                  default:
                     if((sprite_h<screen_h_end   || sprite_h>world_h_mid) && (sprite_v<screen_v_end   || sprite_v>world_v_mid)) render=TRUE;
                     break;
               }
            }
            else
            {
               render=TRUE;
            }

            // Is this quad to be rendered ??

            TRACE_SUSIE1("PaintSprites() Render status %d",render);
#ifdef MY_NO_STATIC
             int pixel_height=0;
             int pixel_width=0;
             int pixel=0;
             int hoff=0,voff=0;
             int hloop=0,vloop=0;
             bool onscreen=0;
#else
            static int pixel_height=0;
            static int pixel_width=0;
            static int pixel=0;
            static int hoff=0,voff=0;
            static int hloop=0,vloop=0;
            static bool onscreen=0;
            static int vquadoff=0;
            static int hquadoff=0;
#endif
             
            if(render)
            {
               // Set the vertical position & offset
               voff=(SWORD)mVPOSSTRT.Word-screen_v_start;

               // Zero the stretch,tilt & acum values
               mTILTACUM.Word=0;

               // Perform the SIZOFF
               if(vsign==1) mVSIZACUM.Word=mVSIZOFF.Word; else mVSIZACUM.Word=0;

               // Take the sign of the first quad (0) as the basic
               // sign, all other quads drawing in the other direction
               // get offset by 1 pixel in the other direction, this
               // fixes the squashed look on the multi-quad sprites.
               //                   if(vsign==-1 && loop>0) voff+=vsign;
               if(loop==0)  vquadoff=vsign;
               if(vsign!=vquadoff) voff+=vsign;

               for(;;)
               {
                  // Vertical scaling is done here
                  mVSIZACUM.Word+=mSPRVSIZ.Word;
                  pixel_height=mVSIZACUM.Byte.High;
                  mVSIZACUM.Byte.High=0;

                  // Update the next data line pointer and initialise our line
                  mSPRDOFF.Word=(UWORD)LineInit(0);

                  // If 1 == next quad, ==0 end of sprite, anyways its END OF LINE
                  if(mSPRDOFF.Word==1)      // End of quad
                  {
                     mSPRDLINE.Word+=mSPRDOFF.Word;
                     break;
                  }

                  if(mSPRDOFF.Word==0)      // End of sprite
                  {
                     loop=4;        // Halt the quad loop
                     break;
                  }

                  // Draw one horizontal line of the sprite
                  for(vloop=0;vloop<pixel_height;vloop++)
                  {
                     // Early bailout if the sprite has moved off screen, terminate quad
                     if(vsign==1 && voff>=SCREEN_HEIGHT)    break;
                     if(vsign==-1 && voff<0)    break;

                     // Only allow the draw to take place if the line is visible
                     if(voff>=0 && voff<SCREEN_HEIGHT)
                     {
                        // Work out the horizontal pixel start position, start + tilt
                        mHPOSSTRT.Word+=((SWORD)mTILTACUM.Word>>8);
                        mTILTACUM.Byte.High=0;
                        hoff=(int)((SWORD)mHPOSSTRT.Word)-screen_h_start;

                        // Zero/Force the horizontal scaling accumulator
                        if(hsign==1) mHSIZACUM.Word=mHSIZOFF.Word; else mHSIZACUM.Word=0;

                        // Take the sign of the first quad (0) as the basic
                        // sign, all other quads drawing in the other direction
                        // get offset by 1 pixel in the other direction, this
                        // fixes the squashed look on the multi-quad sprites.
                        //                              if(hsign==-1 && loop>0) hoff+=hsign;
                        if(loop==0) hquadoff=hsign;
                        if(hsign!=hquadoff) hoff+=hsign;

                        // Initialise our line
                        LineInit(voff);
                        onscreen=FALSE;

                        // Now render an individual destination line
                        while((pixel=LineGetPixel())!=LINE_END)
                        {
                           // This is allowed to update every pixel
                           mHSIZACUM.Word+=mSPRHSIZ.Word;
                           pixel_width=mHSIZACUM.Byte.High;
                           mHSIZACUM.Byte.High=0;

                           for(hloop=0;hloop<pixel_width;hloop++)
                           {
                              // Draw if onscreen but break loop on transition to offscreen
                              if(hoff>=0 && hoff<SCREEN_WIDTH)
                              {
                                 ProcessPixel(hoff,pixel);
                                 onscreen=everonscreen=TRUE;
                              }
                              else
                              {
                                 if(onscreen) break;
                              }
                              hoff+=hsign;
                           }
                        }
                     }
                     voff+=vsign;

                     // For every destination line we can modify SPRHSIZ & SPRVSIZ & TILTACUM
                     if(enable_stretch)
                     {
                        mSPRHSIZ.Word+=mSTRETCH.Word;
                        //                              if(mSPRSYS_VStretch) mSPRVSIZ.Word+=mSTRETCH.Word;
                     }
                     if(enable_tilt)
                     {
                        // Manipulate the tilt stuff
                        mTILTACUM.Word+=mTILT.Word;
                     }
                  }
                  // According to the docs this increments per dest line
                  // but only gets set when the source line is read
                  if(mSPRSYS_VStretch) mSPRVSIZ.Word+=mSTRETCH.Word*pixel_height;

                  // Update the line start for our next run thru the loop
                  mSPRDLINE.Word+=mSPRDOFF.Word;
               }
            }
            else
            {
               // Skip thru data to next quad
               for(;;)
               {
                  // Read the start of line offset

                  mSPRDOFF.Word=(UWORD)LineInit(0);

                  // We dont want to process data so mSPRDLINE is useless to us
                  mSPRDLINE.Word+=mSPRDOFF.Word;

                  // If 1 == next quad, ==0 end of sprite, anyways its END OF LINE

                  if(mSPRDOFF.Word==1) break;   // End of quad
                  if(mSPRDOFF.Word==0)      // End of sprite
                  {
                     loop=4;        // Halt the quad loop
                     break;
                  }

               }
            }

            // Increment quadrant and mask to 2 bit value (0-3)
            quadrant++;
            quadrant&=0x03;
         }

         // Write the collision depositary if required

         if(!mSPRCOLL_Collide && !mSPRSYS_NoCollide)
         {
            switch(mSPRCTL0_Type)
            {
               case sprite_xor_shadow:
               case sprite_boundary:
               case sprite_normal:
               case sprite_boundary_shadow:
               case sprite_shadow:
                  {
                     UWORD coldep=mSCBADR.Word+mCOLLOFF.Word;
                     RAM_POKE(coldep,(UBYTE)mCollision);
                     TRACE_SUSIE2("PaintSprites() COLLOFF=$%04x SCBADR=$%04x",mCOLLOFF.Word,mSCBADR.Word);
                     TRACE_SUSIE2("PaintSprites() Wrote $%02x to SCB collision depositary at $%04x",(UBYTE)mCollision,coldep);
                  }
                  break;
               default:
                  break;
            }
         }

         if(mEVERON)
         {
            UWORD coldep=mSCBADR.Word+mCOLLOFF.Word;
            UBYTE coldat=RAM_PEEK(coldep);
            if(!everonscreen) coldat|=0x80; else coldat&=0x7f;
            RAM_POKE(coldep,coldat);
            TRACE_SUSIE0("PaintSprites() EVERON IS ACTIVE");
            TRACE_SUSIE2("PaintSprites() Wrote $%02x to SCB collision depositary at $%04x",coldat,coldep);
         }

         // Perform Sprite debugging if required, single step on sprite draw
         if(gSingleStepModeSprites)
         {
            char message[256];
            sprintf(message,"CSusie:PaintSprites() - Rendered Sprite %03d",sprcount);
            if(!gError->Warning(message)) gSingleStepModeSprites=0;
         }
      }
      else
      {
         TRACE_SUSIE0("PaintSprites() mSPRCTL1.Bits.SkipSprite==TRUE");
      }

      // Increase sprite number
      sprcount++;

      // Check if we abort after 1st sprite is complete

      //        if(mSPRSYS.Read.StopOnCurrent)
      //        {
      //            mSPRSYS.Read.Status=0;  // Engine has finished
      //            mSPRGO=FALSE;
      //            break;
      //        }

      // Check sprcount for looping SCB, random large number chosen
      if(sprcount>4096)
      {
         // Stop the system, otherwise we may just come straight back in.....
         gSystemHalt=TRUE;
         // Display warning message
         gError->Warning("CSusie:PaintSprites(): Single draw sprite limit exceeded (>4096). The SCB is most likely looped back on itself. Reset/Exit is recommended");
         // Signal error to the caller
         return 0;
      }
   }
   while(1);

   // Fudge factor to fix many flickering issues, also the keypress
   // problem with Hard Drivin and the strange pause in Dirty Larry.
   //   cycles_used>>=2;

   return cycles_used;