#include "motion_sensor.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define REG_DEVICE_CONFIG           0x11U
#define REG_WHO_AM_I                0x75U
#define REG_PWR_MGMT0               0x4EU
#define REG_GYRO_CONFIG0            0x4FU
#define REG_ACCEL_CONFIG0           0x50U
#define REG_ACCEL_DATA_X1           0x1FU

#define ICM42688_WHO_AM_I_VALUE     0x47U

#if !MOTION_SENSOR_USE_SOFT_I2C
static I2C_HandleTypeDef s_motion_i2c = {0};
#endif
static uint8_t s_initialized = 0U;
static uint8_t s_enabled = 0U;
static uint8_t s_i2c_ready = 0U;
static uint8_t s_device_available = 0U;
static uint8_t s_i2c_address = MOTION_SENSOR_I2C_ADDRESS;

static uint8_t s_sample_valid = 0U;
static int32_t s_norm_baseline_mg = MOTION_SENSOR_GRAVITY_MG;
static uint8_t s_motion_state = 0U;
static uint8_t s_motion_confirm_count = 0U;
static uint8_t s_static_confirm_count = 0U;
static uint32_t s_last_motion_tick = 0U;
static uint8_t s_last_motion_valid = 0U;
static int16_t s_last_sample[3] = {0};
static uint8_t s_last_sample_valid = 0U;
static int32_t s_last_valid_norm_mg = MOTION_SENSOR_GRAVITY_MG;
static int16_t s_prev_sample[3] = {0};
static uint8_t s_prev_sample_valid = 0U;
static uint8_t s_sensitivity_level = MOTION_SENSOR_SENSITIVITY_LEVEL_DEFAULT;

static uint32_t motion_sensor_get_moving_threshold(void)
{
    const uint32_t thresholds[] = {100U, 95U, 90U, 85U, 80U};
    uint8_t idx = (s_sensitivity_level >= MOTION_SENSOR_SENSITIVITY_LEVEL_MIN && 
                    s_sensitivity_level <= MOTION_SENSOR_SENSITIVITY_LEVEL_MAX) ? 
                   (s_sensitivity_level - 1U) : 0U;
    return thresholds[idx];
}

static uint32_t motion_sensor_get_static_threshold(void)
{
    const uint32_t thresholds[] = {40U, 38U, 35U, 32U, 30U};
    uint8_t idx = (s_sensitivity_level >= MOTION_SENSOR_SENSITIVITY_LEVEL_MIN && 
                    s_sensitivity_level <= MOTION_SENSOR_SENSITIVITY_LEVEL_MAX) ? 
                   (s_sensitivity_level - 1U) : 0U;
    return thresholds[idx];
}

static uint8_t motion_sensor_get_moving_debounce(void)
{
    const uint8_t debounces[] = {2U, 2U, 2U, 1U, 1U};
    uint8_t idx = (s_sensitivity_level >= MOTION_SENSOR_SENSITIVITY_LEVEL_MIN && 
                    s_sensitivity_level <= MOTION_SENSOR_SENSITIVITY_LEVEL_MAX) ? 
                   (s_sensitivity_level - 1U) : 0U;
    return debounces[idx];
}

static uint8_t motion_sensor_get_static_debounce(void)
{
    const uint8_t debounces[] = {6U, 6U, 6U, 5U, 5U};
    uint8_t idx = (s_sensitivity_level >= MOTION_SENSOR_SENSITIVITY_LEVEL_MIN && 
                    s_sensitivity_level <= MOTION_SENSOR_SENSITIVITY_LEVEL_MAX) ? 
                   (s_sensitivity_level - 1U) : 0U;
    return debounces[idx];
}

#if MOTION_SENSOR_USE_SOFT_I2C
#ifndef MOTION_SENSOR_SOFT_I2C_DELAY_CYCLES
#define MOTION_SENSOR_SOFT_I2C_DELAY_CYCLES   500U
#endif

