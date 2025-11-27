#ifndef __MOTION_SENSOR_H
#define __MOTION_SENSOR_H

#include "./SYSTEM/sys/sys.h"

#define MOTION_SENSOR_I2C_INSTANCE            I2C3
#define MOTION_SENSOR_I2C_CLK_ENABLE()        do{ __HAL_RCC_I2C3_CLK_ENABLE(); }while(0)
#define MOTION_SENSOR_I2C_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)
#define MOTION_SENSOR_I2C_SCL_PIN             GPIO_PIN_9
#define MOTION_SENSOR_I2C_SCL_PORT            GPIOC
#define MOTION_SENSOR_I2C_SCL_AF              GPIO_AF4_I2C3
#define MOTION_SENSOR_I2C_SDA_PIN             GPIO_PIN_8
#define MOTION_SENSOR_I2C_SDA_PORT            GPIOA
#define MOTION_SENSOR_I2C_SDA_AF              GPIO_AF4_I2C3

#define MOTION_SENSOR_AD0_PORT                GPIOB
#define MOTION_SENSOR_AD0_PIN                 GPIO_PIN_12
#define MOTION_SENSOR_AD0_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

#define MOTION_SENSOR_I2C_ADDRESS             0x69U

#ifndef MOTION_SENSOR_I2C_TIMING
#define MOTION_SENSOR_I2C_TIMING              0x30A0A7FBU  
#endif
#define MOTION_SENSOR_I2C_TIMEOUT_MS          50U

#ifndef MOTION_SENSOR_USE_SOFT_I2C
#define MOTION_SENSOR_USE_SOFT_I2C            1
#endif

#ifndef MOTION_SENSOR_ACCEL_SENSITIVITY_LSB_PER_G
#define MOTION_SENSOR_ACCEL_SENSITIVITY_LSB_PER_G    8192
#endif

#ifndef MOTION_SENSOR_GRAVITY_MG
#define MOTION_SENSOR_GRAVITY_MG                     1000
#endif

#ifndef MOTION_SENSOR_MOVING_THRESHOLD_MG
#define MOTION_SENSOR_MOVING_THRESHOLD_MG            150
#endif

#ifndef MOTION_SENSOR_STATIC_THRESHOLD_MG
#define MOTION_SENSOR_STATIC_THRESHOLD_MG            60
#endif

#ifndef MOTION_SENSOR_MOVING_DEBOUNCE_COUNT
#define MOTION_SENSOR_MOVING_DEBOUNCE_COUNT          3
#endif

#ifndef MOTION_SENSOR_STATIC_DEBOUNCE_COUNT
#define MOTION_SENSOR_STATIC_DEBOUNCE_COUNT          10
#endif

#ifndef MOTION_SENSOR_BASELINE_FILTER_COEFF
#define MOTION_SENSOR_BASELINE_FILTER_COEFF          16
#endif

#ifndef MOTION_SENSOR_BASELINE_TRACK_THRESHOLD_MG
#define MOTION_SENSOR_BASELINE_TRACK_THRESHOLD_MG    40
#endif

#ifndef MOTION_SENSOR_MIN_VALID_NORM_MG
#define MOTION_SENSOR_MIN_VALID_NORM_MG              200
#endif

#ifndef MOTION_SENSOR_MAX_VALID_NORM_MG
#define MOTION_SENSOR_MAX_VALID_NORM_MG              4000
#endif

#define MOTION_SENSOR_STATIC_PAUSE_MS         2000U                 
#define MOTION_SENSOR_STATIC_SHUTDOWN_MS      (5U * 60U * 1000U)    

/* 灵敏度等级定义：1级最不灵敏，5级最灵敏 */
#define MOTION_SENSOR_SENSITIVITY_LEVEL_MIN   1U
#define MOTION_SENSOR_SENSITIVITY_LEVEL_MAX   5U
#define MOTION_SENSOR_SENSITIVITY_LEVEL_DEFAULT  1U

void motion_sensor_init(void);
uint8_t motion_sensor_is_moving(void);  
void motion_sensor_enable(void);         
void motion_sensor_disable(void);        
void motion_sensor_reset_static_timer(void);  
uint32_t motion_sensor_get_static_time(void);
void motion_sensor_set_sensitivity_level(uint8_t level);
uint8_t motion_sensor_get_sensitivity_level(void); 

#endif
