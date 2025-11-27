#include "fan.h"

static TIM_HandleTypeDef s_fan_tim = {0};
static uint32_t s_fan_period = FAN_PWM_RESOLUTION_STEPS - 1U;
static uint8_t s_fan_initialized = 0U;
static uint8_t s_fan_enabled = 0U;
static uint8_t s_fan_pwm_started = 0U;
static uint8_t s_fan_pwm_ready = 0U;
static uint8_t s_fan_speed_percent = 0U;
static fan_level_t s_fan_level = FAN_LEVEL_LOW;
static uint8_t s_delay_pending = 0U;
static uint32_t s_delay_deadline = 0U;

static uint32_t fan_get_timer_clock(void)
{
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    uint32_t d2ppre1 = (RCC->D2CFGR & RCC_D2CFGR_D2PPRE1_Msk) >> RCC_D2CFGR_D2PPRE1_Pos;

    if (d2ppre1 >= 4U)    
    {
        return pclk1 * 2U;
    }

    return pclk1;
}

static void fan_pwm_config(void)
{
    uint32_t timer_clk = fan_get_timer_clock();
    uint32_t target_counts = (timer_clk / FAN_PWM_DEFAULT_FREQ_HZ);
    uint32_t prescaler = 1U;

    if (target_counts > FAN_PWM_RESOLUTION_STEPS)
    {
        prescaler = target_counts / FAN_PWM_RESOLUTION_STEPS;
        if ((target_counts % FAN_PWM_RESOLUTION_STEPS) != 0U)
        {
            prescaler++;
        }
    }

    if (prescaler == 0U)
    {
        prescaler = 1U;
    }

    uint32_t effective_counts = target_counts / prescaler;
    if (effective_counts == 0U)
    {
        effective_counts = FAN_PWM_RESOLUTION_STEPS;
    }

    s_fan_period = (effective_counts > 0U) ? (effective_counts - 1U) : (FAN_PWM_RESOLUTION_STEPS - 1U);

    s_fan_tim.Instance = FAN_PWM_TIM;
    s_fan_tim.Init.Prescaler = (uint32_t)(prescaler - 1U);
    s_fan_tim.Init.CounterMode = TIM_COUNTERMODE_UP;
    s_fan_tim.Init.Period = s_fan_period;
    s_fan_tim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    s_fan_tim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
}

void fan_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    TIM_OC_InitTypeDef tim_oc = {0};

    if (s_fan_initialized)
    {
        return;
    }

    FAN_EN_GPIO_CLK_ENABLE();
    FAN_PWM_GPIO_CLK_ENABLE();
    FAN_PWM_TIM_CLK_ENABLE();

    
    gpio.Pin = FAN_EN_GPIO_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(FAN_EN_GPIO_PORT, &gpio);
    HAL_GPIO_WritePin(FAN_EN_GPIO_PORT, FAN_EN_GPIO_PIN, FAN_EN_INACTIVE_LEVEL);

    
    gpio.Pin = FAN_PWM_PIN;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = FAN_PWM_GPIO_AF;
    HAL_GPIO_Init(FAN_PWM_PORT, &gpio);

    fan_pwm_config();

    if (HAL_TIM_PWM_Init(&s_fan_tim) == HAL_OK)
    {
        tim_oc.OCMode = TIM_OCMODE_PWM1;
        tim_oc.Pulse = 0U;
        tim_oc.OCPolarity = TIM_OCPOLARITY_HIGH;
        tim_oc.OCFastMode = TIM_OCFAST_DISABLE;

        if (HAL_TIM_PWM_ConfigChannel(&s_fan_tim, &tim_oc, FAN_PWM_TIM_CHANNEL) == HAL_OK)
        {
            s_fan_pwm_ready = 1U;
        }
    }

    s_fan_initialized = 1U;
    s_fan_speed_percent = FAN_DEFAULT_SPEED_PERCENT;
    s_fan_level = FAN_LEVEL_HIGH;
    s_fan_enabled = 0U;
    s_fan_pwm_started = 0U;
    s_delay_pending = 0U;
}

