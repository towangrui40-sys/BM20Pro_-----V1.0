/******************************************************************************
 * 文件：Card.c
 * 功能：YL0202 读卡模块 UART 协议
 *
 * 硬件：
 *   USART3，PA10/PC11 重映射？本工程使用 PC10=TX、PC11=RX
 *   9600，8 数据位，无校验，1 停止位
 *
 * 协议：
 *   传输层：7F | 长度 | 命令码 | 参数2 | 校验
 *   参数1 中出现 7F 时，发送两遍 7F；接收后还原。
 *   校验 = 长度 XOR 命令码 XOR 参数1（不包含帧头和重复 7F）。
 ******************************************************************************/

#include "Card.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"
#include <string.h>

CARD Card;
CMD Cmd;

static unsigned char CheckSum(unsigned char *dat, unsigned char num);
static void UartSendCommand(u8 *buff, u8 cnt);
static u8 UartReceiveCommand(void);

void Card_Init(void)
{
    memset(&Card, 0, sizeof(Card));
    memset(&Cmd, 0, sizeof(Cmd));

    uart3_init(9600);
    delay_ms(300);

    /* 默认开启自动读卡，模块检测到卡后主动上报 */
    OpenAutoReadCard();
}

unsigned char Card_Process(void)
{
    unsigned char status;

    status = CommandProcess();
    Card.LastStatus = status;

    /* 只有真正的卡片命令才置新数据标志，0xAB/0xAC 的应答不算 */
    if ((status == 0x00) && (Cmd.Code >= 0x10) && (Cmd.Code <= 0x15))
    {
        Card.NewData = 1;
    }

    return status;
}

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

void OneKeyReadCard(void)
{
    unsigned char len = 2;

    Cmd.SendBuffer[0] = len;
    Cmd.SendBuffer[1] = 0x10;
    Cmd.SendBuffer[2] = CheckSum(Cmd.SendBuffer, len);
    UartSendCommand(Cmd.SendBuffer, len + 1);
}

void OneKeyMakeCard(unsigned char block, unsigned long value,
                    unsigned char *keyA, unsigned char *keyB,
                    unsigned char *keyBOld)
{
    unsigned char i, len = 25;

    Cmd.SendBuffer[0] = len;
    Cmd.SendBuffer[1] = 0x11;
    Cmd.SendBuffer[2] = block;
    Cmd.SendBuffer[3] = (unsigned char)(value >> 24);
    Cmd.SendBuffer[4] = (unsigned char)(value >> 16);
    Cmd.SendBuffer[5] = (unsigned char)(value >> 8);
    Cmd.SendBuffer[6] = (unsigned char)value;
    for (i = 0; i < 6; i++)
    {
        Cmd.SendBuffer[7 + i] = keyA[i];
        Cmd.SendBuffer[13 + i] = keyB[i];
        Cmd.SendBuffer[19 + i] = keyBOld[i];
    }
    Cmd.SendBuffer[25] = CheckSum(Cmd.SendBuffer, len);
    UartSendCommand(Cmd.SendBuffer, len + 1);
}

void OneKeyInc(unsigned char block, unsigned char *key, unsigned long value)
{
    unsigned char i, len = 13;

    Cmd.SendBuffer[0] = len;
    Cmd.SendBuffer[1] = 0x12;
    Cmd.SendBuffer[2] = block;
    for (i = 0; i < 6; i++)
    {
        Cmd.SendBuffer[3 + i] = key[i];
    }
    Cmd.SendBuffer[9] = (unsigned char)(value >> 24);
    Cmd.SendBuffer[10] = (unsigned char)(value >> 16);
    Cmd.SendBuffer[11] = (unsigned char)(value >> 8);
    Cmd.SendBuffer[12] = (unsigned char)value;
    Cmd.SendBuffer[13] = CheckSum(Cmd.SendBuffer, len);
    UartSendCommand(Cmd.SendBuffer, len + 1);
}

void OneKeyDec(unsigned char block, unsigned char *key, unsigned long value)
{
    unsigned char i, len = 13;

    Cmd.SendBuffer[0] = len;
    Cmd.SendBuffer[1] = 0x13;
    Cmd.SendBuffer[2] = block;
    for (i = 0; i < 6; i++)
    {
        Cmd.SendBuffer[3 + i] = key[i];
    }
    Cmd.SendBuffer[9] = (unsigned char)(value >> 24);
    Cmd.SendBuffer[10] = (unsigned char)(value >> 16);
    Cmd.SendBuffer[11] = (unsigned char)(value >> 8);
    Cmd.SendBuffer[12] = (unsigned char)value;
    Cmd.SendBuffer[13] = CheckSum(Cmd.SendBuffer, len);
    UartSendCommand(Cmd.SendBuffer, len + 1);
}

