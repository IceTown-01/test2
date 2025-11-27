
#ifndef __KEY_H
#define __KEY_H

#include "./SYSTEM/sys/sys.h"

#define KEY1_GPIO_PORT                  GPIOC
#define KEY1_GPIO_PIN                   GPIO_PIN_12
#define KEY1_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)   

#define KEY2_GPIO_PORT                  GPIOC
#define KEY2_GPIO_PIN                   GPIO_PIN_11
#define KEY2_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)   

#define KEY3_GPIO_PORT                  GPIOC
#define KEY3_GPIO_PIN                   GPIO_PIN_10
#define KEY3_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)   

#define KEY4_GPIO_PORT                  GPIOA
#define KEY4_GPIO_PIN                   GPIO_PIN_15
#define KEY4_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)   

#define KEY1         HAL_GPIO_ReadPin(KEY1_GPIO_PORT, KEY1_GPIO_PIN)     
#define KEY2         HAL_GPIO_ReadPin(KEY2_GPIO_PORT, KEY2_GPIO_PIN)     
#define KEY3         HAL_GPIO_ReadPin(KEY3_GPIO_PORT, KEY3_GPIO_PIN)     
#define KEY4         HAL_GPIO_ReadPin(KEY4_GPIO_PORT, KEY4_GPIO_PIN)     

#define KEY1_PRESSED     (KEY1 == GPIO_PIN_RESET)  
#define KEY2_PRESSED     (KEY2 == GPIO_PIN_RESET)  
#define KEY3_PRESSED     (KEY3 == GPIO_PIN_RESET)  
#define KEY4_PRESSED     (KEY4 == GPIO_PIN_RESET)  

#define KEY_EVENT_NONE          0x00    
#define KEY_EVENT_SHORT_PRESS   0x01    
#define KEY_EVENT_LONG_PRESS    0x02    

#define KEY_VAL_NONE    0x00
#define KEY_VAL_KEY1    0x10
#define KEY_VAL_KEY2    0x20
#define KEY_VAL_KEY3    0x30
#define KEY_VAL_KEY4    0x40

#define KEY1_SHORT_PRESS    (KEY_VAL_KEY1 | KEY_EVENT_SHORT_PRESS)  
#define KEY1_LONG_PRESS     (KEY_VAL_KEY1 | KEY_EVENT_LONG_PRESS)   
#define KEY2_SHORT_PRESS    (KEY_VAL_KEY2 | KEY_EVENT_SHORT_PRESS)  
#define KEY2_LONG_PRESS     (KEY_VAL_KEY2 | KEY_EVENT_LONG_PRESS)   
#define KEY3_SHORT_PRESS    (KEY_VAL_KEY3 | KEY_EVENT_SHORT_PRESS)  
#define KEY3_LONG_PRESS     (KEY_VAL_KEY3 | KEY_EVENT_LONG_PRESS)   
#define KEY4_SHORT_PRESS    (KEY_VAL_KEY4 | KEY_EVENT_SHORT_PRESS)  
#define KEY4_LONG_PRESS     (KEY_VAL_KEY4 | KEY_EVENT_LONG_PRESS)   

#define KEY1_PRES    KEY1_SHORT_PRESS
#define KEY2_PRES    KEY2_SHORT_PRESS
#define KEY3_PRES    KEY3_SHORT_PRESS
#define KEY4_PRES    KEY4_SHORT_PRESS

#define KEY_DEBOUNCE_TIME_MS       50      
#define KEY_SHORT_PRESS_TIME_MS    500     
#define KEY_LONG_PRESS_TIME_MS     2000    
#define KEY_SCAN_INTERVAL_MS       10      

void key_init(void);                        
uint8_t key_scan(void);                     
uint8_t key_get_pressed_key(void);          

#define KEY_GET_KEY_VAL(key_event)  ((key_event) & 0xF0)

#define KEY_GET_EVENT_TYPE(key_event)  ((key_event) & 0x0F)

#endif
