#ifndef __DIGITAL_POTENTIOMETER_H
#define __DIGITAL_POTENTIOMETER_H

#include "./SYSTEM/sys/sys.h"

void digital_potentiometer_init(void);
void digital_potentiometer_set_level(uint8_t level);  
uint8_t digital_potentiometer_get_level(void);        
uint8_t digital_potentiometer_get_max_level(void);    

#endif
