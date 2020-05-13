void ili9341_write_frame_lynx(uint16_t* buffer, uint16_t* myPalette, uint8_t scale)
{
    short x, y;
    odroid_display_lock();
    //xTaskToNotify = xTaskGetCurrentTaskHandle();
    
    if (buffer == NULL)
    {
        // clear the buffer
        for (int i = 0; i < LINE_BUFFERS; ++i)
        {
            memset(line[i], 0, 320 * sizeof(uint16_t) * LINE_COUNT);
        }

        // clear the screen
        send_reset_drawing(0, 0, 320, 240);

        for (y = 0; y < 240; y += LINE_COUNT)
        {
            uint16_t* line_buffer = line_buffer_get();
            send_continue_line(line_buffer, 320, LINE_COUNT);
        }
    }
    else
    {
        uint16_t* framePtr = buffer;
        
        // scale = 0;

        if (scale)
        {
            const uint16_t displayWidth = 320;
            
            send_reset_drawing(0, /*(240-102*2)/2*/ 18, displayWidth, LYNX_GAME_HEIGHT*2);

            for (y = 0; y < LYNX_GAME_HEIGHT; y += 2)
            {
              uint16_t* line_buffer = line_buffer_get();
              uint16_t* line_buffer_ptr = line_buffer; 
              for (short i = 0; i < 2; ++i) // LINE_COUNT
              {
                  int index = (i*2) * displayWidth;

                  int bufferIndex = ((y + i) * LYNX_GAME_WIDTH);

                  for (x = 0; x < LYNX_GAME_WIDTH; ++x)
                  {
                    uint16_t val = framePtr[bufferIndex++];
                    line_buffer[index] = val;
                    line_buffer[index+1] = val;
                    line_buffer[index+displayWidth] = val;
                    line_buffer[index+displayWidth+1] = val;
                    index+=2;
                  }
              }
              /*
              {
                  for (x = 0; x < LYNX_GAME_WIDTH; ++x)
                  {
                    *line_buffer_ptr = *framePtr;
                    line_buffer_ptr+=2;
                    framePtr++;
                  }
                  line_buffer_ptr+=320;

                  for (x = 0; x < LYNX_GAME_WIDTH; ++x)
                  {
                    *line_buffer_ptr = *framePtr;
                    line_buffer_ptr+=2;
                    framePtr++;
                  }
              }
              */
              // display
              send_continue_line(line_buffer, displayWidth, 4);
            }
        }
        else
        {
            send_reset_drawing((320 / 2) - (LYNX_GAME_WIDTH / 2), (240 / 2) - (LYNX_GAME_HEIGHT / 2), LYNX_GAME_WIDTH, LYNX_GAME_HEIGHT);

            for (y = 0; y < LYNX_GAME_HEIGHT; y += LINE_COUNT)
            {
              int linesWritten = 0;
              uint16_t* line_buffer = line_buffer_get();

              for (short i = 0; i < LINE_COUNT; ++i)
              {
                  if((y + i) >= LYNX_GAME_HEIGHT)
                    break;

                  int index = (i) * LYNX_GAME_WIDTH;
                  int bufferIndex = ((y + i) * LYNX_GAME_WIDTH);

                  for (x = 0; x < LYNX_GAME_WIDTH; ++x)
                  {
                    //line_buffer[index++] = 22; // framePtr[bufferIndex++];//myPalette[framePtr[bufferIndex++]];
                    line_buffer[index++] = framePtr[bufferIndex++];
                  }

                  ++linesWritten;
              }

              // display
              send_continue_line(line_buffer, LYNX_GAME_WIDTH, linesWritten);
              // break;
            }
        }
    }

    //send_continue_wait();

    odroid_display_unlock();
}


typedef struct
{
   union
   {
      struct
      {
#ifdef MSB_FIRST
         uint8_t unused:8;
         uint8_t unused2:8;
         uint8_t unused3:4;
         uint8_t Blue:4;
         uint8_t Red:4;
         uint8_t Green:4;
#else
         uint8_t Green:4;
         uint8_t Red:4;
         uint8_t Blue:4;
#endif
      }Colours;
      uint32_t     Index;
   };
}TPALETTE;