static void motion_sensor_soft_i2c_delay(void)
{
    for (volatile uint32_t i = 0U; i < MOTION_SENSOR_SOFT_I2C_DELAY_CYCLES; i++)
    {
        __NOP();
    }
}

static void motion_sensor_soft_i2c_set_sda_mode(uint32_t mode)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = MOTION_SENSOR_I2C_SDA_PIN;
    gpio.Mode = mode;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(MOTION_SENSOR_I2C_SDA_PORT, &gpio);
}

static void motion_sensor_soft_i2c_set_sda(uint8_t high)
{
    HAL_GPIO_WritePin(MOTION_SENSOR_I2C_SDA_PORT,
                      MOTION_SENSOR_I2C_SDA_PIN,
                      high ? GPIO_PIN_SET : GPIO_PIN_RESET);
    motion_sensor_soft_i2c_delay();
}

static void motion_sensor_soft_i2c_set_scl(uint8_t high)
{
    HAL_GPIO_WritePin(MOTION_SENSOR_I2C_SCL_PORT,
                      MOTION_SENSOR_I2C_SCL_PIN,
                      high ? GPIO_PIN_SET : GPIO_PIN_RESET);
    motion_sensor_soft_i2c_delay();
}

static uint8_t motion_sensor_soft_i2c_read_sda(void)
{
    return (uint8_t)HAL_GPIO_ReadPin(MOTION_SENSOR_I2C_SDA_PORT, MOTION_SENSOR_I2C_SDA_PIN);
}

static void motion_sensor_soft_i2c_start(void)
{
    motion_sensor_soft_i2c_set_sda_mode(GPIO_MODE_OUTPUT_OD);
    motion_sensor_soft_i2c_set_sda(1U);
    motion_sensor_soft_i2c_set_scl(1U);
    motion_sensor_soft_i2c_set_sda(0U);
    motion_sensor_soft_i2c_set_scl(0U);
}

static void motion_sensor_soft_i2c_stop(void)
{
    motion_sensor_soft_i2c_set_sda_mode(GPIO_MODE_OUTPUT_OD);
    motion_sensor_soft_i2c_set_sda(0U);
    motion_sensor_soft_i2c_set_scl(1U);
    motion_sensor_soft_i2c_set_sda(1U);
}

static uint8_t motion_sensor_soft_i2c_write_byte(uint8_t value)
{
    motion_sensor_soft_i2c_set_sda_mode(GPIO_MODE_OUTPUT_OD);
    for (uint8_t i = 0U; i < 8U; i++)
    {
        uint8_t bit = (value & 0x80U) ? 1U : 0U;
        motion_sensor_soft_i2c_set_sda(bit);
        motion_sensor_soft_i2c_set_scl(1U);
        motion_sensor_soft_i2c_set_scl(0U);
        value <<= 1U;
    }

    motion_sensor_soft_i2c_set_sda_mode(GPIO_MODE_INPUT);
    motion_sensor_soft_i2c_set_scl(1U);
    uint8_t ack = (motion_sensor_soft_i2c_read_sda() == 0U) ? 1U : 0U;
    motion_sensor_soft_i2c_set_scl(0U);
    motion_sensor_soft_i2c_set_sda_mode(GPIO_MODE_OUTPUT_OD);
    motion_sensor_soft_i2c_set_sda(1U);
    return ack;
}

static uint8_t motion_sensor_soft_i2c_read_byte(uint8_t nack)
{
    uint8_t value = 0U;
    motion_sensor_soft_i2c_set_sda_mode(GPIO_MODE_INPUT);
    for (uint8_t i = 0U; i < 8U; i++)
    {
        motion_sensor_soft_i2c_set_scl(1U);
        value <<= 1U;
        if (motion_sensor_soft_i2c_read_sda())
        {
            value |= 0x01U;
        }
        motion_sensor_soft_i2c_set_scl(0U);
    }

    motion_sensor_soft_i2c_set_sda_mode(GPIO_MODE_OUTPUT_OD);
    motion_sensor_soft_i2c_set_sda(nack ? 1U : 0U);
    motion_sensor_soft_i2c_set_scl(1U);
    motion_sensor_soft_i2c_set_scl(0U);
    motion_sensor_soft_i2c_set_sda(1U);

    return value;
}

