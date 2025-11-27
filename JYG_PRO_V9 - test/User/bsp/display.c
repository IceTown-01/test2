#include "display.h"
#include <stdio.h>
#include <string.h>

static const unsigned char* mode_images[MODE_COUNT] = 
{
    gImage_mode1_126x174,  
    gImage_mode2_126x174,  
    gImage_mode3_126x174,  
    gImage_mode4_126x174,  
    gImage_mode5_126x174   
};

static const unsigned char* level_images[LEVEL_COUNT] = 
{
    gImage_dang1,  
    gImage_dang2,  
    gImage_dang3,  
    gImage_dang4,  
    gImage_dang5   
};

#define TIME_DIGIT_WIDTH              10U
#define TIME_COLON_WIDTH              4U
#define TIME_FONT_HEIGHT              20U
#define TIME_CHAR_SPACING_DEFAULT     2U
#define TIME_CHAR_SPACING_NARROW      1U
#define TIME_TEXT_MAX_CHARS           5U
#define TIME_TEXT_MAX_WIDTH           (TIME_TEXT_MAX_CHARS * TIME_DIGIT_WIDTH + (TIME_TEXT_MAX_CHARS - 1U) * TIME_CHAR_SPACING_DEFAULT)
#define TIME_TEXT_BACKGROUND_GRAY     0x0FU
#define TIME_TEXT_FOREGROUND_GRAY     0x00U
#define TIME_TEXT_COLON_GRAY          0x03U

static uint8_t s_time_gray_buffer[((TIME_TEXT_MAX_WIDTH * TIME_FONT_HEIGHT) + 1U) / 2U];

#define SEG_A  (1U << 0)
#define SEG_B  (1U << 1)
#define SEG_C  (1U << 2)
#define SEG_D  (1U << 3)
#define SEG_E  (1U << 4)
#define SEG_F  (1U << 5)
#define SEG_G  (1U << 6)

static const uint8_t s_digit_segments[10] =
{
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,             
    SEG_B | SEG_C,                                             
    SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,                     
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,                     
    SEG_B | SEG_C | SEG_F | SEG_G,                             
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,                     
    SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,             
    SEG_A | SEG_B | SEG_C,                                     
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,     
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G              
};

#define SEG_THICKNESS_HORIZONTAL     2U
#define SEG_THICKNESS_VERTICAL       2U
#define SEG_UPPER_VERTICAL_Y         2U
#define SEG_UPPER_VERTICAL_HEIGHT    7U
#define SEG_LOWER_VERTICAL_Y         11U
#define SEG_LOWER_VERTICAL_HEIGHT    7U
#define SEG_MIDDLE_Y                 9U
#define SEG_COLON_TOP_Y              5U
#define SEG_COLON_BOTTOM_Y           12U
#define SEG_COLON_DOT_HEIGHT         3U

static inline uint8_t time_text_get_char_width(char ch)
{
    return (ch == ':') ? TIME_COLON_WIDTH : TIME_DIGIT_WIDTH;
}

static inline uint8_t time_text_get_spacing(char current, char next)
{
    (void)current;
    (void)next;
    if (current == ':' || next == ':')
    {
        return TIME_CHAR_SPACING_NARROW;
    }
    return TIME_CHAR_SPACING_DEFAULT;
}

static inline void time_text_set_pixel(uint8_t *buffer, uint16_t buffer_width, uint16_t x, uint16_t y, uint8_t gray)
{
    uint32_t pixel_index = (uint32_t)y * buffer_width + x;
    uint32_t byte_index = pixel_index >> 1;

    if ((pixel_index & 0x01U) == 0U)
    {
        buffer[byte_index] = (uint8_t)((buffer[byte_index] & 0x0FU) | ((gray & 0x0FU) << 4));
    }
    else
    {
        buffer[byte_index] = (uint8_t)((buffer[byte_index] & 0xF0U) | (gray & 0x0FU));
    }
}

static void time_text_fill_rect(uint8_t *buffer,
                                uint16_t buffer_width,
                                uint16_t origin_x,
                                uint16_t origin_y,
                                uint16_t rect_width,
                                uint16_t rect_height,
                                uint8_t gray)
{
    for (uint16_t dy = 0; dy < rect_height; dy++)
    {
        for (uint16_t dx = 0; dx < rect_width; dx++)
        {
            time_text_set_pixel(buffer, buffer_width, origin_x + dx, origin_y + dy, gray);
        }
    }
}

