//############################################################
// FILE:  PI_Cale.h
// Created on: 2019年7月10日
// Author: XQ
// summary: Header file  and definition
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//版权所有，盗版必究
//STM32F4电机控制开发板
//硕历达电子  
//网址: https://shuolidianzi.taobao.com
//Author-QQ: 616264123
//电机控制QQ群：314306105
//############################################################

#ifndef  PI_Cale_H
#define  PI_Cale_H

#include "Sys.h"   

typedef struct {
        float  Ref;   // PI控制的给定参数
        float  Fbk;   // PI控制的反馈参数
        float  Out;   // PI控制的输出参数
        float  OutF;  // PI控制的滤波后输出参数
        float  Kp;        // PI控制的比例参数
        float  Ki;        // PI控制的积分参数
				float  Kd;		//PID控制的微分参数
        float  Umax;  // PI控制的输出限制最大幅值参数
        float  Umin;  // PI控制的输出限制最小幅值参数
        float  up;        // PI控制的比例项输出参数
        float  ui;        // PI控制的积分项输出参数
        float  v1;        // PI控制的历史输出项参数
        float  i1;        // PI控制的历史积分项输出参数
				float  ud;		// PI控制的微分项输出参数
				float  up_last;	//PID控制的输出微分历史参数
			} PI_Control, *p_PI_Control ;

#define PI_Control_DEFAULTS {0,0,0,0,0,0,0,1,0,0,0,0,0,0,0} // 初始化参数

extern  PI_Control   pi_temp ;


void  PI_Controller(p_PI_Control  pV);  //PI控制算法函数
void  PI_Pare_init(void );    //PI控制参数初始化

#endif /* PI_Cale*/
