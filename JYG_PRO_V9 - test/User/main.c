#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "key.h"
#include "version.h"
#include "beep.h"
#include "display.h"
#include "system_init.h"
#include "laser.h"
#include "tec.h"
#include "fan.h"
#include "wsd.h"
#include "timer.h"
#include "motion_sensor.h"

static uint8_t current_mode = MODE_1;

static uint8_t current_level = LEVEL_MIN;

typedef enum
{
    SYS_STATE_IDLE,         
    SYS_STATE_MODE_SELECT,   
    SYS_STATE_WORKING        
} sys_state_t;

static sys_state_t sys_state = SYS_STATE_IDLE;
static uint8_t timer_started = 0U;
static uint32_t last_displayed_seconds = 0xFFFFFFFFUL;
static uint8_t motion_monitor_active = 0U;
static uint8_t motion_paused = 0U;
static uint32_t mode_145_work_start_tick = 0U;

#define TEC_WORK_POWER_PERCENT         0U
#define TEC_WORK_POWER_PERCENT_2			 7U			//6的值是22V，7的值是19.0V

#define MODE1_WORK_TIME_MS               (30U * 1000U)
#define MOTION_STATIC_PAUSE_MS           MOTION_SENSOR_STATIC_PAUSE_MS
#define MOTION_STATIC_SHUTDOWN_MS        MOTION_SENSOR_STATIC_SHUTDOWN_MS

static void handle_key_event(uint8_t key_event);
static void update_display(void);
static void apply_mode_defaults(uint8_t mode);
static void start_mode_outputs(uint8_t mode);
static void stop_load_outputs(void);
static void sync_outputs_with_level(void);
static void handle_system_power_on(void);
static void handle_system_power_off(void);
static uint8_t mode_allows_level_adjust(uint8_t mode);
static void handle_countdown_timeout(void);
static void update_time_display_if_needed(void);
static uint32_t get_mode_total_time_ms(uint8_t mode);
static void refresh_time_display(void);
static void enable_motion_monitor_if_needed(uint8_t mode);
static void disable_motion_monitor(void);
static void process_motion_sensor(void);

static void apply_mode_defaults(uint8_t mode)
{
    switch (mode)
    {
        case MODE_2:
        case MODE_3:
        case MODE_4:
            current_level = LEVEL_MAX;
            break;

        case MODE_5:
            current_level = LEVEL_MIN;
            break;

        case MODE_1:
        default:
            current_level = LEVEL_MIN;
            break;
    }
}

static void start_mode_outputs(uint8_t mode)
{
    fan_on();

    switch (mode)
    {
        case MODE_2:
            wsd_off();
            tec_off();
            laser_on();
            break;

        case MODE_1:
        case MODE_3:
        case MODE_4:
        case MODE_5:
            laser_off();
            if (mode == MODE_4)
            {
                wsd_off();
							tec_on();
							tec_set_power(TEC_WORK_POWER_PERCENT_2);

            }
            else
            {
                wsd_on();
                wsd_set_level(current_level);
								tec_on();
								tec_set_power(TEC_WORK_POWER_PERCENT);
            }

            
            break;

        default:
            stop_load_outputs();
            break;
    }
}

static void stop_load_outputs(void)
{
    laser_off();
    tec_off();
    wsd_off();
}

static void sync_outputs_with_level(void)
{
    if (sys_state != SYS_STATE_WORKING)
    {
        return;
    }

    fan_on();

    if (current_mode == MODE_1 ||
        current_mode == MODE_3 ||
        current_mode == MODE_4 ||
        current_mode == MODE_5)
    {
        tec_set_power(TEC_WORK_POWER_PERCENT);
    }

    if (current_mode == MODE_1 ||
        current_mode == MODE_3 ||
        current_mode == MODE_5)
    {
        wsd_on();
        wsd_set_level(current_level);
    }
    else
    {
        wsd_off();
    }
}

static void handle_system_power_on(void)
{
    fan_on();
    stop_load_outputs();
    apply_mode_defaults(current_mode);
    if (!motion_monitor_active)
    {
        motion_sensor_enable();
        motion_monitor_active = 1U;
    }
    motion_sensor_reset_static_timer();
    timer_started = 0U;
    last_displayed_seconds = 0xFFFFFFFFUL;
    update_display();
}

static void handle_system_power_off(void)
{
    stop_load_outputs();
    disable_motion_monitor();
    /* 关机时风扇延时10秒关闭 */
    fan_schedule_delay_off(FAN_DELAY_SHUTDOWN_MS);
    timer_stop_countdown();
    timer_started = 0U;
    last_displayed_seconds = 0xFFFFFFFFUL;
}

