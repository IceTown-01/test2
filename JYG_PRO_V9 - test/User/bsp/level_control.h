#ifndef __LEVEL_CONTROL_H
#define __LEVEL_CONTROL_H

#include "./SYSTEM/sys/sys.h"

void level_control_init(void);
void level_control_increase(void);      
void level_control_decrease(void);      
void level_control_set_level(uint8_t level);  
uint8_t level_control_get_level(void);        
uint8_t level_control_get_min_level(void);    
uint8_t level_control_get_max_level(void);    

#endif


