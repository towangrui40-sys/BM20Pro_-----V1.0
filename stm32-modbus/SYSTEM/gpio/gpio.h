#ifndef __GPIO_H
#define __GPIO_H	 

#include "stm32f10x.h"

#define ON	1
#define OFF	0

	//================电机控制端口============
	
//MC_FWD
#define MC_FWD(a)	if (a)	\
					GPIO_ResetBits(GPIOC,GPIO_Pin_0);\
					else		\
					GPIO_SetBits(GPIOC,GPIO_Pin_0)	
//MC_REV
#define MC_REV(a)	if (a)	\
					GPIO_ResetBits(GPIOC,GPIO_Pin_13);\
					else		\
					GPIO_SetBits(GPIOC,GPIO_Pin_13)	
//MC_RESET
#define MC_RESET(a)	if (a)	\
					GPIO_ResetBits(GPIOB,GPIO_Pin_12);\
					else		\
					GPIO_SetBits(GPIOB,GPIO_Pin_12)		
//MC_ALARM
					
#define MC_ALARM		PAin(15)						
#define DOOR				PAin(0)      		//DOOR接口      门开到位传感器
#define LOCK				PAin(1) 				//LOCK接口      门关到位传感器
//#define TANK_FLAG		PBin(13)      	//U8  tank_flag    锁紧与解锁到位传感器   				 		
#define TANK_LOCK		PBin(14)		    //U7  tank_lock    按下后 传感器收到信号锁紧
	//================直流控制端口============
	
//DC_CTRL1
#define DC_CTRL1(a)	if (a)	\
					GPIO_ResetBits(GPIOA,GPIO_Pin_6);\
					else		\
					GPIO_SetBits(GPIOA,GPIO_Pin_6)
//DC_CTRL2
#define DC_CTRL2(a)	if (a)	\
					GPIO_ResetBits(GPIOA,GPIO_Pin_7);\
					else		\
					GPIO_SetBits(GPIOA,GPIO_Pin_7)	
//DC_CTRL3
#define DC_CTRL3(a)	if (a)	\
					GPIO_ResetBits(GPIOA,GPIO_Pin_8);\
					else		\
					GPIO_SetBits(GPIOA,GPIO_Pin_8)					
//LEDRUN
#define LED_RUN(a)	if (a)	\
					GPIO_SetBits(GPIOB,GPIO_Pin_3);\
					else		\
					GPIO_ResetBits(GPIOB,GPIO_Pin_3)	
//BEEP_OUT
#define BEEP_OUT(a)	if (a)	\
					GPIO_SetBits(GPIOD,GPIO_Pin_2);\
					else		\
					GPIO_ResetBits(GPIOD,GPIO_Pin_2)	
					
//================AC控制端口============	
	
//AC_CTRL1
#define AC_CTRL1(a)	if (a)	\
					GPIO_ResetBits(GPIOC,GPIO_Pin_3);\
					else		\
					GPIO_SetBits(GPIOC,GPIO_Pin_3)	
//AC_CTRL2
#define AC_CTRL2(a)	if (a)	\
					GPIO_ResetBits(GPIOC,GPIO_Pin_2);\
					else		\
					GPIO_SetBits(GPIOC,GPIO_Pin_2)		
//AC_CTRL3
#define AC_CTRL3(a)	if (a)	\
					GPIO_ResetBits(GPIOC,GPIO_Pin_1);\
					else		\
					GPIO_SetBits(GPIOC,GPIO_Pin_1)	

					
			
#define WHEEL_RIGHT_HALL		PBin(15)
#define WHEEL_PISITION_HALL	PAin(5)		
#define WHEEL_LEFT_HALL			PBin(11)						
					
void GPIO_Config(void);//初始化
void Init_Motor_Gpio(void);
void Init_Door_State_Gpio(void);
void Init_Wheel_Gpio(void);
void Init_Roller_Gpio(void);
		 				    
#endif
