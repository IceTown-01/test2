

#include "lcd.h"

void ShowStaticImage_Example(void)
{
    
    
    
}

void ShowDynamicImage_Example(void)
{
    uint16_t img_buffer[LCD_HEIGHT * LCD_WIDTH];
    uint32_t x, y;
    
    
    for (y = 0; y < LCD_HEIGHT; y++)
    {
        for (x = 0; x < LCD_WIDTH; x++)
        {
            
            uint16_t color = ((x + y) % 32 < 16) ? 0xFFFF : 0x0000;
            img_buffer[y * LCD_WIDTH + x] = color;
        }
    }
    
    
    LCD_ShowImage(img_buffer);
}


