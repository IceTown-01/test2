#include "lcd.h"
#include "stm32h7xx_hal_spi.h"
#include "stm32h7xx_hal.h"

SPI_HandleTypeDef hspi1;

/* 直接寄存器发送，绕过HAL等待路径；返回0表示超时失败 */
static uint8_t SPI1_TX_Blocking(const uint8_t *pdata, uint16_t size, uint32_t timeout_ms)
{
  uint32_t start = HAL_GetTick();
  SPI_TypeDef *SPIx = hspi1.Instance;

  if (size == 0) return 1;

  /* 等待SPI就绪（确保上一次传输完成） */
  /* 检查是否有正在进行的传输，等待EOT标志或TXP标志 */
  /* 如果TXP为0且EOT为0，说明可能有传输正在进行，需要等待 */
  uint32_t wait_start = HAL_GetTick();
  while ((SPIx->SR & (SPI_SR_EOT | SPI_SR_TXP)) == 0U) {
    /* 如果EOT和TXP都为0，可能正在传输中，等待 */
    if ((HAL_GetTick() - wait_start) > 10U) {
      /* 超时10ms，假设SPI空闲或可以继续，清除可能的状态 */
      break;
    }
  }
  /* 清除所有标志，准备新的传输 */
  SPIx->IFCR = 0xFFFFFFFFU;

  /* 切换到1-line TX模式（半双工发送模式） */
  /* 注意：必须在设置TSIZE之前设置方向 */
  SPI_1LINE_TX(&hspi1);

  /* 确保SPI已禁用，因为TSIZE只能在SPI禁用时修改 */
  if ((SPIx->CR1 & SPI_CR1_SPE) != 0) {
    CLEAR_BIT(SPIx->CR1, SPI_CR1_SPE);
    /* 等待SPI真正禁用（需要几个时钟周期） */
    while ((SPIx->CR1 & SPI_CR1_SPE) != 0) {
      __NOP();
    }
  }

  /* 配置发送字节数TSIZE（必须在SPI禁用时设置） */
  MODIFY_REG(SPIx->CR2, SPI_CR2_TSIZE, size);

  /* 使能SPI外设 */
  SET_BIT(SPIx->CR1, SPI_CR1_SPE);
  
  /* 短暂延时，确保SPI使能生效 */
  __NOP(); __NOP(); __NOP(); __NOP();

  /* 启动传输（主模式必须设置CSTART） */
  if (hspi1.Init.Mode == SPI_MODE_MASTER) {
    SET_BIT(SPIx->CR1, SPI_CR1_CSTART);
  }

  uint16_t idx = 0;
  while (idx < size)
  {
    /* 等待TXP为1可写入 */
    uint32_t txp_start = HAL_GetTick();
    while ((SPIx->SR & SPI_SR_TXP) == 0U)
    {
      if ((HAL_GetTick() - start) > timeout_ms) {
        /* TXP超时，传输失败 */
        return 0;
      }
      /* 防止死循环，检查是否卡住 */
      if ((HAL_GetTick() - txp_start) > 100U) {
        /* TXP标志超过100ms未设置，可能是SPI配置问题 */
        return 0;
      }
    }
    /* 写入一个字节到TXDR */
    *((__IO uint8_t *)&SPIx->TXDR) = pdata[idx++];
    /* 若有接收标志则读出抛弃，避免状态机堵塞 */
#ifdef SPI_SR_RXP
    if ((SPIx->SR & SPI_SR_RXP) != 0U) {
      volatile uint8_t dummy = *((__IO uint8_t *)&SPIx->RXDR);
      (void)dummy;
    }
#endif
  }

  /* 等待传输结束：等待EOT标志被设置（SET状态） */
  /* EOT在传输开始时为RESET（0），传输完成后为SET（1） */
  uint32_t eot_wait_start = HAL_GetTick();
#ifdef SPI_SR_TXC
  while (((SPIx->SR & SPI_SR_EOT) == 0U) && ((SPIx->SR & SPI_SR_TXC) == 0U))
#else
  while ((SPIx->SR & SPI_SR_EOT) == 0U)
#endif
  {
    if ((HAL_GetTick() - start) > timeout_ms) {
      /* 总超时 */
      return 0;
    }
    /* 防止死循环，检查是否卡住 */
    if ((HAL_GetTick() - eot_wait_start) > 100U) {
      /* EOT标志超过100ms未设置，可能是SPI配置或数据未发送完成 */
      /* 检查是否有错误标志 */
      if ((SPIx->SR & (SPI_FLAG_UDR | SPI_FLAG_FRE | SPI_FLAG_OVR)) != 0U) {
        /* 有错误标志，传输失败 */
        return 0;
      }
    }
  }
  /* 若定义了BSY则等待其清零 */
#ifdef SPI_SR_BSY
  while ((SPIx->SR & SPI_SR_BSY) != 0U) {
    if ((HAL_GetTick() - start) > timeout_ms) return 0;
  }
#endif
  /* 清EOT/(可选)TXC 标志；若超时未能达成则软复位SPI一次 */
#ifdef SPI_IFCR_TXC
  SPIx->IFCR = SPI_IFCR_EOTC | SPI_IFCR_TXC;
#else
  SPIx->IFCR = SPI_IFCR_EOTC;
#endif
  return 1;
}

/* 超时则切换到模式3重试一次 */
static uint8_t SPI1_TX_WithFallback(const uint8_t *pdata, uint16_t size)
{
  uint32_t timeout_ms;
  
  // 根据数据量动态调整超时时间（大块数据需要更长时间）
  if (size > 50000) {
    timeout_ms = 1000;  // 大块数据使用1秒超时
  } else if (size > 20000) {
    timeout_ms = 500;   // 中等数据块使用500ms超时
  } else {
    timeout_ms = 100;   // 小块数据使用100ms超时
  }
  
  if (SPI1_TX_Blocking(pdata, size, timeout_ms)) return 1;

  /* 切到 CPOL=1/CPHA=2EDGE 再试一次 */
  HAL_SPI_DeInit(&hspi1);
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK) {
    return 0;
  }
  __HAL_SPI_ENABLE(&hspi1);
  return SPI1_TX_Blocking(pdata, size, timeout_ms);
}


/**
  * @brief 初始化JD9613显示屏所需的GPIO
  * @retval None
  */
static void JD9613_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* 1. 使能GPIO时钟 */
  __HAL_RCC_GPIOA_CLK_ENABLE(); // 使能GPIOA时钟 (SCK, MOSI)
  __HAL_RCC_GPIOB_CLK_ENABLE(); // 使能GPIOB时钟 (CS, DC, RST)

  /* 2. 配置SPI引脚: SCK/MOSI */
  GPIO_InitStruct.Pin = LCD_SCK_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = LCD_SPI_AF;
  HAL_GPIO_Init(LCD_SCK_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LCD_MOSI_PIN;
  HAL_GPIO_Init(LCD_MOSI_PORT, &GPIO_InitStruct);

  /* 为防止H7 2线模式下TXP不置位，默认也将 MISO 配置为AF5（PA6） */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* 3. 配置控制引脚: PB0 (CS), PB1 (DC), PB2 (RST) 为推挽输出模式 */
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;   // 推挽输出
  GPIO_InitStruct.Pull = GPIO_NOPULL;           // 无上下拉
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;  // 高速
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* 4. 初始化后，将控制引脚设置为默认电平 */
  // 拉高片选(CS)，不选中显示屏
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
  // 数据/命令(DC)引脚初始电平可根据需要设置，通常先设为命令模式
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
  // 拉高复位(RST)，结束复位状态
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
}

