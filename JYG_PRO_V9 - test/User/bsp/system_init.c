#include "system_init.h"
#include "key.h"
#include "beep.h"
#include "lcd.h"
#include "laser.h"
#include "tec.h"
#include "fan.h"
#include "wsd.h"
#include "motion_sensor.h"

void system_init(void)
{
    laser_init(); 
    key_init();      
    beep_init();     
    LCD_Init();      
    fan_init();      
    /* 系统初始化时风扇保持关闭（待机状态） */   
    tec_init();      
    wsd_init();      
    motion_sensor_init(); 
    
}