void ili9341_write_frame_lynx_v2_original(uint8_t* buffer, uint32_t* myPalette)
{
    uint8_t* framePtr = buffer;
    TPALETTE    *mPaletteInLine;
    short x, y;
   send_reset_drawing((320 / 2) - (LYNX_GAME_WIDTH / 2), (240 / 2) - (LYNX_GAME_HEIGHT / 2), LYNX_GAME_WIDTH, LYNX_GAME_HEIGHT);
    for (y = 0; y < LYNX_GAME_HEIGHT; y += LINE_COUNT)
    {
      int linesWritten = 0;
      uint16_t* line_buffer = line_buffer_get();

      for (short i = 0; i < LINE_COUNT; ++i)
      {
          if((y + i) >= LYNX_GAME_HEIGHT)
            break;
          mPaletteInLine = framePtr;
          framePtr+=64;
          int index = (i) * LYNX_GAME_WIDTH;

          for (x = 0; x < LYNX_GAME_WIDTH/2; ++x)
          {
            uint8_t source=*framePtr;
            framePtr++;
            uint16_t value1 = myPalette[mPaletteInLine[source>>4].Index];
            uint16_t value2 = myPalette[mPaletteInLine[source&0x0f].Index];
            //uint16_t value1 = mPaletteInLine[source>>4].Index;
            //uint16_t value2 = mPaletteInLine[source&0x0f].Index;
             
            line_buffer[index++] = value1;
            line_buffer[index++] = value2;
            //line_buffer[index++] = source>>4;
            //line_buffer[index++] = source&0x0f;
          }

          ++linesWritten;
      }

      // display
      send_continue_line(line_buffer, LYNX_GAME_WIDTH, linesWritten);
    }
}

// 2x2
void ili9341_write_frame_lynx_v2_mode0(uint8_t* buffer, uint32_t* myPalette) {
    uint8_t* framePtr = buffer;
    TPALETTE    *mPaletteInLine;
    uint16_t x, y;
    send_reset_drawing(0, /*(240-102*2)/2*/ 18, LYNX_GAME_WIDTH*2, LYNX_GAME_HEIGHT*2);
    for (y = 0; y < LYNX_GAME_HEIGHT; y += 2)
    {
      uint16_t* line_buffer = line_buffer_get();
      uint16_t* line_buffer_ptr = line_buffer; 
      for (short i = 0; i < 2; ++i) // LINE_COUNT
      {
          int index = (i*2) * LYNX_GAME_WIDTH*2;
          mPaletteInLine = framePtr;
          framePtr+=64;
          for (x = 0; x < LYNX_GAME_WIDTH / 2; ++x)
          {
            uint8_t source=*framePtr;
            framePtr++;
            uint16_t value1 = myPalette[mPaletteInLine[source>>4].Index];
            uint16_t value2 = myPalette[mPaletteInLine[source&0x0f].Index];
            
            line_buffer[index] = value1;
            line_buffer[index+1] = value1;
            line_buffer[index+LYNX_GAME_WIDTH*2] = value1;
            line_buffer[index+LYNX_GAME_WIDTH*2+1] = value1;
            index+=2;
            line_buffer[index] = value2;
            line_buffer[index+1] = value2;
            line_buffer[index+LYNX_GAME_WIDTH*2] = value2;
            line_buffer[index+LYNX_GAME_WIDTH*2+1] = value2;
            index+=2;
          }
      }
      send_continue_line(line_buffer, LYNX_GAME_WIDTH*2, 4);
    }
}

// 2x1
void ili9341_write_frame_lynx_v2_mode1(uint8_t* buffer, uint32_t* myPalette) {
    uint8_t* framePtr = buffer;
    TPALETTE    *mPaletteInLine;
    uint16_t x, y;
    send_reset_drawing(0, /*(240-102*2)/2*/ 18, LYNX_GAME_WIDTH*2, LYNX_GAME_HEIGHT*2);
    for (y = 0; y < LYNX_GAME_HEIGHT; y += 2)
    {
      uint16_t* line_buffer = line_buffer_get();
      uint16_t* line_buffer_ptr = line_buffer; 
      for (short i = 0; i < 2; ++i) // LINE_COUNT
      {
          int index = (i*2) * LYNX_GAME_WIDTH*2;
          mPaletteInLine = framePtr;
          framePtr+=64;
          for (x = 0; x < LYNX_GAME_WIDTH / 2; ++x)
          {
            uint8_t source=*framePtr;
            framePtr++;
            uint16_t value1 = myPalette[mPaletteInLine[source>>4].Index];
            uint16_t value2 = myPalette[mPaletteInLine[source&0x0f].Index];
            
            line_buffer[index] = value1;
            line_buffer[index+1] = value1;
            //line_buffer[index+LYNX_GAME_WIDTH*2] = value1;
            //line_buffer[index+LYNX_GAME_WIDTH*2+1] = value1;
            index+=2;
            line_buffer[index] = value2;
            line_buffer[index+1] = value2;
            //line_buffer[index+LYNX_GAME_WIDTH*2] = value2;
            //line_buffer[index+LYNX_GAME_WIDTH*2+1] = value2;
            index+=2;
          }
      }
      send_continue_line(line_buffer, LYNX_GAME_WIDTH*2, 4);
    }
}

