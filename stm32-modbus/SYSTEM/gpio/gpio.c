#include "gpio.h"
#include "door_ctrl.h"

//初始化PB5和PE5为输出口.并使能这两个口的时钟		    
//LED IO初始化
void GPIO_Config(void)
{
 
 GPIO_InitTypeDef  GPIO_InitStructure;
 	
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);	 //使能PB,PE端口时钟
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
 GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);	

/*-----------------------GPIO OUT CFG-------------------*/	
//PA5--AC_CTRL_EX PA6--DC_CTRL1 PA7--DC_CTRL2 PA8--DC_CTRL3 
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8;	
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz 	
 GPIO_Init(GPIOA, &GPIO_InitStructure);					 //根据设定参数初始化
	
//PB3--LEDRUN  PB12--RESET
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_12;	
 GPIO_Init(GPIOB, &GPIO_InitStructure);					 //根据设定参数初始化
	
//PC0--FWD  PC1--AC_CTRL3 PC2--AC_CTRL2 PC3--AC_CTRL1 PC13--REV
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 |GPIO_Pin_1 |GPIO_Pin_2 |GPIO_Pin_3|GPIO_Pin_13;	
 GPIO_Init(GPIOC, &GPIO_InitStructure);					 //根据设定参数初始化

 //PD2--BEEP
 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;	
 GPIO_Init(GPIOD, &GPIO_InitStructure);					 //根据设定参数初始化

/*-----------------------GPIO IN CFG-------------------*/		
//PA15--ALARM
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;//
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//
  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIO	
  
 //PC12 VBUS
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;//
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//
  GPIO_Init(GPIOC, &GPIO_InitStructure);//初始化GPIO	
/////////////////////////////////////////////////////////////////////
	MC_FWD(OFF); 
	MC_REV(OFF);
	MC_RESET(OFF);
	
	DC_CTRL1(OFF);
	DC_CTRL2(OFF);
	DC_CTRL3(OFF);
	
	AC_CTRL1(OFF);
	AC_CTRL2(OFF);
	AC_CTRL3(OFF);
	
	LED_RUN(OFF);
	BEEP_OUT(OFF);
}

//DC motor control
void Init_Motor_Gpio(void)  //
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);//使能GPIOA时钟

	GPIO_PinRemapConfig(GPIO_FullRemap_TIM3, ENABLE);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;//DC MOTOR PC8 M1 PC9 M2
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//普通输出模式
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//100MHz
  GPIO_Init(GPIOC, &GPIO_InitStructure);//初始化GPIO	
}

void Init_Door_State_Gpio(void)   //sensor input  Init
{
	//PA0--DOOR      PA1--LOCK  
	//case open      case close
  GPIO_InitTypeDef  GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;//
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//
  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIO	
	
	//PB13--U8       						PB14--U7 
	//tank_flag      				 		tank_lock
	//锁紧与解锁到位传感器       收到信号锁紧
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14;//
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//
  GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIO	
	
}
void Init_Wheel_Gpio(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);//使能GPIOD时钟
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15|GPIO_Pin_11;//rollor
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//普通输出模式
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIO	
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//使能GPIOD时钟
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;//rollor
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//普通输出模式
	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIO	
}
