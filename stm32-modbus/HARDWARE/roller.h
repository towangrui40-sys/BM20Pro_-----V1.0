#ifndef __ROLLER_H
#define __ROLLER_H	 
#include "sys.h"

#define ROLLER_DELAY_500US 5
#define ROLLER_DELAY_1MS 10
#define ROLLER_DELAY_5MS 50
#define ROLLER_DELAY_10MS 100

typedef struct
{
	u8 phase_A;
	u8 phase_B;
	u8 pre_phase_A;
	u8 pre_phase_B;
	u8 key;
	u8 pre_key;
    s8 direction;
	u8 key_count;
    u16 rtu_data;
	u16 rtu_data_clear;
} RollerTypeDef;

void Roller_Clear(RollerTypeDef  *rol);
void Roller_Init(RollerTypeDef  *rol);
void Roller_Fresh_Timer(RollerTypeDef  *rol);
void Roller_Dispose(RollerTypeDef  *rol);
#endif
