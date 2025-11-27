#ifndef __WSD_H
#define __WSD_H

#include "./SYSTEM/sys/sys.h"

#define WSD_EN_GPIO_PORT                 GPIOE
#define WSD_EN_GPIO_PIN                  GPIO_PIN_15
#define WSD_EN_GPIO_CLK_ENABLE()         do{ __HAL_RCC_GPIOE_CLK_ENABLE(); }while(0)
#define WSD_EN_ACTIVE_LEVEL              GPIO_PIN_SET
#define WSD_EN_INACTIVE_LEVEL            GPIO_PIN_RESET

#define WSD_I2C_INSTANCE                 I2C2
#define WSD_I2C_CLK_ENABLE()             do{ __HAL_RCC_I2C2_CLK_ENABLE(); }while(0)
#define WSD_I2C_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)
#define WSD_I2C_SCL_PIN                  GPIO_PIN_10
#define WSD_I2C_SCL_PORT                 GPIOB
#define WSD_I2C_SCL_AF                   GPIO_AF4_I2C2
#define WSD_I2C_SDA_PIN                  GPIO_PIN_11
#define WSD_I2C_SDA_PORT                 GPIOB
#define WSD_I2C_SDA_AF                   GPIO_AF4_I2C2

#define WSD_DIGIPOT_I2C_ADDRESS          0x2FU

#define WSD_I2C_TIMEOUT_MS               50U

#define WSD_LEVEL_MIN                    1U
#define WSD_LEVEL_MAX                    5U

void wsd_init(void);
void wsd_on(void);
void wsd_off(void);
uint8_t wsd_is_on(void);
void wsd_set_level(uint8_t level);
uint8_t wsd_get_level(void);

#endif
