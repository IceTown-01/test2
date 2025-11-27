#include <stdio.h>
#include "gpio.h"
#include "stm32f10x.h"
#include "tim.h"
#include "stdbool.h"
#include "usart4.h"

#define PWM_state  0

volatile u8 left = 0;
volatile u8 right = 0;
volatile bool turn = 0;

#define fac_us 			((SystemCoreClock / 8)/1000000) 
#define fac_ms 			fac_us*1000
 
void Delay_Init(void)
{
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);  //初始化 SysTick 时钟 选择 HCLK/8 = 9M
}




void Delay_us(uint32_t us) //最大值 1864135
{
	uint32_t temp=0;
	
	SysTick->LOAD = fac_us * us;  //给 Load 寄存器赋初值
	SysTick->VAL  = 0;            //将 VAL  寄存器清空
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; //打开 SysTick
	do
	{
		temp = SysTick->CTRL;
	}while((temp&0x01) && !(temp&(1<<16)));  //判断 SysTick 是否使能并且 VAL 是否为 0
	SysTick->CTRL &= !SysTick_CTRL_ENABLE_Msk; //关闭 SysTick
	SysTick->VAL  = 0;            //将 VAL  寄存器清空
}


void Delay_ms(uint32_t ms) //最大值 1864
{
	uint32_t temp=0;
//	uint32_t Val = 0;
	
	SysTick->LOAD = fac_ms * ms;  //给 Load 寄存器赋初值
	SysTick->VAL  = 0;            //将 VAL  寄存器清空
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; //打开 SysTick
	do
	{
//		Val = SysTick->VAL;
		temp = SysTick->CTRL;
	}while((temp&0x01) && !(temp&(1<<16)));  //判断 SysTick 是否使能并且 VAL 是否为 0
	SysTick->CTRL &= !SysTick_CTRL_ENABLE_Msk; //关闭 SysTick
	SysTick->VAL  = 0;            //将 VAL  寄存器清空

}

// 改进的使能函数
void SGM42630_Enable(void) {
    GPIO_ResetBits(MOTOR21_EN_PORT, MOTOR21_EN_PIN);
    Delay_ms(2);  // 等待驱动芯片稳定（文档要求）
}

	
int main()
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
//	
//	u16 count;
//	bool state = 1;
	char text[10] = "nihao";
	SystemInit();
	Delay_Init();    // 必须先初始化SysTick
	
	MX_UART4_Init();
	UART4_NVIC_Init(2,1);
	
	Delay_ms(10);
	Motor20_GPIO_Init();    // 初始化GPIO
	Motor21_GPIO_Init();    // 初始化GPIO
	Motor22_GPIO_Init();    // 初始化GPIO
	
	GPIO_ResetBits(MOTOR20_EN_PORT, MOTOR20_EN_PIN);
	GPIO_ResetBits(MOTOR21_EN_PORT, MOTOR21_EN_PIN);
	GPIO_ResetBits(MOTOR22_EN_PORT, MOTOR22_EN_PIN);
	Delay_ms(2);  // 等待驱动芯片稳定（文档要求）
	GPIO_ResetBits(MOTOR21_DIR_PORT, MOTOR21_DIR_PIN); 			// 默认方向		set顺时针		左边左右，右边上下
	GPIO_SetBits(MOTOR22_DIR_PORT, MOTOR22_DIR_PIN);			  // 默认方向		res向右		向下
	GPIO_SetBits(MOTOR20_DIR_PORT, MOTOR20_DIR_PIN); 				//
	Delay_us(1);
	
	printf("%s",text);

	while(1) {
		
		GPIO_SetBits(MOTOR20_STEP_PORT, MOTOR20_STEP_PIN); // 
		GPIO_SetBits(MOTOR21_STEP_PORT, MOTOR21_STEP_PIN); // 
		GPIO_SetBits(MOTOR22_STEP_PORT, MOTOR22_STEP_PIN); // 
		Delay_us(50);						//40最大
		GPIO_ResetBits(MOTOR20_STEP_PORT, MOTOR20_STEP_PIN); //
		GPIO_ResetBits(MOTOR21_STEP_PORT, MOTOR21_STEP_PIN); //
		GPIO_ResetBits(MOTOR22_STEP_PORT, MOTOR22_STEP_PIN); //
		Delay_us(50);  
		
//		if(UART4_IDLE_FLAG == 1)
//		{
//			
//			GPIO_ResetBits(MOTOR20_DIR_PORT, MOTOR20_DIR_PIN);
//			
//		}
		
	
	}
  
  
}