static HAL_StatusTypeDef motion_sensor_soft_i2c_mem_write(uint8_t address, uint8_t reg, const uint8_t *buffer, uint16_t len)
{
    motion_sensor_soft_i2c_start();
    if (!motion_sensor_soft_i2c_write_byte((address << 1U) | 0x00U))
    {
        motion_sensor_soft_i2c_stop();
        return HAL_ERROR;
    }
    if (!motion_sensor_soft_i2c_write_byte(reg))
    {
        motion_sensor_soft_i2c_stop();
        return HAL_ERROR;
    }
    for (uint16_t i = 0U; i < len; i++)
    {
        if (!motion_sensor_soft_i2c_write_byte(buffer[i]))
        {
            motion_sensor_soft_i2c_stop();
            return HAL_ERROR;
        }
    }
    motion_sensor_soft_i2c_stop();
    return HAL_OK;
}

static HAL_StatusTypeDef motion_sensor_soft_i2c_mem_read(uint8_t address, uint8_t reg, uint8_t *buffer, uint16_t len)
{
    motion_sensor_soft_i2c_start();
    if (!motion_sensor_soft_i2c_write_byte((address << 1U) | 0x00U))
    {
        motion_sensor_soft_i2c_stop();
        return HAL_ERROR;
    }
    if (!motion_sensor_soft_i2c_write_byte(reg))
    {
        motion_sensor_soft_i2c_stop();
        return HAL_ERROR;
    }

    motion_sensor_soft_i2c_start();
    if (!motion_sensor_soft_i2c_write_byte((address << 1U) | 0x01U))
    {
        motion_sensor_soft_i2c_stop();
        return HAL_ERROR;
    }

    for (uint16_t i = 0U; i < len; i++)
    {
        uint8_t nack = (i + 1U == len) ? 1U : 0U;
        buffer[i] = motion_sensor_soft_i2c_read_byte(nack);
    }

    motion_sensor_soft_i2c_stop();
    return HAL_OK;
}
#endif

static void motion_sensor_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    MOTION_SENSOR_I2C_GPIO_CLK_ENABLE();
    MOTION_SENSOR_AD0_GPIO_CLK_ENABLE();

    gpio.Pin = MOTION_SENSOR_I2C_SCL_PIN;
#if MOTION_SENSOR_USE_SOFT_I2C
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
#else
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Alternate = MOTION_SENSOR_I2C_SCL_AF;
#endif
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(MOTION_SENSOR_I2C_SCL_PORT, &gpio);

    gpio.Pin = MOTION_SENSOR_I2C_SDA_PIN;
#if MOTION_SENSOR_USE_SOFT_I2C
    gpio.Mode = GPIO_MODE_OUTPUT_OD;
    gpio.Pull = GPIO_PULLUP;
#else
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_PULLUP;
    gpio.Alternate = MOTION_SENSOR_I2C_SDA_AF;
#endif
    HAL_GPIO_Init(MOTION_SENSOR_I2C_SDA_PORT, &gpio);

    gpio.Pin = MOTION_SENSOR_AD0_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MOTION_SENSOR_AD0_PORT, &gpio);
    HAL_GPIO_WritePin(MOTION_SENSOR_AD0_PORT, MOTION_SENSOR_AD0_PIN, GPIO_PIN_SET);

#if MOTION_SENSOR_USE_SOFT_I2C
    HAL_GPIO_WritePin(MOTION_SENSOR_I2C_SCL_PORT, MOTION_SENSOR_I2C_SCL_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTION_SENSOR_I2C_SDA_PORT, MOTION_SENSOR_I2C_SDA_PIN, GPIO_PIN_SET);
