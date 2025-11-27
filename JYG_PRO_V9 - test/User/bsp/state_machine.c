#include "state_machine.h"
#include "mode_manager.h"
#include "timer.h"

static system_state_t current_state = SYSTEM_IDLE;
static work_mode_t current_mode = MODE_NONE;

void state_machine_init(void)
{
    current_state = SYSTEM_IDLE;
    current_mode = MODE_NONE;
}

static void state_idle_handler(state_event_t event)
{
    switch (event)
    {
        case EVT_KEY1_LONG_PRESS:
            
            
            current_state = SYSTEM_MODE_SELECT;
            current_mode = MODE_1_F_SPOTS;  
            break;
        default:
            break;
    }
}

static void state_mode_select_handler(state_event_t event)
{
    switch (event)
    {
        case EVT_KEY2_SHORT_PRESS:
            
            mode_manager_next_mode();
            current_mode = mode_manager_get_mode();
            break;
            
        case EVT_KEY1_SHORT_PRESS:
            
            
            
            current_state = SYSTEM_WORKING;
            break;
            
        case EVT_KEY3_SHORT_PRESS:
            
            
            break;
            
        case EVT_KEY4_SHORT_PRESS:
            
            
            break;
        default:
            break;
    }
}

static void state_working_handler(state_event_t event)
{
    switch (event)
    {
        case EVT_KEY1_SHORT_PRESS:
            if (current_mode == MODE_1_F_SPOTS)
            {
                
                
                current_state = SYSTEM_WORKING;
            }
            break;
            
        case EVT_MODE1_PAUSE:
            if (current_mode == MODE_1_F_SPOTS)
            {
                
                
                current_state = SYSTEM_PAUSED;
            }
            break;
            
        case EVT_TIMER_TIMEOUT:
            
            
            current_state = SYSTEM_FINISHED;
            break;
            
        case EVT_MOTION_STATIC:
            if (current_mode == MODE_3_COLLAGEN || current_mode == MODE_4_SOOTING)
            {
                
                
                current_state = SYSTEM_PAUSED;
            }
            break;
            
        case EVT_KEY3_SHORT_PRESS:
            
            
            break;
            
        case EVT_KEY4_SHORT_PRESS:
            
            
            break;
        default:
            break;
    }
}

static void state_paused_handler(state_event_t event)
{
    switch (event)
    {
        case EVT_KEY1_SHORT_PRESS:
            
            
            
            current_state = SYSTEM_WORKING;
            break;
        default:
            break;
    }
}

static void state_finished_handler(state_event_t event)
{
    switch (event)
    {
        case EVT_TIMER_TIMEOUT:
            
            
            
            current_state = SYSTEM_SHUTDOWN;
            break;
        default:
            break;
    }
}

void state_machine_process_event(state_event_t event)
{
    if (event == EVT_NONE) return;
    
    switch (current_state)
    {
        case SYSTEM_IDLE:
            state_idle_handler(event);
            break;
            
        case SYSTEM_MODE_SELECT:
            state_mode_select_handler(event);
            break;
            
        case SYSTEM_WORKING:
            state_working_handler(event);
            break;
            
        case SYSTEM_PAUSED:
            state_paused_handler(event);
            break;
            
        case SYSTEM_FINISHED:
            state_finished_handler(event);
            break;
            
        case SYSTEM_SHUTDOWN:
            
            break;
            
        default:
            break;
    }
}

void state_machine_update(void)
{
    
    if (timer_is_timeout())
    {
        if (current_state == SYSTEM_WORKING)
        {
            state_machine_process_event(EVT_TIMER_TIMEOUT);
        }
        else if (current_state == SYSTEM_FINISHED)
        {
            
            state_machine_process_event(EVT_TIMER_TIMEOUT);
        }
    }
    
    
    if (current_state == SYSTEM_WORKING && current_mode == MODE_1_F_SPOTS)
    {
        
    }
    
    
    if (current_state == SYSTEM_WORKING && 
        (current_mode == MODE_3_COLLAGEN || current_mode == MODE_4_SOOTING))
    {
        
    }
}

system_state_t state_machine_get_state(void)
{
    return current_state;
}


