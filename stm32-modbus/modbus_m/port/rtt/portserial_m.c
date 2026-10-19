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
 * File: $Id: portserial_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions $
 */

#include "port.h"
#include "usart.h"	
#include "stm32f10x.h"
/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"


#if ( (MB_MASTER_RTU_ENABLED | MB_MASTER_ASCII_ENABLED) > 0)
/* ----------------------- Static variables ---------------------------------*/
//ALIGN(RT_ALIGN_SIZE)
/* software simulation serial transmit IRQ handler thread stack */
//static rt_uint8_t serial_soft_trans_irq_stack[512];
///* software simulation serial transmit IRQ handler thread */
//static struct rt_thread thread_serial_soft_trans_irq;
///* serial event */
//static struct rt_event event_serial;
///* modbus master serial device */
//static rt_serial_t *serial;

/* ----------------------- Defines ------------------------------------------*/
/* serial transmit event */
#define EVENT_SERIAL_TRANS_START    (1<<0)

#ifdef USART_DMA
#define USART2_DMA_RX_BUFFER_MAX_LENGTH        (256)
#define USART2_DMA_TX_BUFFER_MAX_LENGTH        (256)
uint8_t USART2_DMA_RX_Buffer[USART2_DMA_RX_BUFFER_MAX_LENGTH];
uint8_t USART2_DMA_TX_Buffer[USART2_DMA_TX_BUFFER_MAX_LENGTH];
uint16_t DMA_MasterTX_Cnt = 0;
uint16_t DMA_MasterRX_Cnt = 0;
#endif
/* ----------------------- static functions ---------------------------------*/
static void prvvMasterUARTTxReadyISR( void );
static void prvvMasterUARTRxISR( void );
/* ----------------------- Start implementation -----------------------------*/

#ifdef USART_DMA
void USART2_DMA_Tx_Configuration(void)
{
	DMA_InitTypeDef  DMA_InitStructure;
	
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 , ENABLE);						//DMA1时钟使能
	DMA_DeInit(DMA1_Channel7);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART2->DR;		//DMA外设地址
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)USART2_DMA_TX_Buffer;	//发送缓存指针
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;						//传输方向
    DMA_InitStructure.DMA_BufferSize = USART2_DMA_TX_BUFFER_MAX_LENGTH;		//传输长度
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;		//外设递增
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 				//内存递增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//外设数据宽度：BYTE
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;			//内存数据宽度：BYTE
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;							//循环模式：否//（注：DMA_Mode_Normal为正常模式，DMA_Mode_Circular为循环模式）
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh; 				//优先级：高
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; 							//内存：内存（都）
	DMA_Init(DMA1_Channel7 , &DMA_InitStructure);							//初始化DMA1_Channel7
	DMA_ClearFlag(DMA1_FLAG_GL7);
	DMA_Cmd(DMA1_Channel7 , DISABLE); 										//开启DMA传输
}

void USART2_DMA_Rx_Configuration(void)
{
	DMA_InitTypeDef  DMA_InitStructure;
 
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 , ENABLE);						//DMA1时钟使能
    DMA_DeInit(DMA1_Channel6);  		
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART2->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)USART2_DMA_RX_Buffer;	//接收缓存指针    
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;                    
    DMA_InitStructure.DMA_BufferSize = USART2_DMA_RX_BUFFER_MAX_LENGTH;		//缓冲大小
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;       
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; 
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;        
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;							//（注：DMA_Mode_Normal为正常模式，DMA_Mode_Circular为循环模式）                           
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;               
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;                           
    DMA_Init(DMA1_Channel6, &DMA_InitStructure);            
    DMA_ClearFlag(DMA1_FLAG_GL6);                              
    DMA_Cmd(DMA1_Channel6 , ENABLE);  
}
#endif

