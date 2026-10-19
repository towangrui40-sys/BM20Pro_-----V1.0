/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
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
 * File: $Id: portserial.c,v 1.1 2006/08/22 21:35:13 wolti Exp $
 */

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "stm32f10x.h"

//#define USART3_ENABLE

#ifdef USART_DMA
#define USART1_DMA_RX_BUFFER_MAX_LENGTH        (256)
#define USART1_DMA_TX_BUFFER_MAX_LENGTH        (256)
uint8_t USART1_DMA_RX_Buffer[USART1_DMA_RX_BUFFER_MAX_LENGTH];
uint8_t USART1_DMA_TX_Buffer[USART1_DMA_TX_BUFFER_MAX_LENGTH];

#ifdef USART3_ENABLE
#define USART3_DMA_RX_BUFFER_MAX_LENGTH        (256)
#define USART3_DMA_TX_BUFFER_MAX_LENGTH        (256)
uint8_t USART3_DMA_RX_Buffer[USART3_DMA_RX_BUFFER_MAX_LENGTH];
uint8_t USART3_DMA_TX_Buffer[USART3_DMA_TX_BUFFER_MAX_LENGTH];
#endif

uint16_t DMA_TX_Cnt = 0;
uint16_t DMA_RX_Cnt = 0;
#endif
/* ----------------------- static functions ---------------------------------*/
static void prvvUARTTxReadyISR( void );
static void prvvUARTRxISR( void );

/* ----------------------- Start implementation -----------------------------*/
#ifdef USART_DMA
void USART1_DMA_Tx_Configuration(void)
{
	DMA_InitTypeDef  DMA_InitStructure;
	
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 , ENABLE);						//DMA1时钟使能
	DMA_DeInit(DMA1_Channel4);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;		//DMA外设地址
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)USART1_DMA_TX_Buffer;	//发送缓存指针
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;						//传输方向
    DMA_InitStructure.DMA_BufferSize = USART1_DMA_TX_BUFFER_MAX_LENGTH;		//传输长度
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;		//外设递增
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 				//内存递增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//外设数据宽度：BYTE
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;			//内存数据宽度：BYTE
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;							//循环模式：否//（注：DMA_Mode_Normal为正常模式，DMA_Mode_Circular为循环模式）
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh; 				//优先级：高
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; 							//内存：内存（都）
	DMA_Init(DMA1_Channel4 , &DMA_InitStructure);							//初始化DMA1_Channel2
	DMA_ClearFlag(DMA1_FLAG_GL4);
	DMA_Cmd(DMA1_Channel4 , DISABLE); 										//开启DMA传输
}

void USART1_DMA_Rx_Configuration(void)
{
	DMA_InitTypeDef  DMA_InitStructure;
 
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 , ENABLE);						//DMA1时钟使能
    DMA_DeInit(DMA1_Channel5);  		
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART1->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)USART1_DMA_RX_Buffer;	//接收缓存指针    
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;                    
    DMA_InitStructure.DMA_BufferSize = USART1_DMA_RX_BUFFER_MAX_LENGTH;		//缓冲大小
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;       
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; 
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;        
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;							//（注：DMA_Mode_Normal为正常模式，DMA_Mode_Circular为循环模式）                           
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;               
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;                           
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);            
    DMA_ClearFlag(DMA1_FLAG_GL5);                              
    DMA_Cmd(DMA1_Channel5 , ENABLE);  
}
#ifdef USART3_ENABLE
void USART3_DMA_Tx_Configuration(void)
{
	DMA_InitTypeDef  DMA_InitStructure;
	
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 , ENABLE);						//DMA1时钟使能
	DMA_DeInit(DMA1_Channel2);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART3->DR;		//DMA外设地址
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)USART3_DMA_TX_Buffer;	//发送缓存指针
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;						//传输方向
    DMA_InitStructure.DMA_BufferSize = USART3_DMA_TX_BUFFER_MAX_LENGTH;		//传输长度
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;		//外设递增
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 				//内存递增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//外设数据宽度：BYTE
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;			//内存数据宽度：BYTE
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;							//循环模式：否//（注：DMA_Mode_Normal为正常模式，DMA_Mode_Circular为循环模式）
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh; 				//优先级：高
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; 							//内存：内存（都）
	DMA_Init(DMA1_Channel2 , &DMA_InitStructure);							//初始化DMA1_Channel2
	DMA_ClearFlag(DMA1_FLAG_GL2);
	DMA_Cmd(DMA1_Channel2 , DISABLE); 										//开启DMA传输
}

