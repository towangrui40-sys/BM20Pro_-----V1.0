/******************** (C) COPYRIGHT 2015 ********************
* 文件名          : Card.c
* 作者            : 优灵科技开发团队
* 版本            : V3.1.0.0
* 日期            : 2015-5-10
* 描述            : 集合了模块对卡片的所有发送命令操作
										可以选择有用的调用，无用的删除
										还有命令的解析，将命令中的卡号和数据等存到Card结构体中
* 技术支持        : QQ: 3419865840
********************************************************************************/
/* 包含文件 -------------------------------------------------------------------*/
#include "Card.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
/* 宏定义 ---------------------------------------------------------------------*/
/* 本文件使用的变量 -----------------------------------------------------------*/
CARD Card;
CMD Cmd;
/* 本文件使用的函数声明 -------------------------------------------------------*/
unsigned char CheckSum(unsigned char *dat, unsigned char num);
void UartSendCommand(u8 *buff, u8 cnt);
u8 UartReceiveCommand(void);
/* 本文件函数体 ---------------------------------------------------------------*/


/*******************************************************************************
* 函数名         : OneKeyReadCard
* 描述           : 一键读卡。
* 输入           : 无
* 输出           : 无
* 返回           : 无
*******************************************************************************/
void OpenAutoReadCard(void)
{
	unsigned char len = 8;
	
	Cmd.SendBuffer[0] = 0x08;
	Cmd.SendBuffer[1] = 0xAB;
	Cmd.SendBuffer[2] = 0x00;
	Cmd.SendBuffer[3] = 0x03;
	Cmd.SendBuffer[4] = 0x08;
	Cmd.SendBuffer[5] = 0x05;
	Cmd.SendBuffer[6] = 0x02;
	Cmd.SendBuffer[7] = 0x07;
	Cmd.SendBuffer[8] = 0xA8;
	UartSendCommand(Cmd.SendBuffer, len + 1);
}

void CloseAutoReadCard(void)
{
	unsigned char len = 8;
	
	Cmd.SendBuffer[0] = 0x08;
	Cmd.SendBuffer[1] = 0xAB;
	Cmd.SendBuffer[2] = 0x01;
	Cmd.SendBuffer[3] = 0x03;
	Cmd.SendBuffer[4] = 0x08;
	Cmd.SendBuffer[5] = 0x05;
	Cmd.SendBuffer[6] = 0x02;
	Cmd.SendBuffer[7] = 0x07;
	Cmd.SendBuffer[8] = 0xA9;
	UartSendCommand(Cmd.SendBuffer, len + 1);
}

/*******************************************************************************
* 函数名         : OneKeyReadCard
* 描述           : 一键读卡。
* 输入           : 无
* 输出           : 无
* 返回           : 无
*******************************************************************************/
void OneKeyReadCard(void)
{
	unsigned char len = 2;
	
	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0x10;
	Cmd.SendBuffer[2] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len + 1);
}

/*******************************************************************************
* 函数名         : OneKeyMakeCard
* 描述           : 一键办卡。
* 输入           : block: 将要操作的块地址。
                   value: 将要操作的值。
                   keyA : 办卡设置的密钥A。
									 keyB : 办卡设置的密钥B。
* 输出           : 无
* 返回           : 无
*******************************************************************************/
void OneKeyMakeCard(unsigned char block, unsigned long value, unsigned char *keyA, unsigned char *keyB, unsigned char *keyBOld)
{
	unsigned char i, len = 25;
	
	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0x11;
	Cmd.SendBuffer[2] = block;
	Cmd.SendBuffer[3] = (unsigned char)(value >> 24);
	Cmd.SendBuffer[4] = (unsigned char)(value >> 16);
	Cmd.SendBuffer[5] = (unsigned char)(value >> 8);
	Cmd.SendBuffer[6] = (unsigned char)(value);
	for(i = 0; i < 6; i ++)
	{
		Cmd.SendBuffer[7 + i] = keyA[i];
	}
	for(i = 0; i < 6; i ++)
	{
		Cmd.SendBuffer[13 + i] = keyB[i];
	}
	for(i = 0; i < 6; i ++)
	{
		Cmd.SendBuffer[19 + i] = keyBOld[i];
	}
	Cmd.SendBuffer[25] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len + 1);
}

