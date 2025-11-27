#ifndef __TEC_H
#define __TEC_H

#include "./SYSTEM/sys/sys.h"

#define TEC_EN_GPIO_PORT                 GPIOE
#define TEC_EN_GPIO_PIN                  GPIO_PIN_0
#define TEC_EN_GPIO_CLK_ENABLE()         do{ __HAL_RCC_GPIOE_CLK_ENABLE(); }while(0)
#define TEC_EN_ACTIVE_LEVEL              GPIO_PIN_SET
#define TEC_EN_INACTIVE_LEVEL            GPIO_PIN_RESET

#define TEC_I2C_INSTANCE                 I2C1
#define TEC_I2C_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)
#define TEC_I2C_CLK_ENABLE()             do{ __HAL_RCC_I2C1_CLK_ENABLE(); }while(0)
#define TEC_I2C_SCL_PIN                  GPIO_PIN_8
#define TEC_I2C_SCL_PORT                 GPIOB
#define TEC_I2C_SCL_AF                   GPIO_AF4_I2C1
#define TEC_I2C_SDA_PIN                  GPIO_PIN_9
#define TEC_I2C_SDA_PORT                 GPIOB
#define TEC_I2C_SDA_AF                   GPIO_AF4_I2C1

#define TEC_DIGIPOT_I2C_ADDRESS          0x2FU

#define TEC_I2C_TIMEOUT_MS               50U

#define TEC_DEFAULT_POWER_PERCENT        100U

void tec_init(void);
void tec_on(void);
void tec_off(void);
uint8_t tec_get_state(void);
void tec_set_power(uint8_t power_level);  
uint8_t tec_get_power(void);

#endif
