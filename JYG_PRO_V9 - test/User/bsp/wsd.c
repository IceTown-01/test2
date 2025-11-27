#include "wsd.h"

static I2C_HandleTypeDef s_wsd_i2c;
static uint8_t s_wsd_initialized = 0U;
static uint8_t s_wsd_enabled = 0U;
static uint8_t s_wsd_i2c_ready = 0U;
static uint8_t s_wsd_level = WSD_LEVEL_MIN;

#ifndef WSD_I2C_TIMING_VALUE
#define WSD_I2C_TIMING_VALUE              0x30A0A7FBU   
#endif

#define WSD_I2C_MAX_RETRY                 3U

/* 档位到数字电位器值的映射表
 * 档位1 (4V):  0x7F (127) - 最低档
 * 档位2 (6V):  0x60 (96)
 * 档位3 (8V):  0x40 (64)
 * 档位4 (10V): 0x20 (32)
 * 档位5 (13V): 0x18 (24)  - 最高档
 * 0x15U---------
 * 注意: 如果5档电压不准确，可能需要微调0x18这个值
 *       电压偏高则增大该值，电压偏低则减小该值
 */
static const uint8_t s_level_wiper_map[WSD_LEVEL_MAX] = {
    0x19U,  
    0x0FU,  
    0x0CU,  
    0x0BU,  
    0x0AU   
};

static inline uint8_t wsd_clamp_level(uint8_t level)
{
    if (level < WSD_LEVEL_MIN) return WSD_LEVEL_MIN;
    if (level > WSD_LEVEL_MAX) return WSD_LEVEL_MAX;
    return level;
}

static void wsd_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    WSD_EN_GPIO_CLK_ENABLE();

    gpio.Pin = WSD_EN_GPIO_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(WSD_EN_GPIO_PORT, &gpio);

    HAL_GPIO_WritePin(WSD_EN_GPIO_PORT, WSD_EN_GPIO_PIN, WSD_EN_INACTIVE_LEVEL);
}

static void wsd_i2c_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    WSD_I2C_GPIO_CLK_ENABLE();
    WSD_I2C_CLK_ENABLE();

    gpio.Pin = WSD_I2C_SCL_PIN;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = WSD_I2C_SCL_AF;
    HAL_GPIO_Init(WSD_I2C_SCL_PORT, &gpio);

    gpio.Pin = WSD_I2C_SDA_PIN;
    gpio.Alternate = WSD_I2C_SDA_AF;
    HAL_GPIO_Init(WSD_I2C_SDA_PORT, &gpio);

    s_wsd_i2c.Instance = WSD_I2C_INSTANCE;
    s_wsd_i2c.Init.Timing = WSD_I2C_TIMING_VALUE;
    s_wsd_i2c.Init.OwnAddress1 = 0U;
    s_wsd_i2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    s_wsd_i2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    s_wsd_i2c.Init.OwnAddress2 = 0U;
    s_wsd_i2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    s_wsd_i2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    s_wsd_i2c_ready = 0U;
    (void)HAL_I2C_DeInit(&s_wsd_i2c);
    if (HAL_I2C_Init(&s_wsd_i2c) == HAL_OK)
    {
        (void)HAL_I2CEx_ConfigAnalogFilter(&s_wsd_i2c, I2C_ANALOGFILTER_ENABLE);
        (void)HAL_I2CEx_ConfigDigitalFilter(&s_wsd_i2c, 0U);
        s_wsd_i2c_ready = 1U;
    }
}

static HAL_StatusTypeDef wsd_write_wiper(uint8_t wiper_value)
{
    HAL_StatusTypeDef status = HAL_ERROR;
    uint8_t retry = 0U;

    if (!s_wsd_i2c_ready)
    {
        return HAL_ERROR;
    }

    while (retry < WSD_I2C_MAX_RETRY)
    {
        if (HAL_I2C_IsDeviceReady(&s_wsd_i2c, (WSD_DIGIPOT_I2C_ADDRESS << 1U), 1U, WSD_I2C_TIMEOUT_MS) == HAL_OK)
        {
            status = HAL_I2C_Master_Transmit(&s_wsd_i2c,
                                             (WSD_DIGIPOT_I2C_ADDRESS << 1U),
                                             &wiper_value,
                                             1U,
                                             WSD_I2C_TIMEOUT_MS);
            if (status == HAL_OK)
            {
                break;
            }
        }
        retry++;
        HAL_Delay(2U);
    }

    return status;
}

static uint8_t wsd_level_to_wiper(uint8_t level)
{
    uint8_t idx = wsd_clamp_level(level) - 1U;
    return s_level_wiper_map[idx];
}

void wsd_init(void)
{
    if (s_wsd_initialized)
    {
        return;
    }

    wsd_gpio_init();
    wsd_i2c_init();

    s_wsd_initialized = 1U;
    s_wsd_enabled = 0U;

    if (s_wsd_i2c_ready)
    {
        (void)wsd_write_wiper(wsd_level_to_wiper(s_wsd_level));
    }
}

void wsd_on(void)
{
    if (!s_wsd_initialized)
    {
        wsd_init();
    }

    HAL_GPIO_WritePin(WSD_EN_GPIO_PORT, WSD_EN_GPIO_PIN, WSD_EN_ACTIVE_LEVEL);
    s_wsd_enabled = 1U;

    if (s_wsd_i2c_ready)
    {
        (void)wsd_write_wiper(wsd_level_to_wiper(s_wsd_level));
    }
}

void wsd_off(void)
{
    if (!s_wsd_initialized)
    {
        wsd_init();
    }

    HAL_GPIO_WritePin(WSD_EN_GPIO_PORT, WSD_EN_GPIO_PIN, WSD_EN_INACTIVE_LEVEL);
    s_wsd_enabled = 0U;
}

uint8_t wsd_is_on(void)
{
    return s_wsd_enabled;
}

void wsd_set_level(uint8_t level)
{
    uint8_t clamped = wsd_clamp_level(level);
    s_wsd_level = clamped;

    if (!s_wsd_initialized)
    {
        wsd_init();
    }

    if (s_wsd_i2c_ready)
    {
        (void)wsd_write_wiper(wsd_level_to_wiper(clamped));
    }
}

uint8_t wsd_get_level(void)
{
    return s_wsd_level;
}