// 1x2
void ili9341_write_frame_lynx_v2_mode2(uint8_t* buffer, uint32_t* myPalette) {
    uint8_t* framePtr = buffer;
    TPALETTE    *mPaletteInLine;
    short x, y;
    const uint16_t displayWidth = 320;        
    send_reset_drawing(0, /*(240-102*2)/2*/ 18, displayWidth, LYNX_GAME_HEIGHT*2);
    for (y = 0; y < LYNX_GAME_HEIGHT; y += 2)
    {
      uint16_t* line_buffer = line_buffer_get();
      uint16_t* line_buffer_ptr = line_buffer; 
      for (short i = 0; i < 2; ++i) // LINE_COUNT
      {
          int index = (i*2) * displayWidth;
          mPaletteInLine = framePtr;
          framePtr+=64;
          for (x = 0; x < LYNX_GAME_WIDTH / 2; ++x)
          {
            uint8_t source=*framePtr;
            framePtr++;
            uint16_t value1 = myPalette[mPaletteInLine[source>>4].Index];
            uint16_t value2 = myPalette[mPaletteInLine[source&0x0f].Index];
            
            line_buffer[index] = value1;
            //line_buffer[index+1] = value1;
            line_buffer[index+displayWidth] = value1;
            //line_buffer[index+displayWidth+1] = value1;
            index+=2;
            line_buffer[index] = value2;
            //line_buffer[index+1] = value2;
            line_buffer[index+displayWidth] = value2;
            //line_buffer[index+displayWidth+1] = value2;
            index+=2;
          }
      }
      send_continue_line(line_buffer, displayWidth, 4);
    }
}

// 1x1
void ili9341_write_frame_lynx_v2_mode3(uint8_t* buffer, uint32_t* myPalette) {
    uint8_t* framePtr = buffer;
    TPALETTE    *mPaletteInLine;
    short x, y;
    const uint16_t displayWidth = 320;        
    send_reset_drawing(0, /*(240-102*2)/2*/ 18, displayWidth, LYNX_GAME_HEIGHT*2);
    for (y = 0; y < LYNX_GAME_HEIGHT; y += 2)
    {
      uint16_t* line_buffer = line_buffer_get();
      uint16_t* line_buffer_ptr = line_buffer; 
      for (short i = 0; i < 2; ++i) // LINE_COUNT
      {
          int index = (i*2) * displayWidth;
          mPaletteInLine = framePtr;
          framePtr+=64;
          for (x = 0; x < LYNX_GAME_WIDTH / 2; ++x)
          {
            uint8_t source=*framePtr;
            framePtr++;
            uint16_t value1 = myPalette[mPaletteInLine[source>>4].Index];
            uint16_t value2 = myPalette[mPaletteInLine[source&0x0f].Index];
            
            line_buffer[index] = value1;
            //line_buffer[index+1] = value1;
            //line_buffer[index+displayWidth] = value1;
            //line_buffer[index+displayWidth+1] = value1;
            index+=2;
            line_buffer[index] = value2;
            //line_buffer[index+1] = value2;
            //line_buffer[index+displayWidth] = value2;
            //line_buffer[index+displayWidth+1] = value2;
            index+=2;
          }
      }
      send_continue_line(line_buffer, displayWidth, 4);
    }
}

