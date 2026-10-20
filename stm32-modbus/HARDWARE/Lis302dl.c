/******************************************************************************
 * 文件：Lis302dl.c
 * 功能：LIS302DL 三轴加速度计驱动（软件 I2C）
 *
 * 硬件连接：
 *   PB6 = SCL
 *   PB7 = SDA
 *   7 位 I2C 地址 0x1C（SA0 = 0），写地址 0x38，读地址 0x39
 *
 * 数据格式：
 *   LIS302DL 输出为 8 位有符号数，2g 量程时约 18mg/LSB。
 *   本驱动把原始有符号数直接写入 MEMS_TYPEDEF.x/y/z，
 *   HMI 的震动阈值 48/65/75/90/200 也是按原始 count 设计的。
 ******************************************************************************/

#include "myiic.h"
#include "lIS302DL.h"

/*
 * 写一个寄存器
 * 返回：0 成功，1 失败
 *
 * IIC_Wait_Ack() 失败时内部已经调用 IIC_Stop() 释放总线，
 * 所以这里直接返回，不再重复 Stop，也不继续发送后面的字节。
 */
uint8_t LIS302DL_WriteReg(uint8_t reg, uint8_t value)
{
    IIC_Start();

    IIC_Send_Byte(LIS302DL_I2C_ADDR_W);
    if (IIC_Wait_Ack())
    {
        return 1;
    }

    IIC_Send_Byte(reg);
    if (IIC_Wait_Ack())
    {
        return 1;
    }

    IIC_Send_Byte(value);
    if (IIC_Wait_Ack())
    {
        return 1;
    }

    IIC_Stop();
    return 0;
}

/*
 * 读一个寄存器
 * 返回：0 成功，1 失败
 *
 * 时序：Start -> 写地址 -> 写寄存器地址 -> 重复 Start -> 读地址
 *       -> 读 1 字节 -> NACK -> Stop
 *
 * 注意：IIC_Read_Byte(0) 内部已经发送 NACK，后面不能再调用 IIC_NAck()，
 *       否则会多发一个 SCL 时钟。
 */
uint8_t LIS302DL_ReadReg(uint8_t reg, uint8_t *value)
{
    IIC_Start();

    IIC_Send_Byte(LIS302DL_I2C_ADDR_W);
    if (IIC_Wait_Ack())
    {
        return 1;
    }

    IIC_Send_Byte(reg);
    if (IIC_Wait_Ack())
    {
        return 1;
    }

    IIC_Start();
    IIC_Send_Byte(LIS302DL_I2C_ADDR_R);
    if (IIC_Wait_Ack())
    {
        return 1;
    }

    *value = IIC_Read_Byte(0);      /* ack=0：读完后回 NACK */
    IIC_Stop();
    return 0;
}

/*
 * 在线检测
 *   WHO_AM_I 必须等于 0x3B 才算 LIS302DL。
 *   不能只判断“非 0”，否则读失败返回 0xFF 也会被误判为传感器存在。
 */
uint8_t LIS302DL_Check(void)
{
    uint8_t who = 0;

    if (LIS302DL_ReadReg(WHO_AM_I, &who) != 0)
    {
        return 0;
    }

    return (who == LIS302DL_WHO_AM_I) ? 1u : 0u;
}

/*
 * 初始化传感器
 *   CTRL_REG1 = 0xC7：
 *     DR=1   -> 400Hz 采样
 *     PD=1   -> 正常工作
 *     FS=0   -> ±2g 量程，约 18mg/LSB
 *     Zen/Yen/Xen=1 -> 三轴使能
 *   CTRL_REG2 = 0x00：关闭高通滤波器，保留重力直流分量
 *
 * 下面保留原来的 CTRL_REG3/自由落体配置，避免改变 INT 引脚行为；
 * 当前读取使用轮询，不依赖这些中断。
 * 注意：FF_WU_CFG_1 = 0x10 实际只置了 ZLIE，原注释里写的
 *       AOI/YLIE/XHIE 与寄存器值不一致。
 */
uint8_t LIS302DL_Config(void)
{
    if (LIS302DL_WriteReg(CTRL_REG1, 0xC7) != 0)
    {
        return 1;
    }

    if (LIS302DL_WriteReg(CTRL_REG2, 0x00) != 0)
    {
        return 1;
    }

    if (LIS302DL_WriteReg(CTRL_REG3, 0xC1) != 0)
    {
        return 1;
    }

    if (LIS302DL_WriteReg(FF_WU_THS_1, 0x28) != 0)
    {
        return 1;
    }

    if (LIS302DL_WriteReg(FF_WU_DURATION_1, 40) != 0)
    {
        return 1;
    }

    if (LIS302DL_WriteReg(FF_WU_CFG_1, 0x10) != 0)
    {
        return 1;
    }

    return 0;
}

/*
 * 初始化 I2C、检测芯片并配置传感器
 * 返回：0 成功，1 失败
 */
uint8_t LIS302DL_Init(void)
{
    IIC_Init();

    if (LIS302DL_Check() == 0)
    {
        return 1;
    }

    return LIS302DL_Config();
}

/*
 * 读取三轴加速度
 * 返回：0=有新数据，1=还没有新数据，2=I2C 错误
 *
 * STATUS_REG 的 bit3 是 ZYXDA，三轴新数据有效标志。
 * 只有该位为 1 时才读 OUT_X/OUT_Y/OUT_Z，避免重复读旧数据。
 */
uint8_t LIS302DL_Output(MEMS_TYPEDEF *mems)
{
    uint8_t status = 0;
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t z = 0;

    if (LIS302DL_ReadReg(STATUS_REG, &status) != 0)
    {
        return 2;
    }

    if ((status & 0x08) == 0)
    {
        return 1;
    }

    if (LIS302DL_ReadReg(OUT_X, &x) != 0)
    {
        return 2;
    }

    if (LIS302DL_ReadReg(OUT_Y, &y) != 0)
    {
        return 2;
    }

    if (LIS302DL_ReadReg(OUT_Z, &z) != 0)
    {
        return 2;
    }

    /* 输出寄存器是 8 位二进制补码，转成 int8_t 后再赋给 s16 完成符号扩展 */
    mems->x = (int8_t)x;
    mems->y = (int8_t)y;
    mems->z = (int8_t)z;

    return 0;
}
