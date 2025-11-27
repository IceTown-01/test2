#ifndef __MODE_MANAGER_H
#define __MODE_MANAGER_H

#include "./SYSTEM/sys/sys.h"

typedef enum
{
    MODE_NONE = 0,          
    MODE_1_F_SPOTS,         
    MODE_2_BRIGHTENING,     
    MODE_3_COLLAGEN,        
    MODE_4_SOOTING,         
    MODE_5_RESTORING        
} work_mode_t;

typedef enum
{
    SYSTEM_IDLE = 0,        
    SYSTEM_MODE_SELECT,     
    SYSTEM_WORKING,         
    SYSTEM_PAUSED,          
    SYSTEM_FINISHED,        
    SYSTEM_SHUTDOWN         
} system_state_t;

void mode_manager_init(void);
void mode_manager_set_mode(work_mode_t mode);  
work_mode_t mode_manager_get_mode(void);       
void mode_manager_next_mode(void);             
void mode_manager_start_work(void);            
void mode_manager_pause_work(void);            
void mode_manager_resume_work(void);           
void mode_manager_stop_work(void);             
system_state_t mode_manager_get_state(void);   

#endif


