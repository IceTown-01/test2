#include "timer.h"
#include "./SYSTEM/delay/delay.h"

typedef struct
{
    uint32_t remaining_ms;
    uint32_t last_tick;
    uint8_t running;
    uint8_t paused;
    uint8_t timeout;
} countdown_timer_t;

static countdown_timer_t s_timer = {0};

static uint32_t timer_get_tick(void)
{
    return HAL_GetTick();
}

static void timer_update_internal(void)
{
    if (!s_timer.running || s_timer.paused)
    {
        return;
    }

    uint32_t now = timer_get_tick();
    uint32_t elapsed = now - s_timer.last_tick;

    if (elapsed > 0U)
    {
        s_timer.last_tick = now;

        if (elapsed >= s_timer.remaining_ms)
        {
            s_timer.remaining_ms = 0U;
            s_timer.running = 0U;
            s_timer.timeout = 1U;
        }
        else
        {
            s_timer.remaining_ms -= elapsed;
        }
    }
}

void timer_init(void)
{
    s_timer.remaining_ms = 0U;
    s_timer.last_tick = timer_get_tick();
    s_timer.running = 0U;
    s_timer.paused = 0U;
    s_timer.timeout = 0U;
}

void timer_start_countdown(uint32_t time_ms)
{
    if (time_ms == 0U)
    {
        timer_stop_countdown();
        s_timer.timeout = 1U;
        return;
    }

    s_timer.remaining_ms = time_ms;
    s_timer.last_tick = timer_get_tick();
    s_timer.running = 1U;
    s_timer.paused = 0U;
    s_timer.timeout = 0U;
}

void timer_stop_countdown(void)
{
    s_timer.running = 0U;
    s_timer.paused = 0U;
    s_timer.remaining_ms = 0U;
    s_timer.timeout = 0U;
}

void timer_pause_countdown(void)
{
    if (s_timer.running && !s_timer.paused)
    {
        timer_update_internal();
        s_timer.paused = 1U;
    }
}

void timer_resume_countdown(void)
{
    if (s_timer.running && s_timer.paused)
    {
        s_timer.last_tick = timer_get_tick();
        s_timer.paused = 0U;
    }
}

uint32_t timer_get_remaining_time(void)
{
    timer_update_internal();
    return s_timer.remaining_ms;
}

uint8_t timer_is_timeout(void)
{
    timer_update_internal();
    return s_timer.timeout;
}

void timer_reset(void)
{
    timer_stop_countdown();
}
