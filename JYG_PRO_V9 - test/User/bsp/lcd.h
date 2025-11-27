#ifndef __LCD_H
#define __LCD_H

#ifdef __cplusplus
extern "C" {
#endif

/* 包含必要的头文件 */
#include "./SYSTEM/sys/sys.h"

/* 屏幕分辨率定义 (请根据您的JD9613屏幕实际参数修改) */
#define LCD_WIDTH   126
#define LCD_HEIGHT  294

/* MADCTL寄存器相关位定义（用于控制扫描方向与颜色顺序） */
#define LCD_MADCTL_MY      0x80U
#define LCD_MADCTL_MX      0x40U
#define LCD_MADCTL_MV      0x20U
#define LCD_MADCTL_BGR     0x08U

/* 初始MADCTL配置：上下对调 + 左右翻转 */
#ifndef LCD_MADCTL_INIT_VALUE
#define LCD_MADCTL_INIT_VALUE   (LCD_MADCTL_MY | LCD_MADCTL_MX)
#endif

/* 引脚定义 - 根据您的连接修改 */
#define LCD_CS_PIN       GPIO_PIN_0
#define LCD_CS_PORT      GPIOB
#define LCD_DC_PIN       GPIO_PIN_1
#define LCD_DC_PORT      GPIOB
#define LCD_RST_PIN      GPIO_PIN_2
#define LCD_RST_PORT     GPIOB

/* SPI接口引脚（默认使用 SPI1: PA5=SCK, PA7=MOSI, 可根据实际硬件换到PB3/PB5） */
#define LCD_SPI_INSTANCE   SPI1
#define LCD_SPI_AF         GPIO_AF5_SPI1

/* 默认走PA口，如果需要改为PB3/PB5，请改下面三行 */
#define LCD_SCK_PORT       GPIOA
#define LCD_SCK_PIN        GPIO_PIN_5   /* 备选: PB3 */
#define LCD_MOSI_PORT      GPIOA
#define LCD_MOSI_PIN       GPIO_PIN_7   /* 备选: PB5 */
/* 如实际连有MISO，可放开并配置：PB4或PA6（AF5） */
// #define LCD_MISO_PORT      GPIOA
// #define LCD_MISO_PIN       GPIO_PIN_6   /* 备选: PB4 */

/* SPI频率配置（基于PLL2P=220MHz）：
 * 可选值：SPI_BAUDRATEPRESCALER_2   -> 110MHz
 *        SPI_BAUDRATEPRESCALER_4   -> 55MHz
 *        SPI_BAUDRATEPRESCALER_8   -> 27.5MHz
 *        SPI_BAUDRATEPRESCALER_16  -> 13.75MHz
 *        SPI_BAUDRATEPRESCALER_32  -> 6.875MHz
 *        SPI_BAUDRATEPRESCALER_64  -> 3.44MHz
 *        SPI_BAUDRATEPRESCALER_128 -> 1.72MHz
 *        SPI_BAUDRATEPRESCALER_256 -> 0.86MHz
 * 
 * JD9613显示屏推荐频率：根据数据手册，通常支持10MHz左右的SPI时钟
 * 初始化建议使用较低频率（5-10MHz），正常运行可用更高频率（10-15MHz）
 * 当前配置：32分频 -> 6.875MHz，适合JD9613显示屏
 */
#ifndef LCD_SPI_PRESCALER
#define LCD_SPI_PRESCALER  SPI_BAUDRATEPRESCALER_32  // 6.875MHz，JD9613推荐频率
#endif

/* 函数声明 */
void LCD_WriteCommand(uint8_t cmd);
void LCD_WriteData(uint8_t data);
void LCD_WriteData16(uint16_t data);
void LCD_Reset(void);
void LCD_Init(void);
void LCD_Clear(uint16_t color);
void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void LCD_FillRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void LCD_ShowFourColors(void);
void LCD_ShowImage(const uint16_t *img_data);
void LCD_ShowPartialImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *img_data);
void LCD_ShowImageBytes(const uint8_t *img_bytes);
void LCD_ShowPartialImageBytes(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *img_bytes);
void LCD_ShowImageBytesOrder(const uint8_t *img_bytes, uint8_t byte_order);
void LCD_ShowPartialImage16Gray(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *img_bytes);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_H */
