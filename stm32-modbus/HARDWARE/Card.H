#ifndef _CARD_H
#define _CARD_H

/* YL0202 串口帧最大长度：命令层最长 0x7E，这里留 64 字节给常用卡操作 */
#define CARD_CMD_BUF_SIZE       64

typedef struct _CMD
{
    unsigned char ReceiveBuffer[CARD_CMD_BUF_SIZE];
    unsigned char SendBuffer[CARD_CMD_BUF_SIZE];
    volatile unsigned char ReceivePoint;
    unsigned char Code;
} CMD;

typedef struct _CARD
{
    unsigned int  Type;         /* 卡类型：0x0400=S50，0x4400=Ultralight */
    unsigned long UID;          /* 卡号，已按低字节在前转换 */
    unsigned long Value;        /* 充值/扣款后的余额 */
    unsigned char KeyA[6];
    unsigned char KeyB[6];
    unsigned char BlockData[16];
    unsigned char NocardData[5];
    unsigned char LastStatus;   /* 最近一次命令状态 */
    volatile unsigned char NewData; /* 收到有效卡数据后置 1，应用处理完清 0 */
} CARD;

extern CARD Card;
extern CMD Cmd;

void Card_Init(void);
unsigned char Card_Process(void);

void OpenAutoReadCard(void);
void CloseAutoReadCard(void);
void OneKeyReadCard(void);
void OneKeyMakeCard(unsigned char block, unsigned long value,
                    unsigned char *keyA, unsigned char *keyB,
                    unsigned char *keyBOld);
void OneKeyInc(unsigned char block, unsigned char *key, unsigned long value);
void OneKeyDec(unsigned char block, unsigned char *key, unsigned long value);
void OneKeyReadBlock(unsigned char block, unsigned char *keyA);
void OneKeyWriteBlock(unsigned char block, unsigned char *keyB,
                      unsigned char *blockData);
unsigned char CommandProcess(void);

#endif