void ili9341_write_frame_lynx_v2_original_rotate_R(uint8_t* buffer, uint32_t* myPalette)
{
    TPALETTE    *mPaletteInLine;
    short x, y;
   send_reset_drawing((320 / 2) - (LYNX_GAME_HEIGHT / 2), (240 / 2) - (LYNX_GAME_WIDTH / 2),LYNX_GAME_HEIGHT , LYNX_GAME_WIDTH);
    for (x = 0; x < LYNX_GAME_WIDTH; x+=LINE_COUNT)
    {
      int linesWritten = 0;
      uint16_t* line_buffer = line_buffer_get();

      for (short i = 0; i < LINE_COUNT; ++i)
      {
          if((x + i) >= LYNX_GAME_WIDTH)
            break;
          
          int index = (i) * LYNX_GAME_HEIGHT;

          for (y = LYNX_GAME_HEIGHT -1; y >=0; y-- )
          {
            uint16_t offset = (y * (64 + LYNX_GAME_WIDTH/2));
            mPaletteInLine = buffer + offset; 
            uint8_t source=*((uint8_t*)(buffer + offset + 64 + (x+i)/2));
            uint16_t value1;
            if ((x+i)%2==0) value1 = myPalette[mPaletteInLine[source>>4].Index];
            else value1 = myPalette[mPaletteInLine[source&0x0f].Index];
            
            //uint16_t value1 = myPalette[mPaletteInLine[source>>4].Index];
            //uint16_t value2 = myPalette[mPaletteInLine[source&0x0f].Index];
             
            line_buffer[index++] = value1;
            //line_buffer[index++] = value2;
          }

          ++linesWritten;
      }

      // display
      send_continue_line(line_buffer, LYNX_GAME_HEIGHT, linesWritten);
    }
}

void ili9341_write_frame_lynx_v2_original_rotate_L(uint8_t* buffer, uint32_t* myPalette)
{
    TPALETTE    *mPaletteInLine;
    short x, y;
   send_reset_drawing((320 / 2) - (LYNX_GAME_HEIGHT / 2), (240 / 2) - (LYNX_GAME_WIDTH / 2),LYNX_GAME_HEIGHT , LYNX_GAME_WIDTH);
    for (x = LYNX_GAME_WIDTH -1; x >=0 ; x-=LINE_COUNT)
    {
      int linesWritten = 0;
      uint16_t* line_buffer = line_buffer_get();

      for (short i = 0; i < LINE_COUNT; ++i)
      {
          if((x - i) < 0)
            break;
          
          int index = (i) * LYNX_GAME_HEIGHT;

          for (y = 0; y <LYNX_GAME_HEIGHT; y++ )
          {
            uint16_t offset = (y * (64 + LYNX_GAME_WIDTH/2));
            mPaletteInLine = buffer + offset; 
            uint8_t source=*((uint8_t*)(buffer + offset + 64 + (x-i)/2));
            uint16_t value1;
            if ((x-i)%2==0) value1 = myPalette[mPaletteInLine[source>>4].Index];
            else value1 = myPalette[mPaletteInLine[source&0x0f].Index];
            
            //uint16_t value1 = myPalette[mPaletteInLine[source>>4].Index];
            //uint16_t value2 = myPalette[mPaletteInLine[source&0x0f].Index];
             
            line_buffer[index++] = value1;
            //line_buffer[index++] = value2;
          }

          ++linesWritten;
      }

      // display
      send_continue_line(line_buffer, LYNX_GAME_HEIGHT, linesWritten);
    }
}

void ili9341_write_frame_lynx_v2_mode0_rotate_R(uint8_t* buffer, uint32_t* myPalette)
{
    TPALETTE    *mPaletteInLine;
    short x, y;
    
   send_reset_drawing(84, 0, LYNX_GAME_HEIGHT + LYNX_GAME_HEIGHT/2 , 240);
    for (x = 0; x < LYNX_GAME_WIDTH; x+=2)
    {
      int linesWritten = 0;
      uint16_t* line_buffer = line_buffer_get();

      for (short i = 0; i < 2; ++i)
      {
          if((x + i) >= LYNX_GAME_WIDTH)
            break;
          
          int index = (i) * (LYNX_GAME_HEIGHT + LYNX_GAME_HEIGHT/2);

          for (y = LYNX_GAME_HEIGHT -1; y >=0; y-- )
          {
            uint16_t offset = (y * (64 + LYNX_GAME_WIDTH/2));
            mPaletteInLine = buffer + offset; 
            uint8_t source=*((uint8_t*)(buffer + offset + 64 + (x+i)/2));
            uint16_t value1;
            if ((x+i)%2==0) value1 = myPalette[mPaletteInLine[source>>4].Index];
            else value1 = myPalette[mPaletteInLine[source&0x0f].Index];
             
            line_buffer[index++] = value1;
            if ((y%3)==2) line_buffer[index++] = value1;
          }

          ++linesWritten;
      }
      memcpy(&line_buffer[(2) * (LYNX_GAME_HEIGHT + LYNX_GAME_HEIGHT/2)],
             &line_buffer[(1) * (LYNX_GAME_HEIGHT + LYNX_GAME_HEIGHT/2)],
         (LYNX_GAME_HEIGHT + LYNX_GAME_HEIGHT/2) * 2);
     //memset(&line_buffer[(2) * (LYNX_GAME_HEIGHT + LYNX_GAME_HEIGHT/2)], 0,
     //    (LYNX_GAME_HEIGHT + LYNX_GAME_HEIGHT/2) * 2);
      ++linesWritten;
      
      // display
      send_continue_line(line_buffer, LYNX_GAME_HEIGHT + LYNX_GAME_HEIGHT/2, linesWritten);
    }
}