/*******************************************************************************
* 函数名         : OneKeyInc
* 描述           : 一键充值。
* 输入           : block: 将要操作的块地址。
                   key  : 办卡设置的密钥。
                   value: 将要操作的值。
* 输出           : 无
* 返回           : 无
*******************************************************************************/
void OneKeyInc(unsigned char block, unsigned char *key, unsigned long value)
{
	unsigned char i, len = 13;
	
	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0x12;
	Cmd.SendBuffer[2] = block;
	for(i = 0; i < 6; i ++)
	{
		Cmd.SendBuffer[3 + i] = key[i];
	}
	Cmd.SendBuffer[9] = (unsigned char)(value >> 24);
	Cmd.SendBuffer[10] = (unsigned char)(value >> 16);
	Cmd.SendBuffer[11] = (unsigned char)(value >> 8);
	Cmd.SendBuffer[12] = (unsigned char)(value);
	Cmd.SendBuffer[13] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len + 1);
}

/*******************************************************************************
* 函数名         : OneKeyDec
* 描述           : 一键扣款。
* 输入           : block: 将要操作的块地址。
                   key  : 办卡设置的密钥。
                   value: 将要操作的值。
* 输出           : 无
* 返回           : 无
*******************************************************************************/
void OneKeyDec(unsigned char block, unsigned char *key, unsigned long value)
{
	unsigned char i, len = 13;
	
	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0x13;
	Cmd.SendBuffer[2] = block;
	for(i = 0; i < 6; i ++)
	{
		Cmd.SendBuffer[3 + i] = key[i];
	}
	Cmd.SendBuffer[9] = (unsigned char)(value >> 24);
	Cmd.SendBuffer[10] = (unsigned char)(value >> 16);
	Cmd.SendBuffer[11] = (unsigned char)(value >> 8);
	Cmd.SendBuffer[12] = (unsigned char)(value);
	Cmd.SendBuffer[13] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len + 1);
}

/*******************************************************************************
* 函数名         : OneKeyReadBlock
* 描述           : 一键读块。
* 输入           : block: 将要操作的块地址。
                   keyA : 密钥A。
* 输出           : 无
* 返回           : 无
*******************************************************************************/
void OneKeyReadBlock(unsigned char block, unsigned char *keyA)
{
	unsigned char i, len = 9;
	
	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0x14;
	Cmd.SendBuffer[2] = block;
	for(i = 0; i < 6; i ++)
	{
		Cmd.SendBuffer[3 + i] = keyA[i];
	}
	Cmd.SendBuffer[9] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len + 1);
}

