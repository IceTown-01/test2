#ifndef __FAN_H
#define __FAN_H

#include "./SYSTEM/sys/sys.h"

#define FAN_EN_GPIO_PORT                 GPIOA
#define FAN_EN_GPIO_PIN                  GPIO_PIN_0
#define FAN_EN_GPIO_CLK_ENABLE()         do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)
#define FAN_EN_ACTIVE_LEVEL              GPIO_PIN_SET
#define FAN_EN_INACTIVE_LEVEL            GPIO_PIN_RESET

#define FAN_PWM_PIN                      GPIO_PIN_1
#define FAN_PWM_PORT                     GPIOA
#define FAN_PWM_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)
#define FAN_PWM_GPIO_AF                  GPIO_AF1_TIM2

#define FAN_PWM_TIM                      TIM2
#define FAN_PWM_TIM_CHANNEL              TIM_CHANNEL_2
#define FAN_PWM_TIM_CLK_ENABLE()         do{ __HAL_RCC_TIM2_CLK_ENABLE(); }while(0)

#define FAN_PWM_DEFAULT_FREQ_HZ          25000U
#define FAN_PWM_RESOLUTION_STEPS         1000U

/* 风扇默认速度：10% PWM占空比（硬件低电平有效，实际为最大风量） */
#define FAN_DEFAULT_SPEED_PERCENT        60U

/* 保留旧定义以兼容旧代码，但不再使用 */
#define FAN_LEVEL_LOW_SPEED_PERCENT      60U
#define FAN_LEVEL_HIGH_SPEED_PERCENT     10U

typedef enum
{
    FAN_LEVEL_LOW = 0U,
    FAN_LEVEL_HIGH = 1U
} fan_level_t;

void fan_init(void);
void fan_on(void);
void fan_off(void);
void fan_set_speed(uint8_t speed);  
void fan_set_level(fan_level_t level);
uint8_t fan_get_state(void);
uint8_t fan_get_speed(void);
fan_level_t fan_get_level(void);
void fan_schedule_delay_off(uint32_t delay_ms);
void fan_process(void);

#endif
