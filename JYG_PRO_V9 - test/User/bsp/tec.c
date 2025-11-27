#include "tec.h"

static I2C_HandleTypeDef s_tec_i2c;
static uint8_t s_tec_initialized = 0U;
static uint8_t s_tec_enabled = 0U;
static uint8_t s_tec_power_percent = TEC_DEFAULT_POWER_PERCENT;

#ifndef TEC_I2C_TIMING_VALUE
#define TEC_I2C_TIMING_VALUE              0x30A0A7FBU
#endif

#define TEC_I2C_MAX_RETRY                 3U

static inline uint8_t tec_clamp_percent(uint8_t value)
{
    return (value > 127U) ? 127U : value;
}

static uint8_t tec_percent_to_wiper(uint8_t percent)
{
    /* 反转映射：功率百分比越高，数字电位器值越小
     * 100%功率 → 0x00 (最小值，输出最大电压24V)
     * 0%功率 → 0x7F (最大值，输出最小电压)
     */
//    uint32_t scaled = ((uint32_t)(100U - percent) * 127U + 50U) / 100U;
//		uint32_t scaled = ((uint32_t)(1/(percent/100*24-1))) * 127U;
//    if (scaled > 0x7FU)
//    {
//        scaled = 0x7FU;
//    }
	
	    if (percent > 0x7FU)
    {
        percent = 0x7FU;
    }
    return (uint8_t)percent;
}

static HAL_StatusTypeDef tec_write_wiper(uint8_t wiper_value)
{
    HAL_StatusTypeDef status = HAL_ERROR;
    uint8_t retry = 0U;

    while (retry < TEC_I2C_MAX_RETRY)
    {
        if (HAL_I2C_IsDeviceReady(&s_tec_i2c, (TEC_DIGIPOT_I2C_ADDRESS << 1U), 1U, TEC_I2C_TIMEOUT_MS) == HAL_OK)
        {
            status = HAL_I2C_Master_Transmit(&s_tec_i2c,
                                             (TEC_DIGIPOT_I2C_ADDRESS << 1U),
                                             &wiper_value,
                                             1U,
                                             TEC_I2C_TIMEOUT_MS);
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

static void tec_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    TEC_EN_GPIO_CLK_ENABLE();

    gpio.Pin = TEC_EN_GPIO_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TEC_EN_GPIO_PORT, &gpio);

    HAL_GPIO_WritePin(TEC_EN_GPIO_PORT, TEC_EN_GPIO_PIN, TEC_EN_INACTIVE_LEVEL);
}

static void tec_i2c_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    
    TEC_I2C_GPIO_CLK_ENABLE();
    TEC_I2C_CLK_ENABLE();

    
    gpio.Pin = TEC_I2C_SCL_PIN;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = TEC_I2C_SCL_AF;
    HAL_GPIO_Init(TEC_I2C_SCL_PORT, &gpio);

    gpio.Pin = TEC_I2C_SDA_PIN;
    gpio.Alternate = TEC_I2C_SDA_AF;
    HAL_GPIO_Init(TEC_I2C_SDA_PORT, &gpio);

    s_tec_i2c.Instance = TEC_I2C_INSTANCE;
    s_tec_i2c.Init.Timing = TEC_I2C_TIMING_VALUE;
    s_tec_i2c.Init.OwnAddress1 = 0U;
    s_tec_i2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    s_tec_i2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    s_tec_i2c.Init.OwnAddress2 = 0U;
    s_tec_i2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    s_tec_i2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    (void)HAL_I2C_DeInit(&s_tec_i2c);
    if (HAL_I2C_Init(&s_tec_i2c) != HAL_OK)
    {
        
        return;
    }

    
    (void)HAL_I2CEx_ConfigAnalogFilter(&s_tec_i2c, I2C_ANALOGFILTER_ENABLE);
    (void)HAL_I2CEx_ConfigDigitalFilter(&s_tec_i2c, 0U);
}

void tec_init(void)
{
    if (s_tec_initialized)
    {
        return;
    }

    tec_gpio_init();
    tec_i2c_init();

    if (HAL_I2C_GetState(&s_tec_i2c) == HAL_I2C_STATE_RESET)
    {
        return;
    }

    s_tec_initialized = 1U;
    s_tec_enabled = 0U;
    (void)tec_write_wiper(tec_percent_to_wiper(s_tec_power_percent));
}

void tec_on(void)
{
    if (!s_tec_initialized)
    {
        tec_init();
    }

    HAL_GPIO_WritePin(TEC_EN_GPIO_PORT, TEC_EN_GPIO_PIN, TEC_EN_ACTIVE_LEVEL);
    s_tec_enabled = 1U;

    
    (void)tec_write_wiper(tec_percent_to_wiper(s_tec_power_percent));
}

void tec_off(void)
{
    if (!s_tec_initialized)
    {
        tec_init();
    }

    HAL_GPIO_WritePin(TEC_EN_GPIO_PORT, TEC_EN_GPIO_PIN, TEC_EN_INACTIVE_LEVEL);
    s_tec_enabled = 0U;
}

uint8_t tec_get_state(void)
{
    return s_tec_enabled;
}

void tec_set_power(uint8_t power_level)
{
    uint8_t clamped = tec_clamp_percent(power_level);
    s_tec_power_percent = clamped;

    if (!s_tec_initialized)
    {
        tec_init();
    }

    (void)tec_write_wiper(tec_percent_to_wiper(clamped));
}

uint8_t tec_get_power(void)
{
    return s_tec_power_percent;
}