/**
  * @brief  SPI底层初始化函数（HAL库回调函数）
  * @param  hspi: SPI句柄指针
  * @note   此函数会被HAL_SPI_Init()调用，用于配置SPI的GPIO和时钟
  * @retval None
  */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI1)
  {
    /* 使能SPI1时钟 */
    __HAL_RCC_SPI1_CLK_ENABLE();
    
    /* GPIO已经在JD9613_GPIO_Init()中配置，这里只需要确保时钟已使能 */
    /* 如果需要重新配置GPIO，可以在这里配置，但当前已在JD9613_GPIO_Init()中完成 */
  }
}

/**
  * @brief  SPI底层去初始化函数（HAL库回调函数）
  * @param  hspi: SPI句柄指针
  * @retval None
  */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI1)
  {
    /* 禁用SPI1时钟 */
    __HAL_RCC_SPI1_CLK_DISABLE();
  }
}

/**
  * @brief  向LCD发送命令
  * @param  cmd: 要发送的命令
  * @retval None
  */
void LCD_WriteCommand(uint8_t cmd)
{
    // 设置DC线为低电平，表示发送的是命令
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_RESET);
    // 拉低片选，选中LCD
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);

    // 发送命令
    (void)SPI1_TX_WithFallback(&cmd, 1);

    // 拉高片选，结束传输
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

/**
  * @brief  向LCD发送一个字节的数据
  * @param  data: 要发送的数据
  * @retval None
  */
void LCD_WriteData(uint8_t data)
{
    // 设置DC线为高电平，表示发送的是数据
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    // 拉低片选，选中LCD
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);

    // 发送数据
    (void)SPI1_TX_WithFallback(&data, 1);

    // 拉高片选，结束传输
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

/**
  * @brief  向LCD发送16位数据（用于RGB565颜色等）
  * @param  data: 要发送的16位数据
  * @retval None
  */
void LCD_WriteData16(uint16_t data)
{
    uint8_t data_buffer[2];
    // 将16位数据拆分为两个8位数据。根据设备要求选择字节顺序。
    // 常见顺序：先高8位，后低8位。
    data_buffer[0] = (data >> 8) & 0xFF; // 高字节
    data_buffer[1] = data & 0xFF;        // 低字节

    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);

    (void)SPI1_TX_WithFallback(data_buffer, 2);

    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

/**
  * @brief  LCD硬件复位
  * @retval None
  */
void LCD_Reset(void)
{
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET); // 拉低复位
    HAL_Delay(120);  // 延时120ms
    HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);   // 拉高复位
    HAL_Delay(120);  // 再延时120ms
}

/**
  * @brief  LCD初始化
  * @retval None
  * @note   初始化序列需要根据JD9613数据手册填写
  */
void LCD_Init(void)
{

  JD9613_GPIO_Init();

    // 先开启SPI1外设时钟
    __HAL_RCC_SPI1_CLK_ENABLE();

    // SPI1初始化参数示例 (在CubeMX或代码中设置)
    hspi1.Instance = LCD_SPI_INSTANCE;
    hspi1.Init.Mode = SPI_MODE_MASTER;          // 主模式
    hspi1.Init.Direction = SPI_DIRECTION_1LINE; // 仅发送，避免RX路径影响传输完成标志
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;     // 数据帧8位
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;   // 时钟极性CPOL=0 (模式0)
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;       // 时钟相位CPHA=0 (模式0)[2,4](@ref)
    hspi1.Init.NSS = SPI_NSS_SOFT;              // 软件控制NSS（片选由PB0控制）
    hspi1.Init.BaudRatePrescaler = LCD_SPI_PRESCALER; // SPI时钟频率配置
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;      // 高位先行
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    /* H7 SPIv2 建议项：关闭NSS脉冲，设置FIFO阈值，保持IO（按可用宏与字段保护） */
    hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
#ifdef SPI_FIFO_THRESHOLD_01DATA
    hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
#endif
#ifdef SPI_MASTER_KEEP_IO_STATE_ENABLE
    hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
#endif
#ifdef SPI_IOSWAP_DISABLE
    hspi1.Init.IOSwap = SPI_IOSWAP_DISABLE;
#endif
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        //
    }
    // 显式使能SPI，确保外设开启
    __HAL_SPI_ENABLE(&hspi1);

    // 硬件复位
    LCD_Reset();

    // JD9613初始化序列
// 进入扩展命令模式
LCD_WriteCommand(0xFE); 
LCD_WriteData(0x01);

// 密码验证命令
LCD_WriteCommand(0xF7); 
LCD_WriteData(0x96);    // 密码字节1
LCD_WriteData(0x13);    // 密码字节2  
LCD_WriteData(0xA9);    // 密码字节3

// 关闭MIPI接口
LCD_WriteCommand(0x90); // mipi off
LCD_WriteData(0x01);

// 电源配置命令1
LCD_WriteCommand(0x2C); 
LCD_WriteData(0x19);
LCD_WriteData(0x0B);
LCD_WriteData(0x24);
LCD_WriteData(0x1B);
LCD_WriteData(0x1B);
LCD_WriteData(0x1B);
LCD_WriteData(0xAA);
LCD_WriteData(0x50);
LCD_WriteData(0x01);
LCD_WriteData(0x16);
LCD_WriteData(0x04);
LCD_WriteData(0x04);
LCD_WriteData(0x04);
LCD_WriteData(0xD7);

// 电源配置命令2
LCD_WriteCommand(0x2D); 
LCD_WriteData(0x66);
LCD_WriteData(0x56);
LCD_WriteData(0x55);

// 电源配置命令3
LCD_WriteCommand(0x2E); 
LCD_WriteData(0x24);
LCD_WriteData(0x04);
LCD_WriteData(0x3F);
LCD_WriteData(0x30);
LCD_WriteData(0x30);
LCD_WriteData(0xA8);
LCD_WriteData(0xB8);
LCD_WriteData(0xB8);
LCD_WriteData(0x07);

// Gamma设置命令
LCD_WriteCommand(0x33); 
LCD_WriteData(0x03);
LCD_WriteData(0x03);
LCD_WriteData(0x03);
LCD_WriteData(0x19);
LCD_WriteData(0x19);
LCD_WriteData(0x19);
LCD_WriteData(0x13);
LCD_WriteData(0x13);
LCD_WriteData(0x13);
LCD_WriteData(0x1A);
LCD_WriteData(0x1A);
LCD_WriteData(0x1A);

// 电源时序设置
LCD_WriteCommand(0x10); 
LCD_WriteData(0x0B);
LCD_WriteData(0x08);
LCD_WriteData(0x64);
LCD_WriteData(0xAE);
LCD_WriteData(0x0B);
LCD_WriteData(0x08);
LCD_WriteData(0x64);
LCD_WriteData(0xAE);
LCD_WriteData(0x00);
LCD_WriteData(0x80);
LCD_WriteData(0x00);
LCD_WriteData(0x00);
LCD_WriteData(0x01);

