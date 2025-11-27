#ifndef __DISPLAY_H
#define __DISPLAY_H

#include "./SYSTEM/sys/sys.h"
#include "lcd.h"
#include "image_logo.h"

#define DISPLAY_MODE_X           0       
#define DISPLAY_MODE_Y           0       
#define DISPLAY_MODE_WIDTH       126     
#define DISPLAY_MODE_HEIGHT      174     

#define DISPLAY_LEVEL_X          0       
#define DISPLAY_LEVEL_Y          174     
#define DISPLAY_LEVEL_WIDTH      126     
#define DISPLAY_LEVEL_HEIGHT     120     

#define DISPLAY_TIME_TEXT_X      0
#define DISPLAY_TIME_TEXT_Y      (DISPLAY_LEVEL_Y + 9)
#define DISPLAY_TIME_TEXT_WIDTH  70
#define DISPLAY_TIME_TEXT_HEIGHT 30

#define MODE_MIN                 1       
#define MODE_MAX                 5       
#define MODE_COUNT               5       

typedef enum
{
    MODE_1 = 1,     
    MODE_2 = 2,     
    MODE_3 = 3,     
    MODE_4 = 4,     
    MODE_5 = 5      
} mode_t;

#define LEVEL_MIN                1       
#define LEVEL_MAX                5       
#define LEVEL_COUNT              5       

void display_init(void);

void display_show_mode(uint8_t mode);

void display_show_level(uint8_t level);
void display_show_time_text(uint32_t remaining_ms, uint32_t total_ms);

void display_clear(void);

void display_refresh(uint8_t mode, uint8_t level);

#endif