void OneKeyReadBlock(unsigned char block, unsigned char *keyA)
{
    unsigned char i, len = 9;

    Cmd.SendBuffer[0] = len;
    Cmd.SendBuffer[1] = 0x14;
    Cmd.SendBuffer[2] = block;
    for (i = 0; i < 6; i++)
    {
        Cmd.SendBuffer[3 + i] = keyA[i];
    }
    Cmd.SendBuffer[9] = CheckSum(Cmd.SendBuffer, len);
    UartSendCommand(Cmd.SendBuffer, len + 1);
}

void OneKeyWriteBlock(unsigned char block, unsigned char *keyB,
                      unsigned char *blockData)
{
    unsigned char i, len = 25;

    Cmd.SendBuffer[0] = len;
    Cmd.SendBuffer[1] = 0x15;
    Cmd.SendBuffer[2] = block;
    for (i = 0; i < 6; i++)
    {
        Cmd.SendBuffer[3 + i] = keyB[i];
    }
    for (i = 0; i < 16; i++)
    {
        Cmd.SendBuffer[9 + i] = blockData[i];
    }
    Cmd.SendBuffer[25] = CheckSum(Cmd.SendBuffer, len);
    UartSendCommand(Cmd.SendBuffer, len + 1);
}

unsigned char CommandProcess(void)
{
    unsigned char i, status, len;

    if (UartReceiveCommand() == 0)
    {
        return 0xFD;
    }

    len = Cmd.ReceiveBuffer[0];
    if ((len < 3) || (len >= CARD_CMD_BUF_SIZE))
    {
        return 0xFB;
    }

    if (Cmd.ReceiveBuffer[len] != CheckSum(Cmd.ReceiveBuffer, len))
    {
        return 0xFB;
    }

    Cmd.Code = Cmd.ReceiveBuffer[1];
    status = Cmd.ReceiveBuffer[2];

    /* 状态、卡类型、卡号在状态错误时也可能存在，先保存再判断 */
    if (len >= 9)
    {
        Card.Type = ((unsigned int)Cmd.ReceiveBuffer[3] << 8) |
                    Cmd.ReceiveBuffer[4];
        Card.UID = Cmd.ReceiveBuffer[8];
        Card.UID <<= 8;
        Card.UID |= Cmd.ReceiveBuffer[7];
        Card.UID <<= 8;
        Card.UID |= Cmd.ReceiveBuffer[6];
        Card.UID <<= 8;
        Card.UID |= Cmd.ReceiveBuffer[5];
    }

    if (status != 0x00)
    {
        return status;
    }

    switch (Cmd.Code)
    {
    case 0x10:
    case 0x11:
    case 0x15:
        break;

    case 0x12:
    case 0x13:
        if (len >= 13)
        {
            Card.Value = Cmd.ReceiveBuffer[9];
            Card.Value <<= 8;
            Card.Value |= Cmd.ReceiveBuffer[10];
            Card.Value <<= 8;
            Card.Value |= Cmd.ReceiveBuffer[11];
            Card.Value <<= 8;
            Card.Value |= Cmd.ReceiveBuffer[12];
        }
        break;

    case 0x14:
        if (len >= 25)
        {
            for (i = 0; i < 16; i++)
            {
                Card.BlockData[i] = Cmd.ReceiveBuffer[9 + i];
            }
        }
        break;

    default:
        break;
    }

    return status;
}

static unsigned char CheckSum(unsigned char *dat, unsigned char num)
{
    unsigned char sum = 0, i;

    for (i = 0; i < num; i++)
    {
        sum ^= dat[i];
    }
    return sum;
}

static void UartSendCommand(u8 *buff, u8 cnt)
{
    u8 i;

    USART_SendData(USART3, 0x7F);
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);

    for (i = 0; i < cnt; i++)
    {
        USART_SendData(USART3, buff[i]);
        while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);

        if (buff[i] == 0x7F)
        {
            USART_SendData(USART3, 0x7F);
            while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
        }
    }
}

static u8 UartReceiveCommand(void)
{
    unsigned char i, j, raw;

    if (Cmd.ReceivePoint == 0)
    {
        return 0;
    }

    /* 模块从收到首字节到发完最长帧约几十毫秒，等 100ms 保证整帧收完 */
    delay_ms(100);

    USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
    raw = Cmd.ReceivePoint;

    if ((raw < 3) || (Cmd.ReceiveBuffer[0] != 0x7F))
    {
        Cmd.ReceivePoint = 0;
        USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
        return 0;
    }

    /* 去掉帧头，得到命令层 */
    Cmd.ReceiveBuffer[0] = Cmd.ReceiveBuffer[1];
    j = 1;
    for (i = 2; i < raw; i++)
    {
        if ((Cmd.ReceiveBuffer[i] == 0x7F) &&
            (Cmd.ReceiveBuffer[i - 1] == 0x7F))
        {
            continue;
        }

        if (j < CARD_CMD_BUF_SIZE)
        {
            Cmd.ReceiveBuffer[j++] = Cmd.ReceiveBuffer[i];
        }
    }

    Cmd.ReceivePoint = 0;
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    return 1;
}
