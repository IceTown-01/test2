#ifndef __VERSION_H
#define __VERSION_H

/******************************************************************************************/
/* 版本信息定义 */

/* 软件版本号 */
#define VERSION_MAJOR           1       /* 主版本号 */
#define VERSION_MINOR           0       /* 次版本号 */
#define VERSION_REVISION        0       /* 修订号 */
#define VERSION_BUILD           1       /* 构建号 */

/* 版本号字符串宏 */
#define VERSION_STRING          "V1.0.0.1"
#define VERSION_STRING_FULL     "V1.0.0.1 Build 1"

/* 编译日期和时间（由编译器自动生成） */
#define BUILD_DATE              __DATE__
#define BUILD_TIME              __TIME__

/* 项目名称 */
#define PROJECT_NAME            "JYG_PRO"
#define PROJECT_DESCRIPTION     "激光和灯控制系统"

/******************************************************************************************/
/* 模块开关宏定义 - 用于调试时打开/关闭各个模块 */

/* 按键模块 */
#define ENABLE_KEY_MODULE               1       /* 1:启用  0:禁用 */

/* 显示模块 */
#define ENABLE_LCD_MODULE               1       /* 1:启用  0:禁用 */

/* 蜂鸣器模块 */
#define ENABLE_BEEP_MODULE              1       /* 1:启用  0:禁用 */

/* 激光控制模块 */
#define ENABLE_LASER_MODULE             1       /* 1:启用  0:禁用 */

/* TEC制冷模块 */
#define ENABLE_TEC_MODULE               1       /* 1:启用  0:禁用 */

/* 风扇控制模块 */
#define ENABLE_FAN_MODULE               1       /* 1:启用  0:禁用 */

/* WSD(灯)控制模块 */
#define ENABLE_WSD_MODULE               1       /* 1:启用  0:禁用 */

/* 运动传感器模块 */
#define ENABLE_MOTION_SENSOR_MODULE     1       /* 1:启用  0:禁用 */

/* 档位控制模块 */
#define ENABLE_LEVEL_CONTROL_MODULE     1       /* 1:启用  0:禁用 */

/* 数字电位器模块 */
#define ENABLE_DIGITAL_POT_MODULE       1       /* 1:启用  0:禁用 */

/* 模式管理模块 */
#define ENABLE_MODE_MANAGER_MODULE      1       /* 1:启用  0:禁用 */

/* 状态机模块 */
#define ENABLE_STATE_MACHINE_MODULE     1       /* 1:启用  0:禁用 */

/* 定时器模块 */
#define ENABLE_TIMER_MODULE             1       /* 1:启用  0:禁用 */

/* 系统初始化模块 */
#define ENABLE_SYSTEM_INIT_MODULE       1       /* 1:启用  0:禁用 */

/******************************************************************************************/
/* 调试功能开关宏 */

/* 串口调试输出 */
#define ENABLE_UART_DEBUG               1       /* 1:启用  0:禁用 */

/* 按键调试输出 */
#define ENABLE_KEY_DEBUG                0       /* 1:启用  0:禁用 */

/* 显示调试输出 */
#define ENABLE_LCD_DEBUG                0       /* 1:启用  0:禁用 */

/* 传感器调试输出 */
#define ENABLE_SENSOR_DEBUG             0       /* 1:启用  0:禁用 */

/* I2C调试输出 */
#define ENABLE_I2C_DEBUG                0       /* 1:启用  0:禁用 */

/* 状态机调试输出 */
#define ENABLE_STATE_MACHINE_DEBUG      0       /* 1:启用  0:禁用 */

/* 模式管理调试输出 */
#define ENABLE_MODE_MANAGER_DEBUG       0       /* 1:启用  0:禁用 */

/******************************************************************************************/
/* 功能特性开关宏 */

/* 运动传感器中断功能（当前未使用，通过轮询方式） */
#define ENABLE_MOTION_SENSOR_INTERRUPT  0       /* 1:启用中断  0:使用轮询 */

/* 低功耗模式 */
#define ENABLE_LOW_POWER_MODE           0       /* 1:启用  0:禁用 */

/* 看门狗功能 */
#define ENABLE_WATCHDOG                 0       /* 1:启用  0:禁用 */


/******************************************************************************************/
/* 版本信息辅助宏 */

/* 获取版本号（组合成一个32位整数） */
#define VERSION_NUMBER                   ((VERSION_MAJOR << 24) | (VERSION_MINOR << 16) | (VERSION_REVISION << 8) | VERSION_BUILD)

/* 版本比较宏 */
#define VERSION_IS_AT_LEAST(major, minor, revision) \
    ((VERSION_MAJOR > (major)) || \
     (VERSION_MAJOR == (major) && VERSION_MINOR > (minor)) || \
     (VERSION_MAJOR == (major) && VERSION_MINOR == (minor) && VERSION_REVISION >= (revision)))

/******************************************************************************************/
/* 模块使能检查宏 - 用于条件编译 */

#if ENABLE_KEY_MODULE
    #define IF_KEY_MODULE_ENABLED(code)  code
#else
    #define IF_KEY_MODULE_ENABLED(code)
#endif

#if ENABLE_LCD_MODULE
    #define IF_LCD_MODULE_ENABLED(code)  code
#else
    #define IF_LCD_MODULE_ENABLED(code)
#endif

#if ENABLE_BEEP_MODULE
    #define IF_BEEP_MODULE_ENABLED(code)  code
#else
    #define IF_BEEP_MODULE_ENABLED(code)
#endif