static void time_text_draw_digit(uint8_t *buffer, uint16_t buffer_width, uint16_t origin_x, uint8_t segments)
{
    uint8_t gray = TIME_TEXT_FOREGROUND_GRAY;

    if (segments & SEG_A)
    {
        time_text_fill_rect(buffer, buffer_width, origin_x + 1U, 0U, TIME_DIGIT_WIDTH - 2U, SEG_THICKNESS_HORIZONTAL, gray);
    }
    if (segments & SEG_B)
    {
        time_text_fill_rect(buffer, buffer_width,
                            origin_x + TIME_DIGIT_WIDTH - SEG_THICKNESS_VERTICAL,
                            SEG_UPPER_VERTICAL_Y,
                            SEG_THICKNESS_VERTICAL,
                            SEG_UPPER_VERTICAL_HEIGHT,
                            gray);
    }
    if (segments & SEG_C)
    {
        time_text_fill_rect(buffer, buffer_width,
                            origin_x + TIME_DIGIT_WIDTH - SEG_THICKNESS_VERTICAL,
                            SEG_LOWER_VERTICAL_Y,
                            SEG_THICKNESS_VERTICAL,
                            SEG_LOWER_VERTICAL_HEIGHT,
                            gray);
    }
    if (segments & SEG_D)
    {
        time_text_fill_rect(buffer, buffer_width,
                            origin_x + 1U,
                            TIME_FONT_HEIGHT - SEG_THICKNESS_HORIZONTAL,
                            TIME_DIGIT_WIDTH - 2U,
                            SEG_THICKNESS_HORIZONTAL,
                            gray);
    }
    if (segments & SEG_E)
    {
        time_text_fill_rect(buffer, buffer_width,
                            origin_x,
                            SEG_LOWER_VERTICAL_Y,
                            SEG_THICKNESS_VERTICAL,
                            SEG_LOWER_VERTICAL_HEIGHT,
                            gray);
    }
    if (segments & SEG_F)
    {
        time_text_fill_rect(buffer, buffer_width,
                            origin_x,
                            SEG_UPPER_VERTICAL_Y,
                            SEG_THICKNESS_VERTICAL,
                            SEG_UPPER_VERTICAL_HEIGHT,
                            gray);
    }
    if (segments & SEG_G)
    {
        time_text_fill_rect(buffer, buffer_width,
                            origin_x + 1U,
                            SEG_MIDDLE_Y,
                            TIME_DIGIT_WIDTH - 2U,
                            SEG_THICKNESS_HORIZONTAL,
                            gray);
    }
}

static void time_text_draw_colon(uint8_t *buffer, uint16_t buffer_width, uint16_t origin_x)
{
    uint8_t gray = TIME_TEXT_COLON_GRAY;
    uint16_t dot_width = 2U;
    uint16_t offset_x = (TIME_COLON_WIDTH > dot_width) ? ((TIME_COLON_WIDTH - dot_width) / 2U) : 0U;

    time_text_fill_rect(buffer, buffer_width,
                        origin_x + offset_x,
                        SEG_COLON_TOP_Y,
                        dot_width,
                        SEG_COLON_DOT_HEIGHT,
                        gray);
    time_text_fill_rect(buffer, buffer_width,
                        origin_x + offset_x,
                        SEG_COLON_BOTTOM_Y,
                        dot_width,
                        SEG_COLON_DOT_HEIGHT,
                        gray);
}

static void time_text_render_char(uint8_t *buffer, uint16_t buffer_width, uint16_t start_x, char ch)
{
    if (ch >= '0' && ch <= '9')
    {
        time_text_draw_digit(buffer, buffer_width, start_x, s_digit_segments[ch - '0']);
    }
    else if (ch == ':')
    {
        time_text_draw_colon(buffer, buffer_width, start_x);
    }
}

void display_init(void)
{
    
    LCD_Clear(0x0000);  
}

void display_show_mode(uint8_t mode)
{
    
    if (mode < MODE_MIN || mode > MODE_MAX)
    {
        return;
    }
    
    
    LCD_ShowPartialImage16Gray(
        DISPLAY_MODE_X, 
        DISPLAY_MODE_Y, 
        DISPLAY_MODE_WIDTH, 
        DISPLAY_MODE_HEIGHT, 
        mode_images[mode - 1]  
    );
}

