#ifndef __VFD_CTRL_H
#define __VFD_CTRL_H

/* 变频器 Modbus 从站地址 */
#define VFD_SLAVE_ADDR              2u

/* 变频器寄存器地址（保持寄存器，功能码 03/06） */
#define VFD_REG_STATUS_BASE         0x1000u
#define VFD_REG_STATUS_COUNT        9u
#define VFD_REG_SPEED               0x1017u
#define VFD_REG_MOTOR_CMD           0x2000u
#define VFD_REG_SPEED_CMD           0x010Du

/*
 * 主站回调的地址映射：
 * 变频器读寄存器从 0x1000 开始，而 usMRegHoldBuf[1] 从索引 0 开始，
 * 所以把 0x1000~0x101F 映射到 usMRegHoldBuf[1][0~0x1F]。
 */
#define VFD_READ_WINDOW_BASE        0x1000u
#define VFD_READ_WINDOW_SIZE        0x20u

/* 状态数据在 usMRegHoldBuf[1] 中的索引 */
#define VFD_IDX_CURRENT             2u          /* 0x1002：输出电流 */
#define VFD_IDX_STATE               5u          /* 0x1005：运行状态 */
#define VFD_IDX_READ_SPEED          0x17u       /* 0x1017 - 0x1000 */

/* 任务轮询周期和 Modbus 超时，单位：tick（200 tick/s） */
#define VFD_POLL_MS                 50u
#define VFD_READ_TIMEOUT_TICKS      100u
#define VFD_WRITE_TIMEOUT_TICKS     100u

void Vfd_Init(void);
void Vfd_Process(void);

#endif