#if ENABLE_LASER_MODULE
    #define IF_LASER_MODULE_ENABLED(code)  code
#else
    #define IF_LASER_MODULE_ENABLED(code)
#endif

#if ENABLE_TEC_MODULE
    #define IF_TEC_MODULE_ENABLED(code)  code
#else
    #define IF_TEC_MODULE_ENABLED(code)
#endif

#if ENABLE_FAN_MODULE
    #define IF_FAN_MODULE_ENABLED(code)  code
#else
    #define IF_FAN_MODULE_ENABLED(code)
#endif

#if ENABLE_WSD_MODULE
    #define IF_WSD_MODULE_ENABLED(code)  code
#else
    #define IF_WSD_MODULE_ENABLED(code)
#endif

#if ENABLE_MOTION_SENSOR_MODULE
    #define IF_MOTION_SENSOR_MODULE_ENABLED(code)  code
#else
    #define IF_MOTION_SENSOR_MODULE_ENABLED(code)
#endif

#if ENABLE_LEVEL_CONTROL_MODULE
    #define IF_LEVEL_CONTROL_MODULE_ENABLED(code)  code
#else
    #define IF_LEVEL_CONTROL_MODULE_ENABLED(code)
#endif

#if ENABLE_DIGITAL_POT_MODULE
    #define IF_DIGITAL_POT_MODULE_ENABLED(code)  code
#else
    #define IF_DIGITAL_POT_MODULE_ENABLED(code)
#endif

#if ENABLE_MODE_MANAGER_MODULE
    #define IF_MODE_MANAGER_MODULE_ENABLED(code)  code
#else
    #define IF_MODE_MANAGER_MODULE_ENABLED(code)
#endif

#if ENABLE_STATE_MACHINE_MODULE
    #define IF_STATE_MACHINE_MODULE_ENABLED(code)  code
#else
    #define IF_STATE_MACHINE_MODULE_ENABLED(code)
#endif

#if ENABLE_TIMER_MODULE
    #define IF_TIMER_MODULE_ENABLED(code)  code
#else
    #define IF_TIMER_MODULE_ENABLED(code)
#endif

/******************************************************************************************/
/* 调试输出宏 */

#if ENABLE_UART_DEBUG
    #include "./SYSTEM/usart/usart.h"
    #define DEBUG_PRINT(fmt, ...)    printf(fmt, ##__VA_ARGS__)
    #define DEBUG_PRINTLN(str)       printf(str "\r\n")
#else
    #define DEBUG_PRINT(fmt, ...)
    #define DEBUG_PRINTLN(str)
#endif

#if ENABLE_KEY_DEBUG && ENABLE_UART_DEBUG
    #define KEY_DEBUG_PRINT(fmt, ...)    printf("[KEY] " fmt, ##__VA_ARGS__)
    #define KEY_DEBUG_PRINTLN(str)       printf("[KEY] " str "\r\n")
#else
    #define KEY_DEBUG_PRINT(fmt, ...)
    #define KEY_DEBUG_PRINTLN(str)
#endif

#if ENABLE_LCD_DEBUG && ENABLE_UART_DEBUG
    #define LCD_DEBUG_PRINT(fmt, ...)    printf("[LCD] " fmt, ##__VA_ARGS__)
    #define LCD_DEBUG_PRINTLN(str)       printf("[LCD] " str "\r\n")
#else
    #define LCD_DEBUG_PRINT(fmt, ...)
    #define LCD_DEBUG_PRINTLN(str)
#endif

#if ENABLE_SENSOR_DEBUG && ENABLE_UART_DEBUG
    #define SENSOR_DEBUG_PRINT(fmt, ...)    printf("[SENSOR] " fmt, ##__VA_ARGS__)
    #define SENSOR_DEBUG_PRINTLN(str)       printf("[SENSOR] " str "\r\n")
#else
    #define SENSOR_DEBUG_PRINT(fmt, ...)
    #define SENSOR_DEBUG_PRINTLN(str)
#endif

#if ENABLE_I2C_DEBUG && ENABLE_UART_DEBUG
    #define I2C_DEBUG_PRINT(fmt, ...)    printf("[I2C] " fmt, ##__VA_ARGS__)
    #define I2C_DEBUG_PRINTLN(str)       printf("[I2C] " str "\r\n")
#else
    #define I2C_DEBUG_PRINT(fmt, ...)
    #define I2C_DEBUG_PRINTLN(str)
#endif

#if ENABLE_STATE_MACHINE_DEBUG && ENABLE_UART_DEBUG
    #define STATE_DEBUG_PRINT(fmt, ...)    printf("[STATE] " fmt, ##__VA_ARGS__)
    #define STATE_DEBUG_PRINTLN(str)       printf("[STATE] " str "\r\n")
#else
    #define STATE_DEBUG_PRINT(fmt, ...)
    #define STATE_DEBUG_PRINTLN(str)
#endif

#if ENABLE_MODE_MANAGER_DEBUG && ENABLE_UART_DEBUG
    #define MODE_DEBUG_PRINT(fmt, ...)    printf("[MODE] " fmt, ##__VA_ARGS__)
    #define MODE_DEBUG_PRINTLN(str)       printf("[MODE] " str "\r\n")
#else
    #define MODE_DEBUG_PRINT(fmt, ...)
    #define MODE_DEBUG_PRINTLN(str)
#endif

/******************************************************************************************/
/* 函数声明 */

/* 获取版本信息字符串 */
const char* version_get_string(void);
const char* version_get_full_string(void);
uint32_t version_get_number(void);

#endif /* __VERSION_H */

