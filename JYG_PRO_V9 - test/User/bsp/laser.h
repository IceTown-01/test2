#ifndef __LASER_H
#define __LASER_H

#include "./SYSTEM/sys/sys.h"

#define LASER_EN_GPIO_PORT                 GPIOC
#define LASER_EN_GPIO_PIN                  GPIO_PIN_1
#define LASER_EN_GPIO_CLK_ENABLE()         do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)

#define LASER_ACTIVE_LEVEL                 GPIO_PIN_SET
#define LASER_INACTIVE_LEVEL               GPIO_PIN_RESET

void laser_init(void);
void laser_on(void);
void laser_off(void);
uint8_t laser_get_state(void);

#endif