static uint8_t mode_allows_level_adjust(uint8_t mode)
{
    return (mode == MODE_1) ? 1U : 0U;
}

static void handle_countdown_timeout(void)
{
    if (!timer_started)
    {
        return;
    }

    if (timer_is_timeout())
    {
        stop_load_outputs();
        disable_motion_monitor();

        if (current_mode == MODE_2)
        {
            fan_on();
            sys_state = SYS_STATE_MODE_SELECT;
            timer_reset();
            timer_started = 0U;
            last_displayed_seconds = 0xFFFFFFFFUL;
            beep_beep();
            refresh_time_display();
        }
        else
        {
            /* 倒计时结束，风扇延时10秒关闭 */
            fan_schedule_delay_off(FAN_DELAY_SHUTDOWN_MS);

            sys_state = SYS_STATE_IDLE;
            timer_reset();
            timer_started = 0U;
            last_displayed_seconds = 0xFFFFFFFFUL;
            beep_beep();
            display_clear();
        }
    }
}

static void update_time_display_if_needed(void)
{
    if (!timer_started)
    {
        return;
    }

    uint32_t remaining_ms = timer_get_remaining_time();
    uint32_t remaining_sec = remaining_ms / 1000U;

    if (remaining_sec != last_displayed_seconds)
    {
        last_displayed_seconds = remaining_sec;
        DEBUG_PRINT("[TIMER] remaining: %lus\r\n", (unsigned long)remaining_sec);
        display_show_time_text(remaining_ms, get_mode_total_time_ms(current_mode));
    }
}

static void handle_key_event(uint8_t key_event)
{
    switch (key_event)
    {
        case KEY1_SHORT_PRESS:
            
            if (sys_state == SYS_STATE_MODE_SELECT)
            {
                sys_state = SYS_STATE_WORKING;
                if (!timer_started)
                {
                    timer_start_countdown(get_mode_total_time_ms(current_mode));
                    timer_started = 1U;
                }
                else
                {
                    timer_resume_countdown();
                }
                start_mode_outputs(current_mode);
                enable_motion_monitor_if_needed(current_mode);
                beep_beep();
                update_display();
            }
            else if (sys_state == SYS_STATE_WORKING)
            {
                
                sys_state = SYS_STATE_MODE_SELECT;
                stop_load_outputs();
                fan_on();
                timer_pause_countdown();
                if (!motion_monitor_active)
                {
                    motion_sensor_enable();
                    motion_monitor_active = 1U;
                }
                motion_sensor_reset_static_timer();
                beep_beep();
                update_display();
            }
            break;
            
        case KEY1_LONG_PRESS:
            
            if (sys_state == SYS_STATE_IDLE)
            {
                
                sys_state = SYS_STATE_MODE_SELECT;
                current_mode = MODE_1;  
                handle_system_power_on();
                timer_reset();
                beep_beep();
            }
            else
            {
                
                sys_state = SYS_STATE_IDLE;
                handle_system_power_off();
                beep_beep();
                display_clear();
            }
            break;
        
        case KEY2_SHORT_PRESS:
            
            if (sys_state == SYS_STATE_MODE_SELECT)
            {
                current_mode++;
                if (current_mode > MODE_MAX)
                {
                    current_mode = MODE_MIN;  
                }
                apply_mode_defaults(current_mode);
                if (timer_started)
                {
                    timer_reset();
                    timer_started = 0U;
                    last_displayed_seconds = 0xFFFFFFFFUL;
                }
                if (!motion_monitor_active)
                {
                    motion_sensor_enable();
                    motion_monitor_active = 1U;
                }
                motion_sensor_reset_static_timer();
                beep_beep();
                update_display();
            }
            break;
        
        case KEY3_SHORT_PRESS:
            
            if (mode_allows_level_adjust(current_mode) &&
                (sys_state == SYS_STATE_WORKING || sys_state == SYS_STATE_MODE_SELECT))
            {
                if (current_level < LEVEL_MAX)
                {
                    current_level++;
                    beep_beep();
                    update_display();
                    sync_outputs_with_level();
                }
            }
            break;
        
        case KEY4_SHORT_PRESS:
            
            if (mode_allows_level_adjust(current_mode) &&
                (sys_state == SYS_STATE_WORKING || sys_state == SYS_STATE_MODE_SELECT))
            {
                if (current_level > LEVEL_MIN)
                {
                    current_level--;
                    beep_beep();
                    update_display();
                    sync_outputs_with_level();
                }
            }
            break;
        
        default:
            break;
    }
}

static void update_display(void)
{
    display_refresh(current_mode, current_level);
    refresh_time_display();
}