void USART3_DMA_Rx_Configuration(void)
{
	DMA_InitTypeDef  DMA_InitStructure;
 
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 , ENABLE);						//DMA1时钟使能
    DMA_DeInit(DMA1_Channel3);  		
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART3->DR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)USART3_DMA_RX_Buffer;	//接收缓存指针    
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;                    
    DMA_InitStructure.DMA_BufferSize = USART3_DMA_RX_BUFFER_MAX_LENGTH;		//缓冲大小
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;       
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;                
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; 
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;        
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;							//（注：DMA_Mode_Normal为正常模式，DMA_Mode_Circular为循环模式）                           
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;               
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;                           
    DMA_Init(DMA1_Channel3, &DMA_InitStructure);            
    DMA_ClearFlag(DMA1_FLAG_GL3);                              
    DMA_Cmd(DMA1_Channel3 , ENABLE);  
}
#endif
#endif
void
vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
    /* If xRXEnable enable serial receive interrupts. If xTxENable enable
     * transmitter empty interrupts.
     */
	if(TRUE==xRxEnable)
	{
		#ifndef USART_DMA
		USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
		#else
		USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
		#endif
	}
	else
	{
		#ifndef USART_DMA
		USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
		#else
		USART_ITConfig(USART1, USART_IT_IDLE, DISABLE);
		#endif		
	}

	if(TRUE==xTxEnable)
	{
		#ifndef USART_DMA
		USART_ITConfig(USART3, USART_IT_TXE, ENABLE);
		#else
		USART_ITConfig(USART1, USART_IT_TC, ENABLE);
		DMA_TX_Cnt =0;
		prvvUARTTxReadyISR();
		#endif
	}
	else
	{
		#ifndef USART_DMA
		USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
		#else
		USART_ITConfig(USART1, USART_IT_TC, DISABLE);
		#endif
	}
}

BOOL
xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	//	USART_ClockInitTypeDef USART_ClockInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	if(ucPORT == 1)
	{
		//使能串口1，PA，AFIO总线
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE); //使能USART2
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
		USART_DeInit(USART1);  //复位串口1

		/* A2 USART1_Tx */
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		//推挽输出-TX
		GPIO_Init(GPIOA, &GPIO_InitStructure);

		/* A3 USART1_Rx  */
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入-RX
		GPIO_Init(GPIOA, &GPIO_InitStructure);


		USART_InitStructure.USART_BaudRate = ulBaudRate; //目前只修改波特率
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		USART_InitStructure.USART_Parity = USART_Parity_No;
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
		
		USART_Init(USART1, &USART_InitStructure);


		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); //两级占先 八级副优先

		 /* Enable USART1_IRQn  Channel */
		NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
		/* ENABLE the USARTx */
		USART_Cmd(USART1, ENABLE);	

		#ifdef USART_DMA	
		USART_ClearFlag(USART1, USART_FLAG_TC); //清除发送完成标志	
		while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);//等待空闲帧发送完成后再清零发送完成标志（警告：如果不使能USART_Mode_Tx，会导致单片机在这里死机）
		USART_ClearFlag(USART1, USART_FLAG_TC);	//清除发送完成标志
	 
		//USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
		//USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
		USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
		USART_ITConfig(USART1, USART_IT_TC, ENABLE);
		USART_DMACmd(USART1 ,   USART_DMAReq_Tx,ENABLE);
		USART_DMACmd(USART1 ,   USART_DMAReq_Rx,ENABLE);
		USART1_DMA_Tx_Configuration();
		USART1_DMA_Rx_Configuration();
		#endif
	}
	else if(ucPORT == 3)
	{
		#ifdef USART3_ENABLE
		//使能串口3，PA，AFIO总线
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);		//使能复用时钟
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);	//使能USART3时钟
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);		//使能GPIOC时钟
		GPIO_PinRemapConfig(GPIO_PartialRemap_USART3,ENABLE);   //USART3重映射
		USART_DeInit(USART3);  //复位串口1

		/* A2 USART3_Tx */
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		//推挽输出-TX
		GPIO_Init(GPIOC, &GPIO_InitStructure);

		/* A3 USART3_Rx  */
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入-RX
		GPIO_Init(GPIOC, &GPIO_InitStructure);


		USART_InitStructure.USART_BaudRate = ulBaudRate; //目前只修改波特率
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		USART_InitStructure.USART_Parity = USART_Parity_No;
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
		
		USART_Init(USART3, &USART_InitStructure);

		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); //两级占先 八级副优先

		 /* Enable USART3_IRQn  Channel */
		NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);
		/* ENABLE the USARTx */
		USART_Cmd(USART3, ENABLE);	
		#ifdef USART_DMA	
		USART_ClearFlag(USART3, USART_FLAG_TC); //清除发送完成标志	
		while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);//等待空闲帧发送完成后再清零发送完成标志（警告：如果不使能USART_Mode_Tx，会导致单片机在这里死机）
		USART_ClearFlag(USART3, USART_FLAG_TC);	//清除发送完成标志
	 
		//USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
		//USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
		USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);
		USART_ITConfig(USART3, USART_IT_TC, ENABLE);
		USART_DMACmd(USART3 ,   USART_DMAReq_Tx,ENABLE);
		USART_DMACmd(USART3 ,   USART_DMAReq_Rx,ENABLE);
		USART3_DMA_Tx_Configuration();
		USART3_DMA_Rx_Configuration();
		#endif
		#endif
	}
	else
	{


	}

		
	return TRUE;
}