// 电源控制命令
LCD_WriteCommand(0x11); 
LCD_WriteData(0x01);
LCD_WriteData(0x1E);
LCD_WriteData(0x01);
LCD_WriteData(0x1E);
LCD_WriteData(0x00);

// 胶合逻辑配置
LCD_WriteCommand(0x03); //r/ Glue
LCD_WriteData(0x93);
LCD_WriteData(0x1C);
LCD_WriteData(0x00);
LCD_WriteData(0x01);
LCD_WriteData(0x7E);

// 系统配置
LCD_WriteCommand(0x19); // system 
LCD_WriteData(0x00);

// 时序控制命令1
LCD_WriteCommand(0x31); 
LCD_WriteData(0x1B);
LCD_WriteData(0x00);
LCD_WriteData(0x06);
LCD_WriteData(0x05);
LCD_WriteData(0x05);
LCD_WriteData(0x05);

// 面板驱动配置
LCD_WriteCommand(0x35); 
LCD_WriteData(0x00);
LCD_WriteData(0x80);
LCD_WriteData(0x80);
LCD_WriteData(0x00);

// 显示延迟设置
LCD_WriteCommand(0x12); 
LCD_WriteData(0x1B); //t_ld_dly

// 面板配置命令
LCD_WriteCommand(0x1A); 
LCD_WriteData(0x01);
LCD_WriteData(0x20);
LCD_WriteData(0x00);
LCD_WriteData(0x08);
LCD_WriteData(0x01);
LCD_WriteData(0x06);
LCD_WriteData(0x06);
LCD_WriteData(0x06);

// 源极驱动配置1
LCD_WriteCommand(0x74); 
LCD_WriteData(0xBD);
LCD_WriteData(0x00);
LCD_WriteData(0x01);
LCD_WriteData(0x08);
LCD_WriteData(0x01);
LCD_WriteData(0xBB);
LCD_WriteData(0x98);

// 源极驱动配置2
LCD_WriteCommand(0x6C); 
LCD_WriteData(0xDC);
LCD_WriteData(0x08);
LCD_WriteData(0x02);
LCD_WriteData(0x01);
LCD_WriteData(0x08);
LCD_WriteData(0x01);
LCD_WriteData(0x30);
LCD_WriteData(0x08);
LCD_WriteData(0x00);

// 源极驱动配置3
LCD_WriteCommand(0x6D); 
LCD_WriteData(0xDC);  
LCD_WriteData(0x08);  
LCD_WriteData(0x02);  
LCD_WriteData(0x01);  
LCD_WriteData(0x08);  
LCD_WriteData(0x02);  
LCD_WriteData(0x30);  
LCD_WriteData(0x08);  
LCD_WriteData(0x00);  

// 源极驱动配置4
LCD_WriteCommand(0x76); 
LCD_WriteData(0xDA);
LCD_WriteData(0x00);
LCD_WriteData(0x02);
LCD_WriteData(0x20);
LCD_WriteData(0x39);
LCD_WriteData(0x80);
LCD_WriteData(0x80);
LCD_WriteData(0x50);
LCD_WriteData(0x05);

// 源极驱动配置5
LCD_WriteCommand(0x6E);
LCD_WriteData(0xDC);
LCD_WriteData(0x00);
LCD_WriteData(0x02);
LCD_WriteData(0x01);
LCD_WriteData(0x00);
LCD_WriteData(0x02);
LCD_WriteData(0x4F);
LCD_WriteData(0x02);
LCD_WriteData(0x00);

// 源极驱动配置6
LCD_WriteCommand(0x6F);
LCD_WriteData(0xDC);
LCD_WriteData(0x00);
LCD_WriteData(0x02);
LCD_WriteData(0x01);
LCD_WriteData(0x00);
LCD_WriteData(0x01);
LCD_WriteData(0x4F);
LCD_WriteData(0x02);
LCD_WriteData(0x00);

// 源极驱动配置7
LCD_WriteCommand(0x80); 
LCD_WriteData(0xBD);
LCD_WriteData(0x00);
LCD_WriteData(0x01);
LCD_WriteData(0x08);
LCD_WriteData(0x01);
LCD_WriteData(0xBB);
LCD_WriteData(0x98);

// 源极驱动配置8
LCD_WriteCommand(0x78);
LCD_WriteData(0xDC);
LCD_WriteData(0x08);
LCD_WriteData(0x02);
LCD_WriteData(0x01);
LCD_WriteData(0x08);
LCD_WriteData(0x01);
LCD_WriteData(0x30);
LCD_WriteData(0x08);
LCD_WriteData(0x00);

// 源极驱动配置9
LCD_WriteCommand(0x79);
LCD_WriteData(0xDC);
LCD_WriteData(0x08);
LCD_WriteData(0x02);
LCD_WriteData(0x01);
LCD_WriteData(0x08);
LCD_WriteData(0x02);
LCD_WriteData(0x30);
LCD_WriteData(0x08);
LCD_WriteData(0x00);

// 源极驱动配置10
LCD_WriteCommand(0x82); 
LCD_WriteData(0xDA);
LCD_WriteData(0x40);
LCD_WriteData(0x02);
LCD_WriteData(0x20);
LCD_WriteData(0x39);
LCD_WriteData(0x00);
LCD_WriteData(0x80);
LCD_WriteData(0x50);
LCD_WriteData(0x05);

// 源极驱动配置11
LCD_WriteCommand(0x7A);  
LCD_WriteData(0xDC);  
LCD_WriteData(0x00);  
LCD_WriteData(0x02);  
LCD_WriteData(0x01);  
LCD_WriteData(0x00);  
LCD_WriteData(0x02);  
LCD_WriteData(0x4F);  
LCD_WriteData(0x02);  
LCD_WriteData(0x00);  

// 源极驱动配置12
LCD_WriteCommand(0x7B); 
LCD_WriteData(0xDC); 
LCD_WriteData(0x00); 
LCD_WriteData(0x02); 
LCD_WriteData(0x01); 
LCD_WriteData(0x00); 
LCD_WriteData(0x01); 
LCD_WriteData(0x4F); 
LCD_WriteData(0x02); 
LCD_WriteData(0x00); 

// Gamma校正设置1
LCD_WriteCommand(0x84); 
LCD_WriteData(0x01);
LCD_WriteData(0x00);
LCD_WriteData(0x09);
LCD_WriteData(0x19);
LCD_WriteData(0x19);
LCD_WriteData(0x19);
LCD_WriteData(0x19);
LCD_WriteData(0x19);
LCD_WriteData(0x19);
LCD_WriteData(0x19);

// Gamma校正设置2
LCD_WriteCommand(0x85); 
LCD_WriteData(0x19);
LCD_WriteData(0x19);
LCD_WriteData(0x19);
LCD_WriteData(0x03);
LCD_WriteData(0x02);
LCD_WriteData(0x08);
LCD_WriteData(0x19);
LCD_WriteData(0x19);
LCD_WriteData(0x19);
LCD_WriteData(0x19);

// 显示配置命令1
LCD_WriteCommand(0x20); 
LCD_WriteData(0x20);
LCD_WriteData(0x00);
LCD_WriteData(0x08);
LCD_WriteData(0x00);
LCD_WriteData(0x02);
LCD_WriteData(0x00);
LCD_WriteData(0x40);
LCD_WriteData(0x00);
LCD_WriteData(0x10);
LCD_WriteData(0x00);
LCD_WriteData(0x04);
LCD_WriteData(0x00);

