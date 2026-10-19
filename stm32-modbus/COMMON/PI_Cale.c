 //############################################################
// FILE:  PI_Cale.c
// Created on: 2017年12月10日
// Author: 
// summary: PI_Cale
//############################################################

#include "PI_Cale.h"

PI_Control   pi_temp=PI_Control_DEFAULTS ;

float  Limit_Sat( float Uint,float U_max, float U_min) //限制赋值函数
{
	float Uout;
 	if(Uint<= U_min)
		Uout= U_min;
	else if( Uint>=U_max)
		Uout=U_max;
	else
	  Uout= Uint;
  return  Uout;
}

void  PI_Controller(p_PI_Control  pV)
{
    /* proportional term */
	pV->up = pV->Ref - pV->Fbk; // 偏差计算
	//pV->up = 0-pV->up;
	/* integral term */
	pV->ui = (pV->Out == pV->v1)?(pV->Ki*pV->up+ pV->i1): pV->i1;  // 抗积分饱和，锁定PI饱和时的积分值
	pV->i1 = pV->ui;  // 当out等于v1时输出，没有达到最大值，没有限制输出，当不等于的时候就是溢出最大，消除积分累加项
	
	/* Derivative term*/
	pV->ud = pV->Kd*(pV->up - pV->up_last);
	pV->up_last = pV->up;
	/* control output*/
	pV->v1 = pV->Kp*pV->up  + pV->ui+pV->ud;  // 比例量角积分量
	
	pV->Out= Limit_Sat(pV->v1, pV->Umax, pV->Umin); // 限制PI输出，超出最大值

	//pV->Out =  pV->Umax+pV->Umin-pV->Out;
//	//error
//	pV->up = pV->Ref - pV->Fbk;
//	//errorsum
//	pV->ui += pV->up*pV->Ki;
//	
//	pV->Out = pV->Kp * pV->up + pV->ui;		//得到PI运算结果

//	pV->Out= Limit_Sat(pV->Out, pV->Umax, pV->Umin); // 限制PI输出，超出最大值	
	
}

void  PI_Pare_init(void )
{
	pi_temp.Kp=1.0f;
	pi_temp.Ki=0.0f;
	pi_temp.Kd = 0.0f;
	pi_temp.Umax =100.0; 
	pi_temp.Umin =10.0; 
	
}
//===========================================================================
// No more.
//===========================================================================

