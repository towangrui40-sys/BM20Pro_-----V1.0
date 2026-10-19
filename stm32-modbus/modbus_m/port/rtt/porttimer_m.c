/*
 * FreeModbus Libary: RT-Thread Port
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: porttimer_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions$
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mb_m.h"
#include "mbport.h"
#include "stm32f10x.h"

#if ( (MB_MASTER_RTU_ENABLED | MB_MASTER_ASCII_ENABLED) > 0)
/* ----------------------- Start implementation -----------------------------*/


BOOL
xMBMasterPortTimersInit( USHORT usTim1Timerout50us )
{
   	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	uint16_t PrescalerValue = 0;
	
	/* TIM4 clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_DeInit(TIM2);
	/* Compute the prescaler value */
	PrescalerValue = (uint16_t) (36000000UL / 20000) - 1; // 1/20000=50us 
	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = (uint16_t) usTim1Timerout50us;
	TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	
	//TIM_ARRPreloadConfig(TIM2, ENABLE);
	
	/* Configure one bit for preemption priority */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); //两级占先 八级副优先
	/* Enable the TIM4 Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	
	NVIC_Init(&NVIC_InitStructure);
	
	/* TIM IT DISABLE */
	TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
	/* TIM2 DISABLE counter */
	TIM_Cmd(TIM2,  ENABLE);
	return TRUE;
}


void
vMBMasterPortTimersEnable(  )
{	
	vMBMasterSetCurTimerMode(MB_TMODE_T35);
	
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	TIM_SetAutoreload(TIM2,xMBMasterGetTimerT35Count());
	TIM_SetCounter(TIM2, 0);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
//	TIM_Cmd(TIM2,  ENABLE);
}

void
vMBMasterPortTimersDisable(  )
{
//	TIM_Cmd(TIM2, DISABLE);
	TIM_SetCounter(TIM2,0x0000); 
	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
}

void MasterTIMERExpiredISR( void )
{
    (void)pxMBMasterPortCBTimerExpired();
}

void vMBMasterPortTimersConvertDelayEnable()
{
	USHORT timeout=MB_MASTER_DELAY_MS_CONVERT*20;//unit ms

	vMBMasterSetCurTimerMode( MB_TMODE_CONVERT_DELAY);
	
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	TIM_SetAutoreload(TIM2,timeout);
	TIM_SetCounter(TIM2, 0);

	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
}

void vMBMasterPortTimersRespondTimeoutEnable()
{
	USHORT timeout=MB_MASTER_TIMEOUT_MS_RESPOND*20;//unit ms
	vMBMasterSetCurTimerMode( MB_TMODE_RESPOND_TIMEOUT);
	
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	TIM_SetAutoreload(TIM2,timeout);
	TIM_SetCounter(TIM2, 0);

	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
}
void TIM2_IRQHandler(void)
{
  if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
  {
    /* Clear TIM4 Capture Compare1 interrupt pending bit*/
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	MasterTIMERExpiredISR( );
  }
}


#endif