#endif
}

static void motion_sensor_i2c_init(void)
{
#if MOTION_SENSOR_USE_SOFT_I2C
    s_i2c_ready = 1U;
#else
    MOTION_SENSOR_I2C_CLK_ENABLE();

    s_motion_i2c.Instance = MOTION_SENSOR_I2C_INSTANCE;
    s_motion_i2c.Init.Timing = MOTION_SENSOR_I2C_TIMING;
    s_motion_i2c.Init.OwnAddress1 = 0U;
    s_motion_i2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    s_motion_i2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    s_motion_i2c.Init.OwnAddress2 = 0U;
    s_motion_i2c.Init.OwnAddress2Masks = 0U;
    s_motion_i2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    s_motion_i2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    s_i2c_ready = 0U;
    (void)HAL_I2C_DeInit(&s_motion_i2c);
    if (HAL_I2C_Init(&s_motion_i2c) == HAL_OK)
    {
        (void)HAL_I2CEx_ConfigAnalogFilter(&s_motion_i2c, I2C_ANALOGFILTER_ENABLE);
        (void)HAL_I2CEx_ConfigDigitalFilter(&s_motion_i2c, 0U);
        s_i2c_ready = 1U;
    }
#endif
}

static HAL_StatusTypeDef motion_sensor_write_reg_addr(uint8_t address, uint8_t reg, uint8_t value)
{
    if (!s_i2c_ready)
    {
        return HAL_ERROR;
    }

#if MOTION_SENSOR_USE_SOFT_I2C
    return motion_sensor_soft_i2c_mem_write(address, reg, &value, 1U);
#else
    return HAL_I2C_Mem_Write(&s_motion_i2c,
                             (address << 1U),
                             reg,
                             I2C_MEMADD_SIZE_8BIT,
                             &value,
                             1U,
                             MOTION_SENSOR_I2C_TIMEOUT_MS);
#endif
}

static HAL_StatusTypeDef motion_sensor_read_regs_addr(uint8_t address, uint8_t reg, uint8_t *buffer, uint16_t len)
{
    if (!s_i2c_ready)
    {
        return HAL_ERROR;
    }

#if MOTION_SENSOR_USE_SOFT_I2C
    return motion_sensor_soft_i2c_mem_read(address, reg, buffer, len);
#else
    return HAL_I2C_Mem_Read(&s_motion_i2c,
                            (address << 1U),
                            reg,
                            I2C_MEMADD_SIZE_8BIT,
                            buffer,
                            len,
                            MOTION_SENSOR_I2C_TIMEOUT_MS);
#endif
}

static HAL_StatusTypeDef motion_sensor_write_reg(uint8_t reg, uint8_t value)
{
    return motion_sensor_write_reg_addr(s_i2c_address, reg, value);
}

static HAL_StatusTypeDef motion_sensor_read_regs(uint8_t reg, uint8_t *buffer, uint16_t len)
{
    return motion_sensor_read_regs_addr(s_i2c_address, reg, buffer, len);
}

static HAL_StatusTypeDef motion_sensor_device_config(uint8_t address)
{
    HAL_StatusTypeDef status;
    uint8_t who = 0U;

    status = motion_sensor_read_regs_addr(address, REG_WHO_AM_I, &who, 1U);
    if (status != HAL_OK || who != ICM42688_WHO_AM_I_VALUE)
    {
        return HAL_ERROR;
    }

    status = motion_sensor_write_reg_addr(address, REG_DEVICE_CONFIG, 0x01U);
    if (status != HAL_OK)
    {
        return status;
    }
    HAL_Delay(2U);

    status = motion_sensor_read_regs_addr(address, REG_WHO_AM_I, &who, 1U);
    if (status != HAL_OK || who != ICM42688_WHO_AM_I_VALUE)
    {
        return HAL_ERROR;
    }

    status = motion_sensor_write_reg_addr(address, REG_PWR_MGMT0, 0x0FU);
    if (status != HAL_OK)
    {
        return status;
    }
    HAL_Delay(2U);

    (void)motion_sensor_write_reg_addr(address, REG_ACCEL_CONFIG0, 0x48U);
    (void)motion_sensor_write_reg_addr(address, REG_GYRO_CONFIG0, 0x48U);

    s_i2c_address = address;

    return HAL_OK;
}