void fan_on(void)
{
    if (!s_fan_initialized)
    {
        fan_init();
    }

    if (!s_fan_initialized)
    {
        return;
    }

    if (s_fan_pwm_ready && !s_fan_pwm_started)
    {
        if (HAL_TIM_PWM_Start(&s_fan_tim, FAN_PWM_TIM_CHANNEL) == HAL_OK)
        {
            s_fan_pwm_started = 1U;
        }
    }

    HAL_GPIO_WritePin(FAN_EN_GPIO_PORT, FAN_EN_GPIO_PIN, FAN_EN_ACTIVE_LEVEL);
    s_fan_enabled = 1U;
    s_delay_pending = 0U;

    /* 风扇开启时使用默认速度（10% PWM占空比） */
    if (s_fan_pwm_ready)
    {
        fan_set_speed(FAN_DEFAULT_SPEED_PERCENT);
    }
}

void fan_off(void)
{
    if (!s_fan_initialized)
    {
        fan_init();
    }

    if (!s_fan_initialized)
    {
        return;
    }

    fan_set_speed(0U);
    HAL_GPIO_WritePin(FAN_EN_GPIO_PORT, FAN_EN_GPIO_PIN, FAN_EN_INACTIVE_LEVEL);
    s_fan_enabled = 0U;
    s_delay_pending = 0U;

    if (s_fan_pwm_ready && s_fan_pwm_started)
    {
        (void)HAL_TIM_PWM_Stop(&s_fan_tim, FAN_PWM_TIM_CHANNEL);
        s_fan_pwm_started = 0U;
    }
}

void fan_set_speed(uint8_t speed)
{
    uint8_t clamped = (speed > 100U) ? 100U : speed;
    uint32_t compare = 0U;

    if (!s_fan_initialized)
    {
        fan_init();
    }

    if (!s_fan_initialized)
    {
        return;
    }

    if (s_fan_pwm_ready)
    {
        /* 硬件为低电平有效，PWM占空比需要反转 */
        uint32_t inverted = (uint32_t)(100U - clamped) * (s_fan_period + 1U);
        compare = inverted / 100U;

        if (compare > s_fan_period)
        {
            compare = s_fan_period;
        }
    }

    if (s_fan_pwm_ready)
    {
        __HAL_TIM_SET_COMPARE(&s_fan_tim, FAN_PWM_TIM_CHANNEL, compare);
    }

    s_fan_speed_percent = clamped;
}

void fan_set_level(fan_level_t level)
{
    uint8_t target = (level == FAN_LEVEL_HIGH) ? FAN_LEVEL_HIGH_SPEED_PERCENT
                                               : FAN_LEVEL_LOW_SPEED_PERCENT;

    if (!s_fan_initialized)
    {
        fan_init();
    }

    if (!s_fan_initialized)
    {
        return;
    }

    fan_set_speed(target);
    s_fan_level = level;
}

uint8_t fan_get_state(void)
{
    return s_fan_enabled;
}

uint8_t fan_get_speed(void)
{
    return s_fan_speed_percent;
}

fan_level_t fan_get_level(void)
{
    return s_fan_level;
}

void fan_schedule_delay_off(uint32_t delay_ms)
{
    if (!s_fan_initialized)
    {
        fan_init();
    }

    if (!s_fan_initialized)
    {
        return;
    }

    s_delay_pending = 1U;
    s_delay_deadline = HAL_GetTick() + delay_ms;
}

void fan_process(void)
{
    if (s_delay_pending)
    {
        int32_t diff = (int32_t)(HAL_GetTick() - s_delay_deadline);
        if (diff >= 0)
        {
            s_delay_pending = 0U;
            fan_off();
        }
    }
}

