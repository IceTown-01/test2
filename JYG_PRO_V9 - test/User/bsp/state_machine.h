#ifndef __STATE_MACHINE_H
#define __STATE_MACHINE_H

#include "./SYSTEM/sys/sys.h"
#include "mode_manager.h"

typedef enum
{
    EVT_NONE = 0,           
    EVT_KEY1_LONG_PRESS,    
    EVT_KEY1_SHORT_PRESS,   
    EVT_KEY2_SHORT_PRESS,   
    EVT_KEY3_SHORT_PRESS,   
    EVT_KEY4_SHORT_PRESS,   
    EVT_TIMER_TIMEOUT,      
    EVT_MODE1_PAUSE,        
    EVT_MOTION_STATIC,      
    EVT_WORK_FINISH         
} state_event_t;

void state_machine_init(void);
void state_machine_process_event(state_event_t event);  
void state_machine_update(void);                        
system_state_t state_machine_get_state(void);          

#endif


