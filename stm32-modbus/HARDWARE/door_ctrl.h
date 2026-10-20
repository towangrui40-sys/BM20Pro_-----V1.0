#ifndef __DOOR_CTRL_H_
#define __DOOR_CTRL_H_

#include "stm32f10x.h"

/* -------------------------------------------------------------------------
 * 盖子直流电机
 * ---------------------------------------------------------------------- */
/* PA0：开盖到位传感器；PA1：关盖到位传感器，均配置为上拉输入 */
#define DOOR_OPEN_LIMIT_GPIO    GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)
#define DOOR_CLOSE_LIMIT_GPIO   GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1)

/*
 * 限位有效电平
 *   1 表示传感器输出高电平时机构到位（当前按原 Door_Unlock/Door_Lock
 *   的停机逻辑统一为 1）。
 *   如果实测为低有效，只需要把这里改成 0，门控制逻辑会全部跟着变。
 */
#define DOOR_LIMIT_ACTIVE_LEVEL 1

#define DOOR_DEFAULT_SPEED      5u      /* 默认速度：直接写入 TIM3 CCR */
#define PWM_OFF_LEVEL           100u    /* CCR=100 时 PWM 输出常低，电机停止 */

/* HMI 写入 ctrl->set_case 的盖子命令 */
typedef enum
{
    DOOR_CMD_NONE = 0,
    DOOR_CMD_OPEN = 1,
    DOOR_CMD_CLOSE = 2
} DoorCommand;

/* 盖子内部状态，用于 20ms 周期的电机状态机 */
typedef enum
{
    DOOR_STATE_UNKNOWN = 0,
    DOOR_STATE_CLOSED,
    DOOR_STATE_OPEN,
    DOOR_STATE_OPENING,
    DOOR_STATE_CLOSING
} DoorState;

/* 盖子电机运行数据 */
typedef struct
{
    uint8_t  state;         /* 当前状态，取值见 DoorState */
    uint8_t  open_limit;    /* 开盖限位原始电平 */
    uint8_t  close_limit;   /* 关盖限位原始电平 */
    uint8_t  open_speed;    /* 开盖速度，TIM3 CCR 值 */
    uint8_t  close_speed;   /* 关盖速度，TIM3 CCR 值 */
    uint32_t move_start;    /* 本次动作开始时的 OSTimeGet() tick */
} DoorDataDef;

/* -------------------------------------------------------------------------
 * 研磨罐锁，外部闭环步进驱动器（Modbus 从站）
 * ---------------------------------------------------------------------- */
#define TANK_POLL_MS            50u     /* 锁任务轮询周期，单位 ms */

/* HMI 写入 ctrl->set_lock 的罐锁命令 */
typedef enum
{
    TANK_CMD_NONE = 0,
    TANK_CMD_UNLOCK = 1,
    TANK_CMD_LOCK = 2,
    TANK_CMD_LOCK_TO_RUN = 3,
    TANK_CMD_RUN_TO_LOCK = 4
} TankCommand;

/* 发布给 HMI 的 ctrl->lock_state 状态值 */
typedef enum
{
    TANK_STATE_UNLOCKED = 0,
    TANK_STATE_LOCKED = 1,
    TANK_STATE_RUN_POSITION = 2
} TankState;

/* 罐锁内部状态机 */
typedef enum
{
    TANK_FSM_IDLE = 0,
    TANK_FSM_SEND,
    TANK_FSM_WAIT_DONE
} TankFsm;

/* 步进驱动器输入寄存器 0x0000 的电机状态 */
typedef enum
{
    MOTOR_STATE_STOPPED = 0,
    MOTOR_STATE_COMPLETE = 1,
    MOTOR_STATE_EXECUTING = 2,
    MOTOR_STATE_OVERLOAD = 3,
    MOTOR_STATE_STALL = 4
} MotorState;

/* 内部故障码，便于调试时观察锁任务失败原因 */
typedef enum
{
    TANK_FAULT_NONE = 0,
    TANK_FAULT_OVERLOAD,
    TANK_FAULT_STALL,
    TANK_FAULT_TIMEOUT,
    TANK_FAULT_COMM
} TankFault;

/* 罐锁任务运行数据 */
typedef struct
{
    uint8_t  fsm;           /* 内部状态机，取值见 TankFsm */
    uint8_t  active_cmd;    /* 当前正在执行的 TankCommand */
    uint8_t  motor_state;   /* 驱动器返回的 MotorState */
    uint8_t  lock_state;    /* 发布给 HMI 的 TankState */
    uint8_t  fault;         /* 最近一次故障，取值见 TankFault */
    uint8_t  saw_moving;    /* 是否已经看到过“执行中”状态 */
    uint8_t  status_valid;  /* 最近一次输入寄存器读取是否成功 */
    uint8_t  retry;         /* 发送失败重试计数 */
    uint32_t deadline;      /* 命令总超时时间，OSTimeGet() tick */
    uint32_t guard_time;    /* 此 tick 之前不接受“任务完成”，防旧状态误判 */
} LockDataDef;

/* 初始化 TIM3 PWM、门限位、轮盘霍尔和门/锁数据结构 */
void Motor_Init(DoorDataDef *door, LockDataDef *lock);

/* 由 monitor_task 每 20ms 调用，处理盖子电机 */
void Door_Process(DoorDataDef *door);

/*
 * 由 tank_lock_task 调用，处理研磨罐锁。
 * 内部会执行阻塞式 Modbus 主站请求，严禁在中断中调用。
 */
void Lock_Process(LockDataDef *lock);

#endif