int main(void)
{
    uint8_t key_event;
    
    
    sys_cache_enable();                 
    HAL_Init();                         
    sys_stm32_clock_init(192, 5, 2, 4); 
    delay_init(480);                    
    timer_init();
    
    system_init();                      
    
    
    display_init();
    
    
    beep_beep();
    delay_ms(500);
    
    
    while (1)
    {
        fan_process();
        update_time_display_if_needed();
        handle_countdown_timeout();
        process_motion_sensor();
        key_event = key_scan();         
        
        if (key_event != KEY_EVENT_NONE)
        {
            
            handle_key_event(key_event);
        }
        else
        {
            
            delay_ms(KEY_SCAN_INTERVAL_MS);
        }
    }
}

static uint32_t get_mode_total_time_ms(uint8_t mode)
{
    return (mode == MODE_2) ? MODE1_WORK_TIME_MS : DEFAULT_WORK_TIME_MS;
}

static void refresh_time_display(void)
{
    uint32_t total_ms = get_mode_total_time_ms(current_mode);
    uint32_t remaining_ms;

    if (timer_started)
    {
        remaining_ms = timer_get_remaining_time();
        last_displayed_seconds = remaining_ms / 1000U;
    }
    else
    {
        remaining_ms = total_ms;
    }

    display_show_time_text(remaining_ms, total_ms);
}

static void enable_motion_monitor_if_needed(uint8_t mode)
{
    if (mode == MODE_1 || mode == MODE_3)
    {
        if (!motion_monitor_active)
        {
            motion_sensor_enable();
            motion_monitor_active = 1U;
        }
        motion_sensor_reset_static_timer();
        motion_paused = 0U;
    }
    else if (mode == MODE_2 || mode == MODE_4 || mode == MODE_5)
    {
        if (!motion_monitor_active)
        {
            motion_sensor_enable();
            motion_monitor_active = 1U;
        }
        mode_145_work_start_tick = HAL_GetTick();
        motion_paused = 0U;
    }
    else
    {
        disable_motion_monitor();
    }
}

static void disable_motion_monitor(void)
{
    if (motion_monitor_active)
    {
        motion_sensor_disable();
        motion_monitor_active = 0U;
    }
    motion_paused = 0U;
}

static void process_motion_sensor(void)
{
    if (!motion_monitor_active)
    {
        return;
    }

    uint8_t moving = motion_sensor_is_moving();

    if (sys_state == SYS_STATE_IDLE || sys_state == SYS_STATE_MODE_SELECT)
    {
        if (moving)
        {
            motion_sensor_reset_static_timer();
        }
        else
        {
            uint32_t static_time = motion_sensor_get_static_time();
            if (static_time >= MOTION_STATIC_SHUTDOWN_MS)
            {
                handle_system_power_off();
                disable_motion_monitor();
                beep_beep();
                display_clear();
            }
        }
        return;
    }

    if (sys_state != SYS_STATE_WORKING)
    {
        return;
    }

    if (current_mode == MODE_2 || current_mode == MODE_4 || current_mode == MODE_5)
    {
        uint32_t work_time = HAL_GetTick() - mode_145_work_start_tick;
        if (!moving && work_time >= MOTION_STATIC_SHUTDOWN_MS)
        {
            stop_load_outputs();
            fan_schedule_delay_off(FAN_DELAY_SHUTDOWN_MS);
            sys_state = SYS_STATE_IDLE;
            disable_motion_monitor();
            timer_reset();
            timer_started = 0U;
            last_displayed_seconds = 0xFFFFFFFFUL;
            beep_beep();
            display_clear();
        }
        return;
    }

    if (current_mode == MODE_1 || current_mode == MODE_3)
    {
        if (moving)
        {
            if (motion_paused)
            {
                start_mode_outputs(current_mode);
                timer_resume_countdown();
                motion_sensor_reset_static_timer();
                motion_paused = 0U;
            }
            return;
        }

        uint32_t static_time = motion_sensor_get_static_time();

        if (!motion_paused && static_time >= MOTION_STATIC_PAUSE_MS)
        {
            stop_load_outputs();
            fan_on();
            timer_pause_countdown();
            motion_paused = 1U;
        }

        if (static_time >= MOTION_STATIC_SHUTDOWN_MS)
        {
            stop_load_outputs();
            fan_schedule_delay_off(FAN_DELAY_SHUTDOWN_MS);
            sys_state = SYS_STATE_IDLE;
            disable_motion_monitor();
            timer_reset();
            timer_started = 0U;
            last_displayed_seconds = 0xFFFFFFFFUL;
            beep_beep();
            display_clear();
        }
        return;
    }
}
