#include "includes.h"

#define SW1 GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13)
#define A GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_14)
#define B GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_15)
void ROLLER_GetKeyCount(RollerTypeDef *obj)
{
static u32 delay_ms;
	
	obj->key  = SW1;
	if(obj->key!=obj->pre_key)
	{
		delay_ms++;
		if(delay_ms >ROLLER_DELAY_10MS)
		{
			delay_ms = 0;
			obj->key  = SW1;
			if(obj->key!=obj->pre_key)
			{
				if(obj->key == 0)
					obj->key_count += 1;
			}
			obj->pre_key=SW1;
		}	
	}

}
void ROLLER_GetDir(RollerTypeDef *obj)  //滾輪引腳判斷
{
	static u8 delay_ms_a,delay_ms_b;
	unsigned char pin_level_a;
	unsigned char pin_level_b;	
	pin_level_a = A;
	pin_level_b = B;	
	if(pin_level_a != obj->phase_A)
	{
		delay_ms_a++;
		if(delay_ms_a>ROLLER_DELAY_500US) 
		{
			obj->phase_A = pin_level_a;
			delay_ms_a = 0;
		}
	}
	if(pin_level_b != obj->phase_B)
	{
		delay_ms_b++;
		if(delay_ms_b>ROLLER_DELAY_500US) 
		{
			obj->phase_B = pin_level_b;
			delay_ms_b = 0;
		}
	}	
	if(obj->phase_A!=obj->pre_phase_A)            //A引脚发生变化
	{
		if(obj->pre_phase_B==0)      //判断B引脚=0顺时针转动计数+1
		{
			if(obj->phase_A==1)
			{
				//obj->direction--;
			}
			else             //B引脚=1  逆时针转动-1
			{
				obj->direction++;
			}	
		}
		else                    //A引脚没有发生变化
		{
			if(obj->phase_A==0)
			{
				//obj->direction--;
			}
			else
			{
				//obj->direction++;
			}							
		}
			obj->pre_phase_A=obj->phase_A;	
			//obj->pre_phase_B=obj->phase_B;		
	}
	else if(obj->phase_B!=obj->pre_phase_B)   //判斷B引脚发生变化
	{
		if(obj->pre_phase_A==0) 
		{
			if(obj->phase_B==1)               
			{
				//obj->direction++;
			}
			else
			{
				obj->direction--;
			}
		}
		else
		{
			if(obj->phase_B==0)
			{
				//obj->direction++;
			}
			else
			{
				//obj->direction--;
			}			
			
		}
		//obj->pre_phase_A=obj->phase_A;	
		obj->pre_phase_B=obj->phase_B;		
	}
}
//	obj->phase_A=A;
//	obj->phase_B=B;
/*	if(obj->phase_A!=obj->pre_phase_A)            //A引脚发生变化
	{
		delay_ms++;
		if(delay_ms> ROLLER_DELAY_500US)
		{
			delay_ms = 0;
			obj->phase_A=A;
			if(obj->phase_A!=obj->pre_phase_A) 
			{
				if(obj->pre_phase_B==0)      //判断B引脚=0顺时针转动计数+1
				{
					if(obj->phase_A==1)
					{
						obj->direction--;
					}
					else             //B引脚=1  逆时针转动-1
					{
						obj->direction++;
					}
					
				}
				else                    //A引脚没有发生变化
				{
					if(obj->phase_A==0)
					{
						obj->direction--;
					}
					else
					{
						obj->direction++;
					}							
				}
				obj->pre_phase_A=A;	
				obj->pre_phase_B=B;		
			}
		}
	}
	else if(obj->phase_B!=obj->pre_phase_B)   //判斷B引脚发生变化
	{
		delay_ms++;
		if(delay_ms> ROLLER_DELAY_500US)
		{
			delay_ms = 0;
			obj->phase_B=B;
			if(obj->phase_B!=obj->pre_phase_B)
			{
				if(obj->pre_phase_A==0) 
				{
					if(obj->phase_B==1)               
					{
						obj->direction++;
					}
					else
					{
						obj->direction--;
					}
					
				}
				else
				{
				if(obj->phase_B==0)
					{
						obj->direction++;
					}
					else
					{
						obj->direction--;
					}			
					
				}
				obj->pre_phase_A=A;	
				obj->pre_phase_B=B;		
			}
		}
	}	*/

void Roller_Init(RollerTypeDef  *rol)
{
	rol->direction = 0;
	rol->key = 0;
	rol->key_count = 0;
	rol->phase_A = 0;
	rol->phase_B = 0;
	rol->pre_key = 0;
	rol->pre_phase_A = 0;
	rol->pre_phase_B = 0;
    rol->rtu_data = 0;
	rol->rtu_data_clear=0;
	Init_Roller_Gpio();
}
	
void Roller_Fresh_Timer(RollerTypeDef  *rol)
{
	ROLLER_GetKeyCount(rol);
	ROLLER_GetDir(rol);
}
void Roller_Clear(RollerTypeDef  *rol)
{ 
	rol->rtu_data = 0;
	rol->rtu_data_clear = 0;
	rol->key_count = 0;
	rol->direction = 0;
}
void Roller_Dispose(RollerTypeDef  *rol)
{
	if(rol->rtu_data_clear!=0) Roller_Clear(rol);
	else
	rol->rtu_data = ((rol->key_count<<8)&0xff00)+(rol->direction&0x00ff);
}