BOOL
xMBPortSerialPutByte( CHAR ucByte )
{
    /* Put a byte in the UARTs transmit buffer. This function is called
     * by the protocol stack if pxMBFrameCBTransmitterEmpty( ) has been
     * called. */
#ifndef USART_DMA
	USART_SendData(USART3, ucByte);
	//USART_SendData(USART2, ucByte);
#else
	USART1_DMA_TX_Buffer[DMA_TX_Cnt++] = ucByte;
#endif
	return TRUE;
}
BOOL
xMBPortSerialDMASendData(void)
{
	if (DMA_TX_Cnt < USART1_DMA_TX_BUFFER_MAX_LENGTH)
	{
		//memcpy(USART3_DMA_TX_Buffer , send_buffer , nSendBytes);
		DMA_Cmd(DMA1_Channel4 , DISABLE);                    				//关闭DMA传输
		DMA_SetCurrDataCounter(DMA1_Channel4 , DMA_TX_Cnt);  				//数据传输量
		DMA_Cmd(DMA1_Channel4 , ENABLE);               						//开启DMA传输
		return TRUE;
	}
	else return FALSE;
	
}


BOOL
xMBPortSerialGetByte( CHAR * pucByte )
{
    /* Return the byte in the UARTs receive buffer. This function is called
     * by the protocol stack after pxMBFrameCBByteReceived( ) has been called.
     */
	*pucByte = USART_ReceiveData(USART1);

    return TRUE;
}

BOOL	
xMBPortSerialDMARevData(CHAR * getBuf,USHORT *cnt)
{
	*cnt = DMA_RX_Cnt;
	memcpy(getBuf , USART1_DMA_RX_Buffer , DMA_RX_Cnt);
	DMA_RX_Cnt =0;
	DMA_SetCurrDataCounter(DMA1_Channel5 , USART1_DMA_RX_BUFFER_MAX_LENGTH);	
	//prvvUARTRxISR();
	return TRUE;
}
/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */
static void prvvUARTTxReadyISR( void )
{
    pxMBFrameCBTransmitterEmpty(  );
}

/* Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
static void prvvUARTRxISR( void )
{
    pxMBFrameCBByteReceived(  );
}
#ifndef USART_DMA
void USART1_IRQHandler(void)
{
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
	{
		prvvUARTRxISR();//接受中断
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);		
	}

	if(USART_GetITStatus(USART1, USART_IT_TXE) == SET)
	{
		prvvUARTTxReadyISR();//发送完成中断
		USART_ClearITPendingBit(USART1, USART_IT_TXE);
	}

	//溢出-如果发生溢出需要先读SR,再读DR寄存器 则可清除不断入中断的问题
	if(USART_GetFlagStatus(USART1,USART_FLAG_ORE)==SET)
	{
		USART_ClearFlag(USART1,USART_FLAG_ORE);	//读SR
		USART_ReceiveData(USART1);				//读DR
	}
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntExit();  											 
#endif
}

/*void USART3_IRQHandler(void)
{
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntEnter();    
#endif
	if(USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
	{
		prvvUARTRxISR();//接受中断
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);		
	}

	if(USART_GetITStatus(USART3, USART_IT_TXE) == SET)
	{
		prvvUARTTxReadyISR();//发送完成中断
		USART_ClearITPendingBit(USART3, USART_IT_TXE);
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
#else
//
//DMA中断方式
//
void USART1_IRQHandler(void)
{
	uint16_t ch;
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntEnter();    
#endif
	
	if (USART_GetITStatus(USART1,USART_IT_IDLE) != RESET)
	{		
		USART_ClearITPendingBit(USART1 , USART_IT_IDLE);	//必须先清除总线空闲中断标识，然后读一下数据寄存器，DMA接收才会正确（先读SR，然后读DR才能清除空闲中断标识）注意：这句必须要，否则不能够清除中断标志位。
		ch =  USART_ReceiveData(USART1);					//必须先清除总线空闲中断标识，然后读一下数据寄存器，DMA接收才会正确（先读SR，然后读DR才能清除空闲中断标识）注意：这句必须要，否则不能够清除中断标志位。
 
		DMA_Cmd(DMA1_Channel5 , DISABLE); 					//关闭DMA,防止处理其间有数据
		DMA_ClearFlag(DMA1_FLAG_GL5 | DMA1_FLAG_TC5 | DMA1_FLAG_HT5 | DMA1_FLAG_TE5);
		ch = USART1_DMA_RX_BUFFER_MAX_LENGTH - DMA_GetCurrDataCounter(DMA1_Channel5);
		if (ch > 0)
		{
			DMA_RX_Cnt = ch;
		}
		prvvUARTRxISR();
		DMA_Cmd(DMA1_Channel5, ENABLE);	
	}
	if (USART_GetITStatus(USART1,USART_IT_TC)!= RESET) 
	{
		USART_ClearITPendingBit(USART1, USART_IT_TC);				
		DMA_ClearFlag(DMA1_FLAG_GL4 | DMA1_FLAG_TC4 | DMA1_FLAG_HT4 | DMA1_FLAG_TE4);
		DMA_TX_Cnt =0;
		DMA_SetCurrDataCounter(DMA1_Channel4 , 0);
	}	
	//溢出-如果发生溢出需要先读SR,再读DR寄存器 则可清除不断入中断的问题
	if(USART_GetFlagStatus(USART1,USART_FLAG_ORE)==SET)
	{
		USART_ClearFlag(USART1,USART_FLAG_ORE);	//读SR
		USART_ReceiveData(USART1);				//读DR
	}
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntExit();  											 
#endif
} 
#ifdef USART3_ENABLE
void USART3_IRQHandler(void)
{
	uint16_t ch;
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntEnter();    
#endif
	
	if (USART_GetITStatus(USART3,USART_IT_IDLE) != RESET)
	{		
		USART_ClearITPendingBit(USART3 , USART_IT_IDLE);	//必须先清除总线空闲中断标识，然后读一下数据寄存器，DMA接收才会正确（先读SR，然后读DR才能清除空闲中断标识）注意：这句必须要，否则不能够清除中断标志位。
		ch =  USART_ReceiveData(USART3);					//必须先清除总线空闲中断标识，然后读一下数据寄存器，DMA接收才会正确（先读SR，然后读DR才能清除空闲中断标识）注意：这句必须要，否则不能够清除中断标志位。
 
		DMA_Cmd(DMA1_Channel3 , DISABLE); 					//关闭DMA,防止处理其间有数据
		DMA_ClearFlag(DMA1_FLAG_GL3 | DMA1_FLAG_TC3 | DMA1_FLAG_HT3 | DMA1_FLAG_TE3);
		ch = USART3_DMA_RX_BUFFER_MAX_LENGTH - DMA_GetCurrDataCounter(DMA1_Channel3);
		if (ch > 0)
		{
			DMA_RX_Cnt = ch;
		}
		prvvUARTRxISR();
		DMA_Cmd(DMA1_Channel3, ENABLE);	
	}
	if (USART_GetITStatus(USART3,USART_IT_TC)!= RESET) 
	{
		USART_ClearITPendingBit(USART3, USART_IT_TC);				
		DMA_ClearFlag(DMA1_FLAG_GL2 | DMA1_FLAG_TC2 | DMA1_FLAG_HT2 | DMA1_FLAG_TE2);
		DMA_TX_Cnt =0;
		DMA_SetCurrDataCounter(DMA1_Channel2 , 0);
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
} 
#endif
#endif