BOOL xMBMasterPortSerialInit(UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
//	USART_ClockInitTypeDef USART_ClockInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	if(ucPORT == 2)
	{
		//使能串口1，PA，AFIO总线
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
		USART_DeInit(USART2);  //复位串口1

		 //USART1_TX   PA.9
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.9
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	   
		//USART1_RX	  PA.10
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
		GPIO_Init(GPIOA, &GPIO_InitStructure);  


	   //USART 初始化设置

		USART_InitStructure.USART_BaudRate = ulBaudRate;//一般设置为9600;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
		USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
		USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

		USART_Init(USART2, &USART_InitStructure);
		


		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); //两级占先 八级副优先

	   //Usart1 NVIC 配置

		NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//抢占优先级3
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//子优先级3
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
		NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
	  
	  /* ENABLE the USARTx */
		USART_Cmd(USART2, ENABLE);
		#ifdef USART_DMA	
		USART_ClearFlag(USART2, USART_FLAG_TC); //清除发送完成标志	
		while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);//等待空闲帧发送完成后再清零发送完成标志（警告：如果不使能USART_Mode_Tx，会导致单片机在这里死机）
		USART_ClearFlag(USART2, USART_FLAG_TC);	//清除发送完成标志
	 
		//USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
		//USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
		USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);
		USART_ITConfig(USART2, USART_IT_TC, ENABLE);
		USART_DMACmd(USART2 ,   USART_DMAReq_Tx,ENABLE);
		USART_DMACmd(USART2 ,   USART_DMAReq_Rx,ENABLE);
		USART2_DMA_Tx_Configuration();
		USART2_DMA_Rx_Configuration();
		#endif
	}
	else
	{
		//使能串口1，PA，AFIO总线
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
		USART_DeInit(USART1);  //复位串口1

		 //USART1_TX   PA.9
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	   
		//USART1_RX	  PA.10
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
		GPIO_Init(GPIOA, &GPIO_InitStructure);  


	   //USART 初始化设置

		USART_InitStructure.USART_BaudRate = ulBaudRate;//一般设置为9600;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
		USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
		USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

		USART_Init(USART1, &USART_InitStructure);
		


		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); //两级占先 八级副优先

	   //Usart1 NVIC 配置

		NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//抢占优先级3
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//子优先级3
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
		NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
	  
	  /* ENABLE the USARTx */
		USART_Cmd(USART1, ENABLE);
	}		
	return TRUE;
}



void vMBMasterPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable)
{
	if(TRUE==xRxEnable)
	{
		#ifndef USART_DMA
		USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
		#else
		USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);
		#endif
	}
	else
	{
		#ifndef USART_DMA
		USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
		#else
		USART_ITConfig(USART2, USART_IT_IDLE, DISABLE);
		#endif		
	}

	if(TRUE==xTxEnable)
	{
		#ifndef USART_DMA
		USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
		#else
		USART_ITConfig(USART2, USART_IT_TC, ENABLE);
		DMA_MasterTX_Cnt =0;
		prvvMasterUARTTxReadyISR();
		#endif
	}
	else
	{
		#ifndef USART_DMA
		USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
		#else
		USART_ITConfig(USART2, USART_IT_TC, DISABLE);
		#endif
	}
}

void vMBMasterPortClose(void)
{
	#ifndef USART_DMA
	USART_ITConfig(USART2, USART_IT_TXE|USART_IT_RXNE, DISABLE);
	USART_Cmd(USART2, DISABLE);
	#else
	USART_ITConfig(USART2, USART_IT_TC|USART_IT_IDLE, DISABLE);
	USART_Cmd(USART2, DISABLE);	
	#endif
}

BOOL xMBMasterPortSerialPutByte(CHAR ucByte)
{

#ifndef USART_DMA
	while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET)/*等待发送完成*/
	{  
	}		
	USART_SendData(USART2, ucByte);
#else
	USART2_DMA_TX_Buffer[DMA_MasterTX_Cnt++] = ucByte;
#endif
	return TRUE;

}
BOOL xMBMasterPortSerialDMASendData(void)
{
	if (DMA_MasterTX_Cnt < USART2_DMA_TX_BUFFER_MAX_LENGTH)
	{
		//memcpy(USART3_DMA_TX_Buffer , send_buffer , nSendBytes);
		DMA_Cmd(DMA1_Channel7 , DISABLE);                    				//关闭DMA传输
		DMA_SetCurrDataCounter(DMA1_Channel7 , DMA_MasterTX_Cnt);  				//数据传输量
		DMA_Cmd(DMA1_Channel7 , ENABLE);               						//开启DMA传输
		return TRUE;
	}
	else return FALSE;
}
BOOL xMBMasterPortSerialGetByte(CHAR * pucByte)
{
	*pucByte = USART_ReceiveData(USART2);
	return TRUE;
}
BOOL xMBMasterPortSerialDMARevData(CHAR * getBuf,USHORT *cnt)
{
	*cnt = DMA_MasterRX_Cnt;
	memcpy(getBuf , USART2_DMA_RX_Buffer , DMA_MasterRX_Cnt);
	DMA_MasterRX_Cnt =0;
	DMA_SetCurrDataCounter(DMA1_Channel6 , USART2_DMA_RX_BUFFER_MAX_LENGTH);
	return TRUE;
}
/* 
 * Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */
void prvvMasterUARTTxReadyISR(void)
{
    pxMBMasterFrameCBTransmitterEmpty();
}

/* 
 * Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
void prvvMasterUARTRxISR(void)
{
    pxMBMasterFrameCBByteReceived();
}

/*void USART1_IRQHandler(void)
{
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
	{
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);	
		prvvMasterUARTRxISR();//接受中断
	
	}

	if(USART_GetITStatus(USART3, USART_IT_TXE) == SET)
	{
		USART_ClearITPendingBit(USART3, USART_IT_TXE);
		prvvMasterUARTTxReadyISR();//发送完成中断

	}

	//溢出-如果发生溢出需要先读SR,再读DR寄存器 则可清除不断入中断的问题
	if(USART_GetFlagStatus(USART3,USART_FLAG_ORE)==SET)
	{
		USART_ClearFlag(USART3,USART_FLAG_ORE);	//读SR
		USART_ReceiveData(USART3);				//读DR
	}
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntExit();  											 
#endif
}*/
#ifndef USART_DMA
void USART2_IRQHandler(void)
{
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
	{
		prvvMasterUARTRxISR();//接受中断
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);		
	}

	if(USART_GetITStatus(USART2, USART_IT_TXE) == SET)
	{
		prvvMasterUARTTxReadyISR();//发送完成中断
		USART_ClearITPendingBit(USART2, USART_IT_TXE);
	}

	//溢出-如果发生溢出需要先读SR,再读DR寄存器 则可清除不断入中断的问题
	if(USART_GetFlagStatus(USART2,USART_FLAG_ORE)==SET)
	{
		USART_ClearFlag(USART2,USART_FLAG_ORE);	//读SR
		USART_ReceiveData(USART2);				//读DR
	}
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntExit();  											 
#endif
}
#else
//
//DMA中断方式
//
void USART2_IRQHandler(void)
{
	uint16_t ch;
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntEnter();    
#endif
	if (USART_GetITStatus(USART2,USART_IT_IDLE) != RESET)
	{		
		USART_ClearITPendingBit(USART2 , USART_IT_IDLE);	//必须先清除总线空闲中断标识，然后读一下数据寄存器，DMA接收才会正确（先读SR，然后读DR才能清除空闲中断标识）注意：这句必须要，否则不能够清除中断标志位。
		ch =  USART_ReceiveData(USART2);					//必须先清除总线空闲中断标识，然后读一下数据寄存器，DMA接收才会正确（先读SR，然后读DR才能清除空闲中断标识）注意：这句必须要，否则不能够清除中断标志位。
 
		DMA_Cmd(DMA1_Channel6 , DISABLE); 					//关闭DMA,防止处理其间有数据
		DMA_ClearFlag(DMA1_FLAG_GL6 | DMA1_FLAG_TC6 | DMA1_FLAG_HT6 | DMA1_FLAG_TE6);
		ch = USART2_DMA_RX_BUFFER_MAX_LENGTH - DMA_GetCurrDataCounter(DMA1_Channel6);
		if (ch > 0)
		{
			DMA_MasterRX_Cnt = ch;
		}
		prvvMasterUARTRxISR();
		DMA_Cmd(DMA1_Channel6, ENABLE);	
	}
	if (USART_GetITStatus(USART2,USART_IT_TC)!= RESET) 
	{
		USART_ClearITPendingBit(USART2, USART_IT_TC);				
		DMA_ClearFlag(DMA1_FLAG_GL7 | DMA1_FLAG_TC7 | DMA1_FLAG_HT7 | DMA1_FLAG_TE7);
		DMA_MasterTX_Cnt =0;
		DMA_SetCurrDataCounter(DMA1_Channel7 , 0);
	}	
	//溢出-如果发生溢出需要先读SR,再读DR寄存器 则可清除不断入中断的问题
	if(USART_GetFlagStatus(USART2,USART_FLAG_ORE)==SET)
	{
		USART_ClearFlag(USART2,USART_FLAG_ORE);	//读SR
		USART_ReceiveData(USART2);				//读DR
	}
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntExit();  											 
#endif
} 
#endif
#endif


