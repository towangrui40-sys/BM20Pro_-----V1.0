/******************************************************************************
 * 文件：Vfd_Ctrl.c
 * 功能：欧瑞 E2000 变频器 Modbus 控制
 *
 * 从站地址：0x02
 * 写入（保持寄存器，功能码 06）：
 *   0x2000  <- sys.ctrl->set_motor  启停命令
 *   0x010D  <- sys.ctrl->set_speed  速度设定
 *
 * 读取（保持寄存器，功能码 03）：
 *   0x1000 起 9 个寄存器：
 *     索引 2 = 输出电流
 *     索引 5 = 运行状态
 *   0x1017 起 1 个寄存器：
 *     实际速度/频率
 *
 * 所有 Modbus 请求都在 vfd_task 任务里执行，不能放到中断里。
 ******************************************************************************/

#include "includes.h"
#include "mb.h"
#include "mb_m.h"
#include "user_mb_app.h"
#include "Vfd_Ctrl.h"

extern SysDataDef sys;

/* 上一次成功写入的值；0xFFFF 表示还没有写过 */
static uint16_t s_last_motor_cmd = 0xFFFFu;
static uint16_t s_last_speed_cmd = 0xFFFFu;

/* 轮询节拍：0=写命令，1=读状态区，2=读实际速度 */
static uint8_t s_vfd_tick = 0;

void Vfd_Init(void)
{
    s_last_motor_cmd = 0xFFFFu;
    s_last_speed_cmd = 0xFFFFu;
    s_vfd_tick = 0;
}

/*
 * 变频器周期处理，vfd_task 每 VFD_POLL_MS 调用一次
 *
 * 读和写分在不同节拍，避免一个周期内总线占用时间过长。
 * 写失败时保留上一次成功的值，下个周期会自动重试。
 */
void Vfd_Process(void)
{
    uint16_t motor_cmd;
    uint16_t speed_cmd;

    motor_cmd = (uint16_t)sys.ctrl->set_motor;
    speed_cmd = (uint16_t)sys.ctrl->set_speed;

    switch (s_vfd_tick)
    {
    case 0:
        /* 启停命令只在变化时写，避免每个周期重复发 */
        if (motor_cmd != s_last_motor_cmd)
        {
            if (eMBMasterReqWriteHoldingRegister(
                    VFD_SLAVE_ADDR, VFD_REG_MOTOR_CMD, motor_cmd,
                    VFD_WRITE_TIMEOUT_TICKS) == MB_MRE_NO_ERR)
            {
                s_last_motor_cmd = motor_cmd;
            }
        }

        /* 速度设定只在变化时写 */
        if (speed_cmd != s_last_speed_cmd)
        {
            if (eMBMasterReqWriteHoldingRegister(
                    VFD_SLAVE_ADDR, VFD_REG_SPEED_CMD, speed_cmd,
                    VFD_WRITE_TIMEOUT_TICKS) == MB_MRE_NO_ERR)
            {
                s_last_speed_cmd = speed_cmd;
            }
        }
        break;

    case 1:
        /* 读 0x1000~0x1008：运行状态和输出电流 */
        if (eMBMasterReqReadHoldingRegister(
                VFD_SLAVE_ADDR, VFD_REG_STATUS_BASE, VFD_REG_STATUS_COUNT,
                VFD_READ_TIMEOUT_TICKS) == MB_MRE_NO_ERR)
        {
            sys.ctrl->motor_state =
                (short)usMRegHoldBuf[VFD_SLAVE_ADDR - 1u][VFD_IDX_STATE];
            sys.ctrl->motor_current =
                (short)usMRegHoldBuf[VFD_SLAVE_ADDR - 1u][VFD_IDX_CURRENT];
        }
        break;

    case 2:
        /* 读 0x1017：实际速度/频率 */
        if (eMBMasterReqReadHoldingRegister(
                VFD_SLAVE_ADDR, VFD_REG_SPEED, 1u,
                VFD_READ_TIMEOUT_TICKS) == MB_MRE_NO_ERR)
        {
            sys.ctrl->read_speed =
                (short)usMRegHoldBuf[VFD_SLAVE_ADDR - 1u][VFD_IDX_READ_SPEED];
        }
        break;

    default:
        s_vfd_tick = 0;
        break;
    }

    s_vfd_tick++;
    if (s_vfd_tick >= 3u)
    {
        s_vfd_tick = 0;
    }
}
