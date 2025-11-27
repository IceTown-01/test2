#ifndef __TIMER_H
#define __TIMER_H

#include "./SYSTEM/sys/sys.h"

#define DEFAULT_WORK_TIME_MS         (15 * 60 * 1000)

#define MODE1_PAUSE_INTERVAL_MS      (10 * 1000)

#define MOTION_STATIC_TIMEOUT_MS     (5 * 60 * 1000)

#define FAN_DELAY_SHUTDOWN_MS        (20 * 1000)

void timer_init(void);
void timer_start_countdown(uint32_t time_ms);  
void timer_stop_countdown(void);                
void timer_pause_countdown(void);               
void timer_resume_countdown(void);              
uint32_t timer_get_remaining_time(void);        
uint8_t timer_is_timeout(void);                 
void timer_reset(void);                         

#endif