// 显示配置命令2
LCD_WriteCommand(0x1E); 
LCD_WriteData(0x40);
LCD_WriteData(0x00);
LCD_WriteData(0x10);
LCD_WriteData(0x00);
LCD_WriteData(0x04);
LCD_WriteData(0x00);
LCD_WriteData(0x20);
LCD_WriteData(0x00);
LCD_WriteData(0x08);
LCD_WriteData(0x00);
LCD_WriteData(0x02);
LCD_WriteData(0x00);

// 显示配置命令3
LCD_WriteCommand(0x24); 
LCD_WriteData(0x20);
LCD_WriteData(0x00);
LCD_WriteData(0x08);
LCD_WriteData(0x00);
LCD_WriteData(0x02);
LCD_WriteData(0x00);
LCD_WriteData(0x40);
LCD_WriteData(0x00);
LCD_WriteData(0x10);
LCD_WriteData(0x00);
LCD_WriteData(0x04);
LCD_WriteData(0x00);

// 显示配置命令4
LCD_WriteCommand(0x22); 
LCD_WriteData(0x40);
LCD_WriteData(0x00);
LCD_WriteData(0x10);
LCD_WriteData(0x00);
LCD_WriteData(0x04);
LCD_WriteData(0x00);
LCD_WriteData(0x20);
LCD_WriteData(0x00);
LCD_WriteData(0x08);
LCD_WriteData(0x00);
LCD_WriteData(0x02);
LCD_WriteData(0x00);

// RGB Gamma设置 - 红色分量
LCD_WriteCommand(0x13); 
LCD_WriteData(0x63);
LCD_WriteData(0x52);
LCD_WriteData(0x41);

// RGB Gamma设置 - 绿色分量
LCD_WriteCommand(0x14);
LCD_WriteData(0x36);
LCD_WriteData(0x25);
LCD_WriteData(0x14);

// RGB Gamma设置 - 蓝色分量
LCD_WriteCommand(0x15); 
LCD_WriteData(0x63);
LCD_WriteData(0x52);
LCD_WriteData(0x41);

// RGB Gamma设置 - 全部颜色
LCD_WriteCommand(0x16);
LCD_WriteData(0x36);
LCD_WriteData(0x25);
LCD_WriteData(0x14);

// 显示控制命令
LCD_WriteCommand(0x1D);
LCD_WriteData(0x10);
LCD_WriteData(0x00);
LCD_WriteData(0x00);

// 列地址设置
LCD_WriteCommand(0x2A);
LCD_WriteData(0x0D);
LCD_WriteData(0x07);

// 查找表设置1
LCD_WriteCommand(0x27);
LCD_WriteData(0x00);
LCD_WriteData(0x01);
LCD_WriteData(0x02);
LCD_WriteData(0x03);
LCD_WriteData(0x04);
LCD_WriteData(0x05);

// 查找表设置2
LCD_WriteCommand(0x28);
LCD_WriteData(0x00);
LCD_WriteData(0x01);
LCD_WriteData(0x02);
LCD_WriteData(0x03);
LCD_WriteData(0x04);
LCD_WriteData(0x05);

// 均衡器切换
LCD_WriteCommand(0x26); // switch eq
LCD_WriteData(0x01);
LCD_WriteData(0x01);

// VSR均衡器设置
LCD_WriteCommand(0x86); // VSR eq
LCD_WriteData(0x01);
LCD_WriteData(0x01);

// 进入页面2配置
LCD_WriteCommand(0xFE); // page 2
LCD_WriteData(0x02);

// 页面2特定配置
LCD_WriteCommand(0x16);
LCD_WriteData(0x81);
LCD_WriteData(0x43);
LCD_WriteData(0x23);
LCD_WriteData(0x1E);
LCD_WriteData(0x03);

// 进入页面3配置
LCD_WriteCommand(0xFE); // page 3
LCD_WriteData(0x03);

// 开启数字Gamma校正
LCD_WriteCommand(0x60); // DGC On
LCD_WriteData(0x01);

// 数字Gamma校正参数设置1
LCD_WriteCommand(0x61);
LCD_WriteData(0x00);
LCD_WriteData(0x00);
LCD_WriteData(0x00);
LCD_WriteData(0x00);
LCD_WriteData(0x11);
LCD_WriteData(0x00);
LCD_WriteData(0x0D);
LCD_WriteData(0x26);
LCD_WriteData(0x5A);
LCD_WriteData(0x80);
LCD_WriteData(0x80);
LCD_WriteData(0x95);
LCD_WriteData(0xF8);
LCD_WriteData(0x3B);
LCD_WriteData(0x75);

// 数字Gamma校正参数设置2
LCD_WriteCommand(0x62);
LCD_WriteData(0x21);
LCD_WriteData(0x22);
LCD_WriteData(0x32);
LCD_WriteData(0x43);
LCD_WriteData(0x44);
LCD_WriteData(0xD7);
LCD_WriteData(0x0A);
LCD_WriteData(0x59);
LCD_WriteData(0xA1);
LCD_WriteData(0xE1);
LCD_WriteData(0x52);
LCD_WriteData(0xB7);
LCD_WriteData(0x11);
LCD_WriteData(0x64);
LCD_WriteData(0xB1);

// 数字Gamma校正参数设置3
LCD_WriteCommand(0x63);
LCD_WriteData(0x54);
LCD_WriteData(0x55);
LCD_WriteData(0x66);
LCD_WriteData(0x06);
LCD_WriteData(0xFB);
LCD_WriteData(0x3F);
LCD_WriteData(0x81);
LCD_WriteData(0xC6);
LCD_WriteData(0x06);
LCD_WriteData(0x45);
LCD_WriteData(0x83);

// 数字Gamma校正参数设置4
LCD_WriteCommand(0x64);
LCD_WriteData(0x00);
LCD_WriteData(0x00);
LCD_WriteData(0x11);
LCD_WriteData(0x11);
LCD_WriteData(0x21);
LCD_WriteData(0x00);
LCD_WriteData(0x23);
LCD_WriteData(0x6A);
LCD_WriteData(0xF8);
LCD_WriteData(0x63);
LCD_WriteData(0x67);
LCD_WriteData(0x70);
LCD_WriteData(0xA5);
LCD_WriteData(0xDC);
LCD_WriteData(0x02);

// 数字Gamma校正参数设置5
LCD_WriteCommand(0x65);
LCD_WriteData(0x22);
LCD_WriteData(0x22);
LCD_WriteData(0x32);
LCD_WriteData(0x43);
LCD_WriteData(0x44);
LCD_WriteData(0x24);
LCD_WriteData(0x44);
LCD_WriteData(0x82);
LCD_WriteData(0xC1);
LCD_WriteData(0xF8);
LCD_WriteData(0x61);
LCD_WriteData(0xBF);
LCD_WriteData(0x13);
LCD_WriteData(0x62);
LCD_WriteData(0xAD);

// 数字Gamma校正参数设置6
LCD_WriteCommand(0x66);
LCD_WriteData(0x54);
LCD_WriteData(0x55);
LCD_WriteData(0x65);
LCD_WriteData(0x06);
LCD_WriteData(0xF5);
LCD_WriteData(0x37);
LCD_WriteData(0x76);
LCD_WriteData(0xB8);
LCD_WriteData(0xF5);
LCD_WriteData(0x31);
LCD_WriteData(0x6C);

