#include "laser.h"

static uint8_t s_laser_on = 0U;

void laser_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    LASER_EN_GPIO_CLK_ENABLE();

    gpio.Pin = LASER_EN_GPIO_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LASER_EN_GPIO_PORT, &gpio);

    HAL_GPIO_WritePin(LASER_EN_GPIO_PORT, LASER_EN_GPIO_PIN, LASER_INACTIVE_LEVEL);
    s_laser_on = 0U;
}

void laser_on(void)
{
    HAL_GPIO_WritePin(LASER_EN_GPIO_PORT, LASER_EN_GPIO_PIN, LASER_ACTIVE_LEVEL);
    s_laser_on = 1U;
}

void laser_off(void)
{
    HAL_GPIO_WritePin(LASER_EN_GPIO_PORT, LASER_EN_GPIO_PIN, LASER_INACTIVE_LEVEL);
    s_laser_on = 0U;
}

uint8_t laser_get_state(void)
{
    return s_laser_on;
}