static HAL_StatusTypeDef motion_sensor_read_sample(int16_t sample[3])
{
    uint8_t raw[6];
    HAL_StatusTypeDef status = motion_sensor_read_regs(REG_ACCEL_DATA_X1, raw, sizeof(raw));
    if (status != HAL_OK)
    {
        return status;
    }

    sample[0] = (int16_t)((raw[0] << 8) | raw[1]);
    sample[1] = (int16_t)((raw[2] << 8) | raw[3]);
    sample[2] = (int16_t)((raw[4] << 8) | raw[5]);
    return HAL_OK;
}

static int32_t motion_sensor_calculate_norm_mg(const int16_t sample[3])
{
    int64_t sum = 0;

    for (uint8_t i = 0U; i < 3U; i++)
    {
        int32_t mg = (int32_t)((sample[i] * 1000) / MOTION_SENSOR_ACCEL_SENSITIVITY_LSB_PER_G);
        sum += (int64_t)mg * (int64_t)mg;
    }

    float norm = sqrtf((float)sum);
    if (norm > (float)INT32_MAX)
    {
        norm = (float)INT32_MAX;
    }
    return (int32_t)norm;
}

static void motion_sensor_adapt_baseline(int32_t norm_mg)
{
    int32_t diff = norm_mg - s_norm_baseline_mg;
#if MOTION_SENSOR_BASELINE_FILTER_COEFF == 0
    (void)diff;
    s_norm_baseline_mg = norm_mg;
#else
    s_norm_baseline_mg += diff / MOTION_SENSOR_BASELINE_FILTER_COEFF;
#endif
}

void motion_sensor_init(void)
{
    if (s_initialized)
    {
        return;
    }

    motion_sensor_gpio_init();
    motion_sensor_i2c_init();

    s_device_available = 0U;
    if (s_i2c_ready)
    {
        if (motion_sensor_device_config(MOTION_SENSOR_I2C_ADDRESS) == HAL_OK)
        {
            s_device_available = 1U;
        }
        else
        {
            uint8_t fallback_address = (MOTION_SENSOR_I2C_ADDRESS == 0x69U) ? 0x68U : 0x69U;
            if (fallback_address != MOTION_SENSOR_I2C_ADDRESS)
            {
                if (motion_sensor_device_config(fallback_address) == HAL_OK)
                {
                    s_device_available = 1U;
                }
            }
        }
    }

    s_sample_valid = 0U;
    s_norm_baseline_mg = MOTION_SENSOR_GRAVITY_MG;
    s_motion_state = 0U;
    s_motion_confirm_count = 0U;
    s_static_confirm_count = 0U;
    memset(s_last_sample, 0, sizeof(s_last_sample));
    s_last_sample_valid = 0U;
    s_last_valid_norm_mg = MOTION_SENSOR_GRAVITY_MG;
    memset(s_prev_sample, 0, sizeof(s_prev_sample));
    s_prev_sample_valid = 0U;
    s_last_motion_tick = HAL_GetTick();
    s_last_motion_valid = 0U;

    motion_sensor_set_sensitivity_level(3);

    s_initialized = 1U;
}