// 数字Gamma校正参数设置7
LCD_WriteCommand(0x67);
LCD_WriteData(0x00);
LCD_WriteData(0x10);
LCD_WriteData(0x22);
LCD_WriteData(0x22);
LCD_WriteData(0x22);
LCD_WriteData(0x00);
LCD_WriteData(0x37);
LCD_WriteData(0xA4);
LCD_WriteData(0x7E);
LCD_WriteData(0x22);
LCD_WriteData(0x25);
LCD_WriteData(0x2C);
LCD_WriteData(0x4C);
LCD_WriteData(0x72);
LCD_WriteData(0x9A);

// 数字Gamma校正参数设置8
LCD_WriteCommand(0x68);
LCD_WriteData(0x22);
LCD_WriteData(0x33);
LCD_WriteData(0x43);
LCD_WriteData(0x44);
LCD_WriteData(0x55);
LCD_WriteData(0xC1);
LCD_WriteData(0xE5);
LCD_WriteData(0x2D);
LCD_WriteData(0x6F);
LCD_WriteData(0xAF);
LCD_WriteData(0x23);
LCD_WriteData(0x8F);
LCD_WriteData(0xF3);
LCD_WriteData(0x50);
LCD_WriteData(0xA6);

// 页面3 Gamma参数设置9（续）
LCD_WriteCommand(0x69);
LCD_WriteData(0x65);
LCD_WriteData(0x66);
LCD_WriteData(0x77);
LCD_WriteData(0x07);
LCD_WriteData(0xFD);
LCD_WriteData(0x4E);
LCD_WriteData(0x9C);
LCD_WriteData(0xED);
LCD_WriteData(0x39);
LCD_WriteData(0x86);
LCD_WriteData(0xD3);

// 进入页面5配置
LCD_WriteCommand(0xFE);
LCD_WriteData(0x05);

// 页面5 Gamma参数设置1
LCD_WriteCommand(0x61);
LCD_WriteData(0x00);
LCD_WriteData(0x31);
LCD_WriteData(0x44);
LCD_WriteData(0x54);
LCD_WriteData(0x55);
LCD_WriteData(0x00);
LCD_WriteData(0x92);
LCD_WriteData(0xB5);
LCD_WriteData(0x88);
LCD_WriteData(0x19);
LCD_WriteData(0x90);
LCD_WriteData(0xE8);
LCD_WriteData(0x3E);
LCD_WriteData(0x71);
LCD_WriteData(0xA5);

// 页面5 Gamma参数设置2
LCD_WriteCommand(0x62);
LCD_WriteData(0x55);
LCD_WriteData(0x66);
LCD_WriteData(0x76);
LCD_WriteData(0x77);
LCD_WriteData(0x88);
LCD_WriteData(0xCE);
LCD_WriteData(0xF2);
LCD_WriteData(0x32);
LCD_WriteData(0x6E);
LCD_WriteData(0xC4);
LCD_WriteData(0x34);
LCD_WriteData(0x8B);
LCD_WriteData(0xD9);
LCD_WriteData(0x2A);
LCD_WriteData(0x7D);

// 页面5 Gamma参数设置3
LCD_WriteCommand(0x63);
LCD_WriteData(0x98);
LCD_WriteData(0x99);
LCD_WriteData(0xAA);
LCD_WriteData(0x0A);
LCD_WriteData(0xDC);
LCD_WriteData(0x2E);
LCD_WriteData(0x7D);
LCD_WriteData(0xC3);
LCD_WriteData(0x0D);
LCD_WriteData(0x5B);
LCD_WriteData(0x9E);

// 页面5 Gamma参数设置4
LCD_WriteCommand(0x64);
LCD_WriteData(0x00);
LCD_WriteData(0x31);
LCD_WriteData(0x44);
LCD_WriteData(0x54);
LCD_WriteData(0x55);
LCD_WriteData(0x00);
LCD_WriteData(0xA2);
LCD_WriteData(0xE5);
LCD_WriteData(0xCD);
LCD_WriteData(0x5C);
LCD_WriteData(0x94);
LCD_WriteData(0xCF);
LCD_WriteData(0x09);
LCD_WriteData(0x4A);
LCD_WriteData(0x72);

// 页面5 Gamma参数设置5
LCD_WriteCommand(0x65);
LCD_WriteData(0x55);
LCD_WriteData(0x65);
LCD_WriteData(0x66);
LCD_WriteData(0x77);
LCD_WriteData(0x87);
LCD_WriteData(0x9C);
LCD_WriteData(0xC2);
LCD_WriteData(0xFF);
LCD_WriteData(0x36);
LCD_WriteData(0x6A);
LCD_WriteData(0xEC);
LCD_WriteData(0x45);
LCD_WriteData(0x91);
LCD_WriteData(0xD8);
LCD_WriteData(0x20);

// 页面5 Gamma参数设置6
LCD_WriteCommand(0x66);
LCD_WriteData(0x88);
LCD_WriteData(0x98);
LCD_WriteData(0x99);
LCD_WriteData(0x0A);
LCD_WriteData(0x68);
LCD_WriteData(0xB0);
LCD_WriteData(0xFB);
LCD_WriteData(0x43);
LCD_WriteData(0x8C);
LCD_WriteData(0xD5);
LCD_WriteData(0x0E);

// 页面5 Gamma参数设置7
LCD_WriteCommand(0x67);
LCD_WriteData(0x00);
LCD_WriteData(0x42);
LCD_WriteData(0x55);
LCD_WriteData(0x55);
LCD_WriteData(0x55);
LCD_WriteData(0x00);
LCD_WriteData(0xCB);
LCD_WriteData(0x62);
LCD_WriteData(0xC5);
LCD_WriteData(0x09);
LCD_WriteData(0x44);
LCD_WriteData(0x72);
LCD_WriteData(0xA9);
LCD_WriteData(0xD6);
LCD_WriteData(0xFD);

// 页面5 Gamma参数设置8
LCD_WriteCommand(0x68);
LCD_WriteData(0x66);
LCD_WriteData(0x66);
LCD_WriteData(0x77);
LCD_WriteData(0x87);
LCD_WriteData(0x98);
LCD_WriteData(0x21);
LCD_WriteData(0x45);
LCD_WriteData(0x96);
LCD_WriteData(0xED);
LCD_WriteData(0x29);
LCD_WriteData(0x90);
LCD_WriteData(0xEE);
LCD_WriteData(0x4B);
LCD_WriteData(0xB1);
LCD_WriteData(0x13);

// 页面5 Gamma参数设置9
LCD_WriteCommand(0x69);
LCD_WriteData(0x99);
LCD_WriteData(0xAA);
LCD_WriteData(0xBA);
LCD_WriteData(0x0B);
LCD_WriteData(0x6A);
LCD_WriteData(0xB8);
LCD_WriteData(0x0D);
LCD_WriteData(0x62);
LCD_WriteData(0xB8);
LCD_WriteData(0x0E);
LCD_WriteData(0x54);

// 进入页面7配置
LCD_WriteCommand(0xFE); 
LCD_WriteData(0x07);

// 显示控制命令1
LCD_WriteCommand(0x3E); 
LCD_WriteData(0x00);