/*******************************************************************************
* 函数名         : OneKeyWriteBlock
* 描述           : 一键写块。
* 输入           : block: 将要操作的块地址。
                   keyB : 密钥B。
* 输出           : 无
* 返回           : 无
*******************************************************************************/
void OneKeyWriteBlock(unsigned char block, unsigned char *keyB, unsigned char *blockData)
{
	unsigned char i, len = 25;
	
	Cmd.SendBuffer[0] = len;
	Cmd.SendBuffer[1] = 0x15;
	Cmd.SendBuffer[2] = block;
	for(i = 0; i < 6; i ++)
	{
		Cmd.SendBuffer[3 + i] = keyB[i];
	}
	for(i = 0; i < 16; i ++)
	{
		Cmd.SendBuffer[9 + i] = blockData[i];
	}
	Cmd.SendBuffer[25] = CheckSum(Cmd.SendBuffer, len);
	UartSendCommand(Cmd.SendBuffer, len + 1);
}
/*******************************************************************************
* 函数名         : CommandProcess
* 描述           : 命令处理。
* 输入           : 无
* 输出           : 无
* 返回           : 执行结果。
                   -0   : 正确。
                   -0xFF: 无卡。
                   -0xFE: 卡操作错误。
                   -0xFD: 没有收到命令。
                   -0xFC: 余额不足。
                   -0xFB: 校验错误。
*******************************************************************************/
unsigned char CommandProcess(void)
{
	unsigned char i, status, bTemp;
	
	//接收到命令
	bTemp = UartReceiveCommand();
	if(bTemp == 1)
	{
		//校验命令
		if(Cmd.ReceiveBuffer[Cmd.ReceiveBuffer[0]] == CheckSum(Cmd.ReceiveBuffer, Cmd.ReceiveBuffer[0]))
		{
			//获取卡号，经过UartReceiveCommnad处理过后，有以下规律
			//Cmd.ReceiveBuffer[0] 		-> 命令长度
			//Cmd.ReceiveBuffer[1] 		-> 命令码
			//Cmd.ReceiveBuffer[2] 		-> 状态码
			//Cmd.ReceiveBuffer[3~4] 	-> 卡类型
			//Cmd.ReceiveBuffer[5~8] 	-> 卡号
			Card.UID = Cmd.ReceiveBuffer[8]; Card.UID <<= 8;
			Card.UID |= Cmd.ReceiveBuffer[7]; Card.UID <<= 8;
			Card.UID |= Cmd.ReceiveBuffer[6]; Card.UID <<= 8;
			Card.UID |= Cmd.ReceiveBuffer[5];
			Cmd.Code = Cmd.ReceiveBuffer[1];
			switch(Cmd.Code)
			{
				//一键读卡，这里不能删除
				case 0x10: break;
				//一键办卡，这里不能删除
				case 0x11: break;
				//一键充值
				case 0x12: 
					Card.Value = Cmd.ReceiveBuffer[9];
					Card.Value <<= 8;
					Card.Value |= Cmd.ReceiveBuffer[10];
					Card.Value <<= 8;
					Card.Value |= Cmd.ReceiveBuffer[11];
					Card.Value <<= 8;
					Card.Value |= Cmd.ReceiveBuffer[12];
					break;
				//一键扣款
				case 0x13: 
					Card.Value = Cmd.ReceiveBuffer[9];
					Card.Value <<= 8;
					Card.Value |= Cmd.ReceiveBuffer[10];
					Card.Value <<= 8;
					Card.Value |= Cmd.ReceiveBuffer[11];
					Card.Value <<= 8;
					Card.Value |= Cmd.ReceiveBuffer[12];
					break;
				//一键读块, Cmd.ReceiveBuffer[9~24] -> 数据
				case 0x14: 
					for(i = 0; i < 1; i++){Card.BlockData[i] = Cmd.ReceiveBuffer[10 + i];}
					break;
				//一键写块，这里不能删除
				case 0x15: break;
				default: status = 0xFB; break;
			}
			status = Cmd.ReceiveBuffer[2];
		}
		else {status = 0xFB;}
	}
	else{status = 0xFD;}
	return status;
}

/*******************************************************************************
* 函数名         : CheckSum
* 描述           : 命令校验。
* 输入           : 	dat : 要校验的数组。
										num : 校验的字节数。
* 输出           : 无
* 返回           : 校验值。
*******************************************************************************/
unsigned char CheckSum(unsigned char *dat, unsigned char num)
{
  unsigned char bTemp = 0, i;
  
  for(i = 0; i < num; i ++){bTemp ^= dat[i];}
  return bTemp;
}

/*******************************************************************************
* 函数名         : UartSendCommand
* 描述           : 串口发送命令
* 输入           : 	buff : 发送的数组。
										cnt  : 发送的长度
* 输出           : 无
* 返回           : 校验值。
*******************************************************************************/
void UartSendCommand(u8 *buff, u8 cnt)
{
  u8 i;
	
	//发送命令头
	
  USART_SendData(USART3, 0x7F);
	while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
  for(i = 0; i < cnt; i++)
  {
    USART_SendData(USART3, *(buff + i));
		while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
		//数据中有7F，就多发一个
		if(*(buff + i) == 0x7F)
		{
			USART_SendData(USART3, 0x7F);
			while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
		}
  }
}
/*******************************************************************************
* 函数名         : UartReceiveCommand
* 描述           : 命令接收函数。
* 输入           : 无
* 输出           : 无
* 返回           : 无
*******************************************************************************/
u8 UartReceiveCommand(void)
{
	unsigned char i, j;
	
	//有开始接收命令了
	if(Cmd.ReceivePoint > 0)
	{
		//接收完剩下的数据
		delay_ms(100);
		//去掉命令头
		Cmd.ReceiveBuffer[0] = Cmd.ReceiveBuffer[1];
		//去掉数据中多出的一个7F
		for(i = 2, j = 1; i < Cmd.ReceivePoint; i++)
		{
			//先判断一下这个为7F时，上一个也是7F的话，就是双个7F，就丢弃。
			if(Cmd.ReceiveBuffer[i] == 0x7F && Cmd.ReceiveBuffer[i - 1] == 0x7F){continue;}
			
			Cmd.ReceiveBuffer[j++] = Cmd.ReceiveBuffer[i];
		}
		Cmd.ReceivePoint = 0;
		return 1;
	}
	return 0;
}


///******************* (C) COPYRIGHT 2017 优灵科技 *********END OF FILE********/