uint8_t motion_sensor_is_moving(void)
{
    int16_t sample[3] = {0};
    int32_t norm_mg = MOTION_SENSOR_GRAVITY_MG;
    uint32_t delta_mg;
    uint8_t sample_valid = 0U;
    HAL_StatusTypeDef status;

    if (!s_initialized)
    {
        motion_sensor_init();
    }

    if (!s_enabled)
    {
        return 0U;
    }

    if (!s_device_available)
    {
        s_last_motion_tick = HAL_GetTick();
        s_last_motion_valid = 1U;
        return 1U;
    }

    status = motion_sensor_read_sample(sample);
    if (status == HAL_OK)
    {
        int32_t candidate_norm = motion_sensor_calculate_norm_mg(sample);
        if (candidate_norm >= MOTION_SENSOR_MIN_VALID_NORM_MG &&
            candidate_norm <= MOTION_SENSOR_MAX_VALID_NORM_MG)
        {
            norm_mg = candidate_norm;
            memcpy(s_last_sample, sample, sizeof(sample));
            s_last_sample_valid = 1U;
            s_last_valid_norm_mg = norm_mg;
            sample_valid = 1U;
        }
    }

    if (!sample_valid)
    {
        if (s_last_sample_valid)
        {
            norm_mg = s_last_valid_norm_mg;
        }
        else
        {
            s_last_motion_tick = HAL_GetTick();
            s_last_motion_valid = 1U;
            s_motion_state = 1U;
            s_sample_valid = 0U;
            return 1U;
        }
    }

    if (!s_sample_valid)
    {
        s_norm_baseline_mg = norm_mg;
        s_sample_valid = 1U;
        s_motion_state = 1U;
        s_motion_confirm_count = 0U;
        s_static_confirm_count = 0U;
        s_last_motion_tick = HAL_GetTick();
        s_last_motion_valid = 1U;
        return 1U;
    }

    if (norm_mg < 0)
    {
        norm_mg = 0;
    }

    {
        int32_t diff = norm_mg - s_norm_baseline_mg;
        if (diff < 0)
        {
            diff = -diff;
        }
        delta_mg = (uint32_t)diff;
    }

    uint32_t accel_change_mg = 0U;
    if (s_prev_sample_valid)
    {
        int64_t change_sum = 0;
        for (uint8_t i = 0U; i < 3U; i++)
        {
            int32_t prev_mg = (int32_t)((s_prev_sample[i] * 1000) / MOTION_SENSOR_ACCEL_SENSITIVITY_LSB_PER_G);
            int32_t curr_mg = (int32_t)((sample[i] * 1000) / MOTION_SENSOR_ACCEL_SENSITIVITY_LSB_PER_G);
            int32_t diff = curr_mg - prev_mg;
            if (diff < 0) diff = -diff;
            change_sum += (int64_t)diff * (int64_t)diff;
        }
        float change_norm = sqrtf((float)change_sum);
        if (change_norm > (float)UINT32_MAX)
        {
            change_norm = (float)UINT32_MAX;
        }
        accel_change_mg = (uint32_t)change_norm;
    }

    if (sample_valid)
    {
        memcpy(s_prev_sample, sample, sizeof(sample));
        s_prev_sample_valid = 1U;
    }

    uint32_t moving_threshold = motion_sensor_get_moving_threshold();
    uint32_t static_threshold = motion_sensor_get_static_threshold();
    uint8_t moving_debounce = motion_sensor_get_moving_debounce();
    uint8_t static_debounce = motion_sensor_get_static_debounce();

    if (delta_mg <= static_threshold &&
        delta_mg <= MOTION_SENSOR_BASELINE_TRACK_THRESHOLD_MG &&
        accel_change_mg <= static_threshold)
    {
        motion_sensor_adapt_baseline(norm_mg);
    }

    uint8_t motion_detected = 0U;
    
    if (delta_mg >= moving_threshold)
    {
        motion_detected = 1U;
    }
    else if (accel_change_mg >= moving_threshold && 
             delta_mg >= (moving_threshold * 2U / 3U))
    {
        motion_detected = 1U;
    }
    else if (accel_change_mg >= (moving_threshold * 2U) &&
             delta_mg >= static_threshold)
    {
        motion_detected = 1U;
    }
    else if (s_sensitivity_level >= 4U)
    {
        if (delta_mg >= (moving_threshold * 3U / 5U) ||
            accel_change_mg >= (moving_threshold * 3U / 5U))
        {
            motion_detected = 1U;
        }
    }

    if (motion_detected)
    {
        if (s_motion_confirm_count < 0xFFU)
        {
            s_motion_confirm_count++;
        }
        s_static_confirm_count = 0U;
    }
    else if (delta_mg <= static_threshold && 
             accel_change_mg <= static_threshold)
    {
        if (s_static_confirm_count < 0xFFU)
        {
            s_static_confirm_count++;
        }
        s_motion_confirm_count = 0U;
    }
    else
    {
        if (s_motion_state)
        {
            if (s_static_confirm_count < 0xFFU)
            {
                s_static_confirm_count++;
            }
        }
        else
        {
            if (s_motion_confirm_count > 0U)
            {
                s_motion_confirm_count--;
            }
        }
    }

    if (!s_motion_state && (s_motion_confirm_count >= moving_debounce))
    {
        s_motion_state = 1U;
        s_motion_confirm_count = 0U;
    }

    if (s_motion_state && (s_static_confirm_count >= static_debounce))
    {
        s_motion_state = 0U;
        s_static_confirm_count = 0U;
    }

    if (s_motion_state)
    {
        s_last_motion_tick = HAL_GetTick();
        s_last_motion_valid = 1U;
        return 1U;
    }

    return 0U;
}