// 显示控制命令2
LCD_WriteCommand(0x42); 
LCD_WriteData(0x03);
LCD_WriteData(0x10);

// 显示控制命令3
LCD_WriteCommand(0x4A); 
LCD_WriteData(0x31);

// 显示控制命令4
LCD_WriteCommand(0x5C); 
LCD_WriteData(0x01);

// 显示时序配置1
LCD_WriteCommand(0x3C); 
LCD_WriteData(0x07);
LCD_WriteData(0x00);
LCD_WriteData(0x24);
LCD_WriteData(0x04);
LCD_WriteData(0x3F);
LCD_WriteData(0xE2);

// 显示时序配置2
LCD_WriteCommand(0x44);
LCD_WriteData(0x03);
LCD_WriteData(0x40);
LCD_WriteData(0x3F);
LCD_WriteData(0x02);

// Gamma曲线设置1（高位）
LCD_WriteCommand(0x12);
LCD_WriteData(0xAA);
LCD_WriteData(0xAA);
LCD_WriteData(0xC0);
LCD_WriteData(0xC8);
LCD_WriteData(0xD0);
LCD_WriteData(0xD8);
LCD_WriteData(0xE0);
LCD_WriteData(0xE8);
LCD_WriteData(0xF0);
LCD_WriteData(0xF8);

// Gamma曲线设置2（中位）
LCD_WriteCommand(0x11);
LCD_WriteData(0xAA);
LCD_WriteData(0xAA);
LCD_WriteData(0xAA);
LCD_WriteData(0x60);
LCD_WriteData(0x68);
LCD_WriteData(0x70);
LCD_WriteData(0x78);
LCD_WriteData(0x80);
LCD_WriteData(0x88);
LCD_WriteData(0x90);
LCD_WriteData(0x98);
LCD_WriteData(0xA0);
LCD_WriteData(0xA8);
LCD_WriteData(0xB0);
LCD_WriteData(0xB8);

// Gamma曲线设置3（低位）
LCD_WriteCommand(0x10);
LCD_WriteData(0xAA);
LCD_WriteData(0xAA);
LCD_WriteData(0xAA);
LCD_WriteData(0x00);
LCD_WriteData(0x08);
LCD_WriteData(0x10);
LCD_WriteData(0x18);
LCD_WriteData(0x20);
LCD_WriteData(0x28);
LCD_WriteData(0x30);
LCD_WriteData(0x38);
LCD_WriteData(0x40);
LCD_WriteData(0x48);
LCD_WriteData(0x50);
LCD_WriteData(0x58);

// Gamma查找表设置
LCD_WriteCommand(0x14);
LCD_WriteData(0x03);
LCD_WriteData(0x1F);
LCD_WriteData(0x3F);
LCD_WriteData(0x5F);
LCD_WriteData(0x7F);
LCD_WriteData(0x9F);
LCD_WriteData(0xBF);
LCD_WriteData(0xDF);
LCD_WriteData(0x03);
LCD_WriteData(0x1F);
LCD_WriteData(0x3F);
LCD_WriteData(0x5F);
LCD_WriteData(0x7F);
LCD_WriteData(0x9F);
LCD_WriteData(0xBF);
LCD_WriteData(0xDF);

// 面板驱动配置
LCD_WriteCommand(0x18);
LCD_WriteData(0x70);

// 系统配置命令
LCD_WriteCommand(0x1A);
LCD_WriteData(0x22);
LCD_WriteData(0xBB);
LCD_WriteData(0xAA);
LCD_WriteData(0xFF);
LCD_WriteData(0x24);
LCD_WriteData(0x71);
LCD_WriteData(0x0F);
LCD_WriteData(0x01);
LCD_WriteData(0x00);
LCD_WriteData(0x03);

// 返回主页面（页面0）
LCD_WriteCommand(0xFE); 
LCD_WriteData(0x00);

// 设置扫描方向，实现上下对调左右翻转
LCD_WriteCommand(0x36);
LCD_WriteData(LCD_MADCTL_INIT_VALUE);

// 设置像素格式为16位（5-6-5）RGB格式
LCD_WriteCommand(0x3A); 
LCD_WriteData(0x55);  // 16位像素格式

// 设置DSPI模式
LCD_WriteCommand(0xC4); 
LCD_WriteData(0x80);  // 启用DSPI模式

// 设置列地址（X方向显示区域）
LCD_WriteCommand(0x2A);
LCD_WriteData(0x00);  // 起始列地址高字节
LCD_WriteData(0x00);  // 起始列地址低字节
LCD_WriteData(0x00);  // 结束列地址高字节
LCD_WriteData(0x7D);  // 结束列地址低字节（125列）

// 设置页地址（Y方向显示区域）
LCD_WriteCommand(0x2B);
LCD_WriteData(0x00);  // 起始页地址高字节
LCD_WriteData(0x00);  // 起始页地址低字节
LCD_WriteData(0x01);  // 结束页地址高字节
LCD_WriteData(0x25);  // 结束页地址低字节（293行）

// 关闭撕裂效应输出
LCD_WriteCommand(0x35); 
LCD_WriteData(0x00);  // 撕裂效应关闭

// 设置背光控制
LCD_WriteCommand(0x53); 
LCD_WriteData(0x28);  // 背光控制参数

// 设置显示亮度（最大亮度）
LCD_WriteCommand(0x51); 
LCD_WriteData(0xFF);  // 亮度值（0x00-0xFF）

// 退出睡眠模式（唤醒显示器）
LCD_WriteCommand(0x11);  // SLPOUT命令
HAL_Delay(120);         // 等待120ms确保电源稳定

// 开启显示
LCD_WriteCommand(0x29);  // DISPON命令
HAL_Delay(1);           // 短暂延迟确保命令执行

}

/**
  * @brief  设置显示窗口（用于连续写入像素数据）
  * @param  x0, y0: 窗口左上角坐标
  * @param  x1, y1: 窗口右下角坐标
  * @retval None
  */
void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    // 设置列地址 (X坐标) 的命令和参数序列需要根据JD9613数据手册
    LCD_WriteCommand(0x2A); // 列地址设置命令 (假设)
    LCD_WriteData(x0 >> 8);
    LCD_WriteData(x0 & 0xFF);
    LCD_WriteData(x1 >> 8);
    LCD_WriteData(x1 & 0xFF);

    // 设置页地址 (Y坐标) 的命令和参数序列需要根据JD9613数据手册
    LCD_WriteCommand(0x2B); // 页地址设置命令 (假设)
    LCD_WriteData(y0 >> 8);
    LCD_WriteData(y0 & 0xFF);
    LCD_WriteData(y1 >> 8);
    LCD_WriteData(y1 & 0xFF);

    // 发送写入GRAM的命令
    LCD_WriteCommand(0x2C); // 写GRAM命令 (假设)
}

/**
  * @brief  用指定颜色清屏
  * @param  color: 16位的RGB565颜色值
  * @retval None
  */
void LCD_Clear(uint16_t color)
{
    uint32_t index;
    uint32_t total_pixels = LCD_WIDTH * LCD_HEIGHT;

    // 设置全屏为窗口
    LCD_SetWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

    // 发送写GRAM命令后，连续写入颜色值
    for (index = 0; index < total_pixels; index++)
    {
        LCD_WriteData16(color);
    }
}