void ili9341_write_frame_lynx_v2_mode0_rotate_L(uint8_t* buffer, uint32_t* myPalette)
{
    TPALETTE    *mPaletteInLine;
    short x, y;
    
   send_reset_drawing(84, 0, LYNX_GAME_HEIGHT + LYNX_GAME_HEIGHT/2 , 240);
    for (x = LYNX_GAME_WIDTH -1; x >=0 ; x-=2)
    {
      int linesWritten = 0;
      uint16_t* line_buffer = line_buffer_get();

      for (short i = 0; i < 2; ++i)
      {
          if((x - i) < 0)
            break;
          
          int index = (i) * (LYNX_GAME_HEIGHT + LYNX_GAME_HEIGHT/2);

          for (y = 0; y <LYNX_GAME_HEIGHT; y++ )
          {
            uint16_t offset = (y * (64 + LYNX_GAME_WIDTH/2));
            mPaletteInLine = buffer + offset; 
            uint8_t source=*((uint8_t*)(buffer + offset + 64 + (x-i)/2));
            uint16_t value1;
            if ((x-i)%2==0) value1 = myPalette[mPaletteInLine[source>>4].Index];
            else value1 = myPalette[mPaletteInLine[source&0x0f].Index];
             
            line_buffer[index++] = value1;
            if ((y%3)==2) line_buffer[index++] = value1;
          }

          ++linesWritten;
      }
      memcpy(&line_buffer[(2) * (LYNX_GAME_HEIGHT + LYNX_GAME_HEIGHT/2)],
             &line_buffer[(1) * (LYNX_GAME_HEIGHT + LYNX_GAME_HEIGHT/2)],
         (LYNX_GAME_HEIGHT + LYNX_GAME_HEIGHT/2) * 2);
     //memset(&line_buffer[(2) * (LYNX_GAME_HEIGHT + LYNX_GAME_HEIGHT/2)], 0,
     //    (LYNX_GAME_HEIGHT + LYNX_GAME_HEIGHT/2) * 2);
      ++linesWritten;
      
      // display
      send_continue_line(line_buffer, LYNX_GAME_HEIGHT + LYNX_GAME_HEIGHT/2, linesWritten);
    }
}


void ili9341_write_frame_lynx_v2(uint8_t* buffer, uint32_t* myPalette, uint8_t scale, uint8_t filtering)
{
    odroid_display_lock();
    //xTaskToNotify = xTaskGetCurrentTaskHandle();
    
    if (buffer == NULL)
    {
        // clear the buffer
        for (int i = 0; i < LINE_BUFFERS; ++i)
        {
            memset(line[i], 0, 320 * sizeof(uint16_t) * LINE_COUNT);
        }

        // clear the screen
        send_reset_drawing(0, 0, 320, 240);

        for (short y = 0; y < 240; y += LINE_COUNT)
        {
            uint16_t* line_buffer = line_buffer_get();
            send_continue_line(line_buffer, 320, LINE_COUNT);
        }
    }
    else
    {
        if (scale)
        {
            switch (filtering) {
            case 0:
                ili9341_write_frame_lynx_v2_mode0(buffer, myPalette);
            break;
            case 1:
                ili9341_write_frame_lynx_v2_mode1(buffer, myPalette);
            break;
            case 2:
                ili9341_write_frame_lynx_v2_mode2(buffer, myPalette);
            break;
            case 3:
                ili9341_write_frame_lynx_v2_mode3(buffer, myPalette);
            break;  
            }
        }
        else
        {
            ili9341_write_frame_lynx_v2_original(buffer, myPalette);
        }
    }

    //send_continue_wait();

    odroid_display_unlock();
}