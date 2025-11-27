#include "key.h"
#include "./SYSTEM/delay/delay.h"

typedef struct
{
    uint8_t last_state;         
    uint8_t current_state;      
    uint32_t press_start_time;  
    uint8_t is_pressed;         
    uint8_t event_flag;         
} key_state_t;

static key_state_t key_states[4] = {0};

void key_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    KEY1_GPIO_CLK_ENABLE(); 
    KEY2_GPIO_CLK_ENABLE(); 
    KEY3_GPIO_CLK_ENABLE(); 
    KEY4_GPIO_CLK_ENABLE(); 

    
    gpio_init_struct.Pin = KEY1_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(KEY1_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = KEY2_GPIO_PIN;
    HAL_GPIO_Init(KEY2_GPIO_PORT, &gpio_init_struct);

    
    gpio_init_struct.Pin = KEY3_GPIO_PIN;
    gpio_init_struct.Pull = GPIO_PULLUP;  
    HAL_GPIO_Init(KEY3_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = KEY4_GPIO_PIN;
    HAL_GPIO_Init(KEY4_GPIO_PORT, &gpio_init_struct);

    
    for (int i = 0; i < 4; i++)
    {
        key_states[i].last_state = 0;
        key_states[i].current_state = 0;
        key_states[i].press_start_time = 0;
        key_states[i].is_pressed = 0;
        key_states[i].event_flag = 0;
    }
}

static uint8_t key_read_state(uint8_t key_index)
{
    switch (key_index)
    {
        case 0: return KEY1_PRESSED;
        case 1: return KEY2_PRESSED;
        case 2: return KEY3_PRESSED;
        case 3: return KEY4_PRESSED;
        default: return 0;
    }
}

uint8_t key_get_pressed_key(void)
{
    if (KEY1_PRESSED) return 1;
    if (KEY2_PRESSED) return 2;
    if (KEY3_PRESSED) return 3;
    if (KEY4_PRESSED) return 4;
    return 0;
}

uint8_t key_scan(void)
{
    uint8_t key_event = KEY_EVENT_NONE;
    uint32_t current_time = HAL_GetTick();
    
    
    for (uint8_t i = 0; i < 4; i++)
    {
        key_state_t *ks = &key_states[i];
        
        
        ks->current_state = key_read_state(i);
        
        
        if (ks->current_state && !ks->last_state && !ks->is_pressed)
        {
            
            delay_ms(KEY_DEBOUNCE_TIME_MS);
            if (key_read_state(i))
            {
                ks->is_pressed = 1;
                ks->press_start_time = HAL_GetTick();
            }
        }
        
        
        else if (!ks->current_state && ks->last_state && ks->is_pressed)
        {
            
            delay_ms(KEY_DEBOUNCE_TIME_MS);
            if (!key_read_state(i))
            {
                
                uint32_t press_duration = current_time - ks->press_start_time;
                
                
                if (press_duration < KEY_LONG_PRESS_TIME_MS)
                {
                    
                    if (press_duration >= KEY_DEBOUNCE_TIME_MS)  
                    {
                        switch (i)
                        {
                            case 0: key_event = KEY1_SHORT_PRESS; break;
                            case 1: key_event = KEY2_SHORT_PRESS; break;
                            case 2: key_event = KEY3_SHORT_PRESS; break;
                            case 3: key_event = KEY4_SHORT_PRESS; break;
                        }
                        ks->event_flag = 1;
                    }
                }
                
                ks->is_pressed = 0;
                ks->press_start_time = 0;
            }
        }
        
        
        else if (ks->is_pressed && ks->current_state)
        {
            uint32_t press_duration = current_time - ks->press_start_time;
            
            
            if (press_duration >= KEY_LONG_PRESS_TIME_MS && !ks->event_flag)
            {
                switch (i)
                {
                    case 0: key_event = KEY1_LONG_PRESS; break;
                    case 1: key_event = KEY2_LONG_PRESS; break;
                    case 2: key_event = KEY3_LONG_PRESS; break;
                    case 3: key_event = KEY4_LONG_PRESS; break;
                }
                ks->event_flag = 1;  
            }
        }
        
        
        ks->last_state = ks->current_state;
        
        
        if (!ks->is_pressed && !ks->current_state)
        {
            ks->event_flag = 0;
        }
    }
    
    return key_event;
}