/**
  * @brief  填充矩形区域
  * @param  x0, y0: 矩形左上角坐标
  * @param  x1, y1: 矩形右下角坐标
  * @param  color: 16位的RGB565颜色值
  * @retval None
  */
void LCD_FillRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    uint32_t index;
    uint32_t width = x1 - x0 + 1;
    uint32_t height = y1 - y0 + 1;
    uint32_t total_pixels = width * height;

    // 设置矩形区域为窗口
    LCD_SetWindow(x0, y0, x1, y1);

    // 连续写入颜色值填充矩形
    for (index = 0; index < total_pixels; index++)
    {
        LCD_WriteData16(color);
    }
}

/**
  * @brief  将屏幕分成4块，分别显示红、绿、蓝、黄
  * @retval None
  */
void LCD_ShowFourColors(void)
{
    uint16_t half_width = LCD_WIDTH / 2;
    uint16_t half_height = LCD_HEIGHT / 2;
    
    // 定义RGB565颜色值
    // 红色: 0xF800 (R=11111, G=000000, B=00000)
    // 绿色: 0x07E0 (R=00000, G=111111, B=00000)
    // 蓝色: 0x001F (R=00000, G=000000, B=11111)
    // 黄色: 0xFFE0 (R=11111, G=111111, B=00000)
    uint16_t color_red = 0xF800;
    uint16_t color_green = 0x07E0;
    uint16_t color_blue = 0x001F;
    uint16_t color_yellow = 0xFFE0;
    
    // 左上角：红色
    LCD_FillRect(0, 0, half_width - 1, half_height - 1, color_red);
    
    // 右上角：绿色
    LCD_FillRect(half_width, 0, LCD_WIDTH - 1, half_height - 1, color_green);
    
    // 左下角：蓝色
    LCD_FillRect(0, half_height, half_width - 1, LCD_HEIGHT - 1, color_blue);
    
    // 右下角：黄色
    LCD_FillRect(half_width, half_height, LCD_WIDTH - 1, LCD_HEIGHT - 1, color_yellow);
}

/**
  * @brief  优化的批量写入像素数据（用于图片显示）
  * @param  data: 指向RGB565像素数据数组的指针
  * @param  size: 要写入的像素数量（16位数据点数）
  * @retval None
  * @note   此函数比LCD_WriteData16更高效，因为只处理一次片选信号
  */
static void LCD_WritePixels_Bulk(const uint16_t *data, uint32_t size)
{
    uint32_t i;
    uint8_t data_buffer[2];
    
    if (size == 0) return;
    
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    
    // 逐个转换并发送像素数据，确保字节顺序正确
    for (i = 0; i < size; i++)
    {
        // 按照LCD_WriteData16的方式拆分字节：高字节在前
        data_buffer[0] = (data[i] >> 8) & 0xFF; // 高字节
        data_buffer[1] = data[i] & 0xFF;        // 低字节
        
        // 发送两个字节
        (void)SPI1_TX_WithFallback(data_buffer, 2);
    }
    
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

/**
  * @brief  显示一张图片（16位数组格式）
  * @param  img_data: 指向RGB565格式图片数据的指针（大小为126×294）
  * @retval None
  * @note   图片数据必须是RGB565格式，大小正好是126×294像素
  */
void LCD_ShowImage(const uint16_t *img_data)
{
    // 设置全屏为窗口
    LCD_SetWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    
    // 使用优化的批量传输写入所有像素
    uint32_t total_pixels = LCD_WIDTH * LCD_HEIGHT;
    LCD_WritePixels_Bulk(img_data, total_pixels);
}

/**
  * @brief  在指定位置显示小尺寸图片（16位数组格式）
  * @param  x: 显示位置的X坐标起点
  * @param  y: 显示位置的Y坐标起点
  * @param  width: 图片宽度（像素）
  * @param  height: 图片高度（像素）
  * @param  img_data: 指向RGB565格式图片数据的指针（大小为 width×height）
  * @retval None
  * @note   图片数据必须是RGB565格式，大小为 width×height 像素
  *         用于在屏幕指定位置显示任意尺寸的小图片，适合UI绘制
  */
void LCD_ShowPartialImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint16_t *img_data)
{
    uint16_t x1, y1;
    uint32_t total_pixels;
    
    // 边界检查，确保不超出屏幕范围
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    // 计算实际显示区域（限制在屏幕范围内）
    x1 = (x + width - 1 < LCD_WIDTH) ? (x + width - 1) : (LCD_WIDTH - 1);
    y1 = (y + height - 1 < LCD_HEIGHT) ? (y + height - 1) : (LCD_HEIGHT - 1);
    
    // 设置显示窗口
    LCD_SetWindow(x, y, x1, y1);
    
    // 计算实际像素数量
    total_pixels = (x1 - x + 1) * (y1 - y + 1);
    
    // 使用优化的批量传输写入像素数据
    LCD_WritePixels_Bulk(img_data, total_pixels);
}

/**
  * @brief  显示一张图片（字节数组格式 - Img2lcd单字节模式，大端输出）
  * @param  img_bytes: 指向字节数组的指针（大小为126×294×2字节）
  * @retval None
  * @note   字节数组格式：每2个字节组成一个RGB565像素
  *         假设Img2lcd设置"大端模式"：[高字节][低字节] 格式
  *         如果图片显示错误，使用 LCD_ShowImageBytesOrder 并切换字节顺序
  */
