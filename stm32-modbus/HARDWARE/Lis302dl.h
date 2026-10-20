#ifndef __LIS302DL_H
#define __LIS302DL_H

#include "sys.h"

/* LIS302DL 寄存器地址 */
#define WHO_AM_I            0x0F
#define CTRL_REG1           0x20
#define CTRL_REG2           0x21
#define CTRL_REG3           0x22
#define STATUS_REG          0x27
#define FF_WU_CFG_1         0x30
#define FF_WU_THS_1         0x32
#define FF_WU_DURATION_1    0x33
#define OUT_X               0x29
#define OUT_Y               0x2B
#define OUT_Z               0x2D

/* LIS302DL 的 WHO_AM_I 固定值；LIS3DH 是 0x33，不能混用 */
#define LIS302DL_WHO_AM_I   0x3B

/* 7 位地址 0x1C，左移一位后：写 0x38，读 0x39 */
#define LIS302DL_I2C_ADDR_W 0x38
#define LIS302DL_I2C_ADDR_R 0x39

typedef struct
{
    signed short x;
    signed short y;
    signed short z;
} MEMS_TYPEDEF;

/* 以下函数返回 0 表示成功，非 0 表示 I2C 通信失败 */
uint8_t LIS302DL_WriteReg(uint8_t reg, uint8_t value);
uint8_t LIS302DL_ReadReg(uint8_t reg, uint8_t *value);
uint8_t LIS302DL_Check(void);
uint8_t LIS302DL_Config(void);
uint8_t LIS302DL_Init(void);

/*
 * 读取三轴加速度
 * 返回：0=读到新数据，1=本次还没有新数据，2=I2C 错误
 */
uint8_t LIS302DL_Output(MEMS_TYPEDEF *mems);

#endif