void motion_sensor_enable(void)
{
    if (!s_initialized)
    {
        motion_sensor_init();
    }

    s_enabled = 1U;
    s_sample_valid = 0U;
    s_motion_state = 0U;
    s_norm_baseline_mg = MOTION_SENSOR_GRAVITY_MG;
    s_motion_confirm_count = 0U;
    s_static_confirm_count = 0U;
    memset(s_last_sample, 0, sizeof(s_last_sample));
    s_last_sample_valid = 0U;
    s_last_valid_norm_mg = MOTION_SENSOR_GRAVITY_MG;
    memset(s_prev_sample, 0, sizeof(s_prev_sample));
    s_prev_sample_valid = 0U;
    s_last_motion_tick = HAL_GetTick();
    s_last_motion_valid = 1U;
}

void motion_sensor_disable(void)
{
    s_enabled = 0U;
    s_sample_valid = 0U;
    s_motion_state = 0U;
    s_motion_confirm_count = 0U;
    s_static_confirm_count = 0U;
    s_norm_baseline_mg = MOTION_SENSOR_GRAVITY_MG;
    memset(s_last_sample, 0, sizeof(s_last_sample));
    s_last_sample_valid = 0U;
    s_last_valid_norm_mg = MOTION_SENSOR_GRAVITY_MG;
    memset(s_prev_sample, 0, sizeof(s_prev_sample));
    s_prev_sample_valid = 0U;
    s_last_motion_valid = 0U;
}

void motion_sensor_reset_static_timer(void)
{
    s_last_motion_tick = HAL_GetTick();
    s_last_motion_valid = 1U;
}

uint32_t motion_sensor_get_static_time(void)
{
    if (!s_enabled || !s_last_motion_valid)
    {
        return 0U;
    }

    uint32_t now = HAL_GetTick();
    return now - s_last_motion_tick;
}

void motion_sensor_set_sensitivity_level(uint8_t level)
{
    if (level < MOTION_SENSOR_SENSITIVITY_LEVEL_MIN)
    {
        level = MOTION_SENSOR_SENSITIVITY_LEVEL_MIN;
    }
    else if (level > MOTION_SENSOR_SENSITIVITY_LEVEL_MAX)
    {
        level = MOTION_SENSOR_SENSITIVITY_LEVEL_MAX;
    }
    
    s_sensitivity_level = level;
    
    s_motion_confirm_count = 0U;
    s_static_confirm_count = 0U;
}

uint8_t motion_sensor_get_sensitivity_level(void)
{
    return s_sensitivity_level;
}