void LCD_ShowImageBytes(const uint8_t *img_bytes)
{
    uint32_t total_bytes = LCD_WIDTH * LCD_HEIGHT * 2;
    uint32_t sent_bytes = 0;
    uint16_t chunk_size;
    
    // 设置全屏为窗口
    LCD_SetWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    
    // 分段发送数据（SPI TSIZE最大65535）
    while (sent_bytes < total_bytes)
    {
        chunk_size = (total_bytes - sent_bytes > 60000) ? 60000 : (uint16_t)(total_bytes - sent_bytes);
        (void)SPI1_TX_WithFallback(img_bytes + sent_bytes, chunk_size);
        sent_bytes += chunk_size;
    }
    
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

/**
  * @brief  在指定位置显示小尺寸图片（字节数组格式）
  * @param  x: 显示位置的X坐标起点
  * @param  y: 显示位置的Y坐标起点
  * @param  width: 图片宽度（像素）
  * @param  height: 图片高度（像素）
  * @param  img_bytes: 指向字节数组的指针（大小为 width×height×2 字节）
  * @retval None
  * @note   字节数组格式：每2个字节组成一个RGB565像素
  *         假设Img2lcd设置"大端模式"：[高字节][低字节] 格式
  *         用于在屏幕指定位置显示任意尺寸的小图片，适合UI绘制
  */
void LCD_ShowPartialImageBytes(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *img_bytes)
{
    uint16_t x1, y1;
    uint32_t total_bytes;
    uint32_t sent_bytes = 0;
    uint16_t chunk_size;
    
    // 边界检查，确保不超出屏幕范围
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    // 计算实际显示区域（限制在屏幕范围内）
    x1 = (x + width - 1 < LCD_WIDTH) ? (x + width - 1) : (LCD_WIDTH - 1);
    y1 = (y + height - 1 < LCD_HEIGHT) ? (y + height - 1) : (LCD_HEIGHT - 1);
    
    // 设置显示窗口
    LCD_SetWindow(x, y, x1, y1);
    
    // 计算实际字节数量
    total_bytes = (x1 - x + 1) * (y1 - y + 1) * 2;
    
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    
    // 分段发送数据（SPI TSIZE最大65535）
    while (sent_bytes < total_bytes)
    {
        chunk_size = (total_bytes - sent_bytes > 60000) ? 60000 : (uint16_t)(total_bytes - sent_bytes);
        (void)SPI1_TX_WithFallback(img_bytes + sent_bytes, chunk_size);
        sent_bytes += chunk_size;
    }
    
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

/**
  * @brief  显示一张图片（字节数组格式 - 自定义字节顺序）
  * @param  img_bytes: 指向字节数组的指针（大小为126×294×2字节）
  * @param  byte_order: 字节顺序，0表示低字节在前，非0表示高字节在前
  * @retval None
  * @note   用于处理不同Img2lcd配置的字节顺序
  */
void LCD_ShowImageBytesOrder(const uint8_t *img_bytes, uint8_t byte_order)
{
    uint32_t i;
    uint8_t data_buffer[2];
    
    // 设置全屏为窗口
    LCD_SetWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    
    // 根据字节顺序重组数据
    uint32_t total_pixels = LCD_WIDTH * LCD_HEIGHT;
    for (i = 0; i < total_pixels; i++)
    {
        if (byte_order == 0) {
            // 低字节在前 [低][高]
            data_buffer[0] = img_bytes[i * 2 + 1];  // 高字节
            data_buffer[1] = img_bytes[i * 2];      // 低字节
        } else {
            // 高字节在前 [高][低]
            data_buffer[0] = img_bytes[i * 2];      // 高字节
            data_buffer[1] = img_bytes[i * 2 + 1];  // 低字节
        }
        
        (void)SPI1_TX_WithFallback(data_buffer, 2);
    }
    
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

/**
  * @brief  将4位灰度值转换为RGB565格式
  * @param  gray_4bit: 4位灰度值（0-15）
  * @retval RGB565格式的16位颜色值
  * @note   将16级灰度线性映射到RGB565，使用相同的R、G、B值
  *         灰度值反转：灰度值0映射到RGB(31,63,31)白色，灰度值15映射到RGB(0,0,0)黑色
  */
static uint16_t LCD_Convert16GrayToRGB565(uint8_t gray_4bit)
{
    // 反转灰度值：0→15, 1→14, ..., 15→0
    uint8_t inverted_gray = 15 - gray_4bit;
    
    // 将反转后的4位灰度值（0-15）线性映射到RGB565的5-6-5位
    // 使用线性插值：gray_value * (max_value / 15)
    uint8_t r = (inverted_gray * 31) / 15;  // R: 0-31 (5位)
    uint8_t g = (inverted_gray * 63) / 15;  // G: 0-63 (6位)
    uint8_t b = (inverted_gray * 31) / 15;  // B: 0-31 (5位)
    
    // 组合成RGB565格式：R(5位) + G(6位) + B(5位)
    uint16_t rgb565 = ((uint16_t)r << 11) |  // R: 5位，最高位
                      ((uint16_t)g << 5) |    // G: 6位，中间
                      (uint16_t)b;           // B: 5位，最低位
    
    return rgb565;
}

/**
  * @brief  在指定位置显示16灰度图片（字节数组格式，[高字节][低字节]）
  * @param  x: 显示位置的X坐标起点
  * @param  y: 显示位置的Y坐标起点
  * @param  width: 图片宽度（像素）
  * @param  height: 图片高度（像素）
  * @param  img_bytes: 指向16灰度图片数据的指针（大小为 width×height/2 字节）
  * @retval None
  * @note   数据格式：每个字节包含2个像素，高4位是第一个像素，低4位是第二个像素
  *         字节顺序：[高字节][低字节]格式
  *         16灰度值（0-15）会自动转换为RGB565格式显示
  *         用于在屏幕指定位置显示任意尺寸的16灰度图片，适合UI绘制
  *         实现方式：使用批量转换和分段传输，提高传输效率
  */
void LCD_ShowPartialImage16Gray(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *img_bytes)
{
    uint16_t x1, y1;
    uint32_t total_pixels;
    uint32_t byte_index;
    uint8_t gray_high, gray_low;
    uint16_t rgb565_value;
    
    // 边界检查，确保不超出屏幕范围
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    // 计算实际显示区域（限制在屏幕范围内）
    x1 = (x + width - 1 < LCD_WIDTH) ? (x + width - 1) : (LCD_WIDTH - 1);
    y1 = (y + height - 1 < LCD_HEIGHT) ? (y + height - 1) : (LCD_HEIGHT - 1);
    
    // 设置显示窗口
    LCD_SetWindow(x, y, x1, y1);
    
    // 计算实际像素数量
    total_pixels = (x1 - x + 1) * (y1 - y + 1);
    
    // 计算16灰度数据的总字节数（每个字节包含2个像素）
    uint32_t total_gray_bytes = (total_pixels + 1) / 2;
    
    // 使用缓冲区批量转换和传输
    // 缓冲区大小：每次处理最多30000字节（约15000像素，30000字节RGB565数据）
    #define CONVERSION_BUFFER_SIZE 30000
    static uint8_t rgb565_buffer[CONVERSION_BUFFER_SIZE];
    uint32_t converted_bytes = 0;
    uint16_t chunk_size;
    
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    
    // 批量转换16灰度数据为RGB565格式并发送
    for (byte_index = 0; byte_index < total_gray_bytes; byte_index++)
    {
        // 读取一个字节（包含2个4位灰度值）
        uint8_t byte_data = img_bytes[byte_index];
        
        // 提取高4位和低4位灰度值
        gray_high = (byte_data >> 4) & 0x0F;  // 高4位（第一个像素）
        gray_low = byte_data & 0x0F;           // 低4位（第二个像素）
        
        // 转换为RGB565格式并存储到缓冲区（高字节在前，低字节在后）
        rgb565_value = LCD_Convert16GrayToRGB565(gray_high);
        rgb565_buffer[converted_bytes++] = (rgb565_value >> 8) & 0xFF;  // 高字节
        rgb565_buffer[converted_bytes++] = rgb565_value & 0xFF;         // 低字节
        
        // 如果还有第二个像素需要处理
        if ((byte_index * 2 + 1) < total_pixels) {
            rgb565_value = LCD_Convert16GrayToRGB565(gray_low);
            rgb565_buffer[converted_bytes++] = (rgb565_value >> 8) & 0xFF;  // 高字节
            rgb565_buffer[converted_bytes++] = rgb565_value & 0xFF;         // 低字节
        }
        
        // 当缓冲区满或处理完所有数据时，批量发送
        if (converted_bytes >= CONVERSION_BUFFER_SIZE || (byte_index + 1) >= total_gray_bytes)
        {
            // 分段发送数据（SPI TSIZE最大65535）
            uint32_t buffer_sent = 0;
            while (buffer_sent < converted_bytes)
            {
                chunk_size = (converted_bytes - buffer_sent > 60000) ? 60000 : (uint16_t)(converted_bytes - buffer_sent);
                (void)SPI1_TX_WithFallback(rgb565_buffer + buffer_sent, chunk_size);
                buffer_sent += chunk_size;
            }
            converted_bytes = 0;  // 重置缓冲区计数器
        }
    }
    
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}
