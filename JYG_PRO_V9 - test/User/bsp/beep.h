
#ifndef __BEEP_H
#define __BEEP_H

#include "./SYSTEM/sys/sys.h"

#define BEEP_GPIO_PORT                  GPIOC
#define BEEP_GPIO_PIN                   GPIO_PIN_0
#define BEEP_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)

#define BEEP_PWM_FREQ_HZ                2000U
#define BEEP_PWM_PERIOD_US              (1000000U / BEEP_PWM_FREQ_HZ)
#define BEEP_PWM_HIGH_US                (BEEP_PWM_PERIOD_US / 2U)
#define BEEP_PWM_LOW_US                 (BEEP_PWM_PERIOD_US / 2U)

#define BEEP_DURATION_MS                100U

void beep_init(void);      
void beep_beep(void);       

#endif
