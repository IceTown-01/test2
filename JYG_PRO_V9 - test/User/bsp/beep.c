#include "beep.h"
#include "./SYSTEM/delay/delay.h"

static uint8_t s_beep_initialized = 0U;

void beep_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    if (s_beep_initialized)
    {
        return;
    }
    
    BEEP_GPIO_CLK_ENABLE();
    
    gpio_init_struct.Pin = BEEP_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(BEEP_GPIO_PORT, &gpio_init_struct);
    
    HAL_GPIO_WritePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN, GPIO_PIN_RESET);
    
    s_beep_initialized = 1U;
}

void beep_beep(void)
{
    uint32_t cycles;
    uint32_t i;
    
    if (!s_beep_initialized)
    {
        beep_init();
    }
    
    if (!s_beep_initialized)
    {
        return;
    }
    
    cycles = (BEEP_DURATION_MS * 1000U) / BEEP_PWM_PERIOD_US;
    
    for (i = 0; i < cycles; i++)
    {
        HAL_GPIO_WritePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN, GPIO_PIN_SET);
        delay_us(BEEP_PWM_HIGH_US);
        
        HAL_GPIO_WritePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN, GPIO_PIN_RESET);
        delay_us(BEEP_PWM_LOW_US);
    }
    
    HAL_GPIO_WritePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN, GPIO_PIN_RESET);
}