void display_show_level(uint8_t level)
{
    
    if (level < LEVEL_MIN || level > LEVEL_MAX)
    {
        return;
    }

    LCD_ShowPartialImage16Gray(
        DISPLAY_LEVEL_X, 
        DISPLAY_LEVEL_Y, 
        DISPLAY_LEVEL_WIDTH, 
        DISPLAY_LEVEL_HEIGHT, 
        level_images[level - 1]  
    );

    LCD_ShowPartialImageBytes(126-40,DISPLAY_LEVEL_Y,40,40,gImage_shalou_40x40);
}

void display_show_time_text(uint32_t remaining_ms, uint32_t total_ms)
{
    (void)total_ms;

    uint16_t area_x = DISPLAY_TIME_TEXT_X + 8U;
    uint16_t area_y = DISPLAY_TIME_TEXT_Y;
    uint16_t area_right = area_x + DISPLAY_TIME_TEXT_WIDTH - 1U;
    uint16_t area_bottom = DISPLAY_TIME_TEXT_Y + DISPLAY_TIME_TEXT_HEIGHT - 1U;

    uint32_t minutes = remaining_ms / 60000U;
    uint32_t seconds = (remaining_ms / 1000U) % 60U;
    if (minutes > 99U)
    {
        minutes = 99U;
    }

    char str[6];
    str[0] = (char)('0' + (minutes / 10U));
    str[1] = (char)('0' + (minutes % 10U));
    str[2] = ':';
    str[3] = (char)('0' + (seconds / 10U));
    str[4] = (char)('0' + (seconds % 10U));
    str[5] = '\0';

    uint8_t char_count = (uint8_t)strlen(str);
    if (char_count > TIME_TEXT_MAX_CHARS)
    {
        char_count = TIME_TEXT_MAX_CHARS;
        str[char_count] = '\0';
    }

    uint16_t text_width = 0;
    uint16_t char_widths[TIME_TEXT_MAX_CHARS];
    for (uint8_t i = 0; i < char_count; i++)
    {
        char_widths[i] = time_text_get_char_width(str[i]);
        text_width += char_widths[i];
        if ((i + 1U) < char_count)
        {
            text_width += time_text_get_spacing(str[i], str[i + 1U]);
        }
    }
    uint16_t text_height = TIME_FONT_HEIGHT;

    uint16_t start_x = area_x;
    uint16_t start_y = area_y;

    if (DISPLAY_TIME_TEXT_WIDTH > text_width)
    {
        start_x += (DISPLAY_TIME_TEXT_WIDTH - text_width) / 2U;
    }
    if (DISPLAY_TIME_TEXT_HEIGHT > text_height)
    {
        start_y += (DISPLAY_TIME_TEXT_HEIGHT - text_height) / 2U;
    }
#if 0
    uint16_t background_right = start_x + text_width;
    if (background_right > area_right)
    {
        background_right = area_right;
    }

    
    LCD_FillRect(area_x, area_y, area_right, area_bottom, 0xFFFFU);
    if (background_right >= area_x)
    {
        LCD_FillRect(area_x, area_y, background_right, area_bottom, 0x0000U);
    }
#endif
    uint32_t pixel_count = (uint32_t)text_width * text_height;
    uint32_t buffer_bytes = (pixel_count + 1U) / 2U;
    uint8_t fill_byte = (uint8_t)((TIME_TEXT_BACKGROUND_GRAY << 4) | TIME_TEXT_BACKGROUND_GRAY);
    for (uint32_t i = 0; i < buffer_bytes; i++)
    {
        s_time_gray_buffer[i] = fill_byte;
    }

    uint16_t pen_x = 0;
    for (uint8_t i = 0; i < char_count; i++)
    {
        time_text_render_char(s_time_gray_buffer, text_width, pen_x, str[i]);
        pen_x += char_widths[i];
        if (i + 1U < char_count)
        {
            pen_x += time_text_get_spacing(str[i], str[i + 1U]);
        }
    }

    LCD_ShowPartialImage16Gray(start_x, start_y, text_width, text_height, s_time_gray_buffer);
}

void display_clear(void)
{
    LCD_Clear(0x0000);  
}

void display_refresh(uint8_t mode, uint8_t level)
{
    display_show_mode(mode);
    display_show_level(level);
}
