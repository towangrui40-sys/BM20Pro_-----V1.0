/*
************************************************************************************************
��Ҫ�İ����ļ�

�� ��: INCLUDES.C ucos�����ļ�
�� ��: Jean J. Labrosse
************************************************************************************************
*/

#ifndef __INCLUDES_H__
#define __INCLUDES_H__
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>

#include "ucos_ii.h"
#include "os_cpu.h"
#include "os_cfg.h"

#include "stm32f10x.h"

#include "sys.h"

#include "gpio.h"
#include "delay.h"
#include "myiic.h"
#include "adc.h"
#include "dac.h"
#include "usart.h"

#include "lIS302DL.h"
#include "door_ctrl.h"
#include "Card.h"

#include "rtu_api.h"
#include "rtu_protocol.h"
#include "rtu_process.h"


#define CTRL_RTU_PROTORCAL_VERSION		1

#define RTU_MEM(x) (x+CTRL_RTU_MEM_BASE)

#if (CTRL_RTU_PROTORCAL_VERSION == 1)
enum CTRL_RTU_ADDR
{		
	ADDR_GET_MEMSX = 0,
	ADDR_GET_MEMSY,
	ADDR_GET_MEMSZ,
///////////////////////////////////////	
	ADDR_SET_MOTOR,
	ADDR_SET_SPEED,
	ADDR_READ_SPEED,
	ADDR_MOTOR_STATE,
	ADDR_MOTOR_CURRENT,
///////////////////////////////////////		
	ADDR_SET_LED,
	
	ADDR_SET_AC1,
	ADDR_SET_AC2,
	ADDR_SET_AC3,

	ADDR_SET_CASE,
	ADDR_SET_LOCK,
	ADDR_CASE_STATE,
	ADDR_LOCK_STATE,
	ADDR_LOCATION_TEST,
	ADDR_TANK_LOCK,
	
	ADDR_HREG_TOTAL,
	RTU_MEM_BASE= 0x1000,
};
#endif

typedef struct
{
	short x;
	short y;
	short z;
	short set_motor;
	short set_speed;	
	short read_speed;		
	short motor_state;	
	short motor_current;	
	short set_led;
	short set_ac1;
	short set_ac2;
	short set_ac3;
	short set_case;       //1:Open case 2:close case
	short set_lock;
	short case_state;     //0:关到位 1:开到位 2:中间位置
	short lock_state;
	short location_test;  //0:在中间 1:在1号位 2:在2号位
	short tank_lock;
}CTRL_TypeDef;

	

typedef struct
{
	CTRL_TypeDef *ctrl;

}SysDataDef;


#endif































