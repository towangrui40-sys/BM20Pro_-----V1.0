/******************************************************************************
 * 文件：door_ctrl.c
 * 功能：电机控制
 *   1. 盖子直流电机：TIM3 CH1/CH2 双路 PWM + 开/关到位传感器
 *   2. 研磨罐锁：Modbus RTU 主站控制外部闭环步进驱动器（从站 1）
 *
 * 任务划分：
 *   - Door_Process()：由 monitor_task 每 20ms 调用，只做本地直流电机控制
 *   - Lock_Process()：由 tank_lock_task 每 50ms 调用，内部有阻塞式 Modbus
 *                     请求，严禁在定时器中断里调用
 *
 * 外部驱动器寄存器（E-Drive42 说明书）：
 *   0x0000      电机状态：0 停止，1 任务完成，2 任务执行中，3 过载，4 堵转
 *   0x0001~2    绝对位置，int32，51200 计数/圈
 *   0x0029      启动归零；锁紧机构靠归零压到机械限位实现锁紧
 ******************************************************************************/

#include "includes.h"
#include "mb.h"
#include "mb_m.h"
#include "user_mb_app.h"

extern SysDataDef sys;

/* -------------------------------------------------------------------------
 * 外部步进驱动器（从站 1）参数
 * ---------------------------------------------------------------------- */
#define TANK_SLAVE_ADDR             1u          /* 驱动器 Modbus 从站地址 */
#define TANK_REG_ABS_POSITION       0x0001u     /* 绝对位置寄存器，占用 2 个字 */
#define TANK_REG_HOMING             0x0029u     /* 启动归零寄存器 */

/* 三个机械位置的绝对位置值，单位：编码器计数（51200 计数/圈） */
#define TANK_POS_UNLOCK             4890000L    /* 解锁位置 */
#define TANK_POS_RUN                (-2620000L) /* 锁紧位转到运行位 */
#define TANK_POS_LOCK               0L          /* 运行位回到锁紧位 */

/* Modbus 超时参数，单位：OS tick（当前 200 tick/s，1 tick = 5ms） */
#define TANK_READ_TIMEOUT_TICKS     50u     /* 250ms，读输入寄存器 */
#define TANK_WRITE_TIMEOUT_TICKS    100u    /* 500ms，写保持寄存器 */
#define TANK_CMD_TIMEOUT_TICKS      4000u   /* 20s，一次机械动作的总超时 */
#define TANK_START_GUARD_TICKS      100u    /* 500ms，防止把旧“完成”状态当成新任务完成 */
#define TANK_MAX_RETRIES            2u      /* 发送失败后的重试次数 */

/* 盖子电机一次开/关动作的超时，2000 tick = 10s */
#define DOOR_MOVE_TIMEOUT_TICKS     2000u

/* 盖子电机转向，仅本文件内部使用 */
typedef enum
{
    DOOR_DIR_STOP = 0,
    DOOR_DIR_OPEN,
    DOOR_DIR_CLOSE
} DoorDirection;

static void TIM3_PWM_Init(u16 arr, u16 psc);
static void Door_Motor_Run(DoorDirection direction, uint8_t speed);
static int  Tank_ReadStatus(LockDataDef *lock);
static int  Tank_SendCommand(uint8_t command);
static int  Tank_SendPosition(int32_t position);
static uint8_t Tank_CommandToState(uint8_t command);

/* -------------------------------------------------------------------------
 * 硬件初始化
 * ---------------------------------------------------------------------- */
/*
 * TIM3 PWM 初始化
 *   arr = 99，psc = 71：72MHz / 72 / 100 = 10kHz
 *   PWM1 + 低有效：CCR = 100 时输出常低；CCR = n 时输出低 n%、高 (100-n)%
 *   更新中断只留给 TIM3_IRQHandler 采样轮盘霍尔，不在这里跑电机状态机
 */
static void TIM3_PWM_Init(u16 arr, u16 psc)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef       TIM_OCInitStructure;
    NVIC_InitTypeDef        NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    TIM_TimeBaseStructure.TIM_Period = arr;
    TIM_TimeBaseStructure.TIM_Prescaler = psc;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;

    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
    TIM_OC2Init(TIM3, &TIM_OCInitStructure);
    TIM_OC3Init(TIM3, &TIM_OCInitStructure);
    TIM_OC4Init(TIM3, &TIM_OCInitStructure);

    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM3, ENABLE);
    TIM_ClearFlag(TIM3, TIM_FLAG_Update);
    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);

    /*
     * 更新中断仍然给 TIM3_IRQHandler 采样轮盘霍尔使用。
     * 注意：门锁状态机和 Modbus 请求都已经移出中断，不能在这里恢复。
     */
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 4;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM3, ENABLE);
}

/*
 * 电机控制总初始化
 *   - 配置电机 PWM 引脚、门限位输入、轮盘霍尔输入
 *   - 初始化 TIM3 PWM
 *   - 清空门/锁数据结构，并根据当前限位判断盖子初始状态
 */
void Motor_Init(DoorDataDef *door, LockDataDef *lock)
{
    memset(door, 0, sizeof(*door));
    memset(lock, 0, sizeof(*lock));

    Init_Motor_Gpio();          /* PC6~PC9 复用为 TIM3 PWM 输出 */
    Init_Door_State_Gpio();     /* PA0/PA1 门开/关到位输入 */
    Init_Wheel_Gpio();          /* 轮盘霍尔输入，供 TIM3 中断采样 */
    TIM3_PWM_Init(100u - 1u, 72u - 1u);     /* 10kHz PWM */

    Door_Motor_Run(DOOR_DIR_STOP, 0);       /* 上电先保证电机停止 */

    /* 默认速度值直接写入 TIM3 CCR，数值越小电机越慢 */
    door->open_speed = DOOR_DEFAULT_SPEED;
    door->close_speed = DOOR_DEFAULT_SPEED;

    /* 读取当前限位，决定上电时盖子处于开、关还是中间位置 */
    door->open_limit = DOOR_OPEN_LIMIT_GPIO;
    door->close_limit = DOOR_CLOSE_LIMIT_GPIO;

    if (door->open_limit == DOOR_LIMIT_ACTIVE_LEVEL)
    {
        door->state = DOOR_STATE_OPEN;
    }
    else if (door->close_limit == DOOR_LIMIT_ACTIVE_LEVEL)
    {
        door->state = DOOR_STATE_CLOSED;
    }
    else
    {
        door->state = DOOR_STATE_UNKNOWN;
    }

    /* 罐锁上电默认按“未锁”处理，等驱动器状态和 HMI 命令再更新 */
    lock->fsm = TANK_FSM_IDLE;
    lock->lock_state = TANK_STATE_UNLOCKED;
    lock->fault = TANK_FAULT_NONE;
}

/* -------------------------------------------------------------------------
 * 盖子直流电机
 * ---------------------------------------------------------------------- */
/*
 * 驱动盖子直流电机
 *   direction = DOOR_DIR_OPEN  ：CH1/PC6 输出 PWM，CH2/PC7 拉低
 *   direction = DOOR_DIR_CLOSE ：CH2/PC7 输出 PWM，CH1/PC6 拉低
 *   direction = DOOR_DIR_STOP  ：两路都写 100，输出常低，电机停止
 *
 * 关键点：换向前必须先把另一路复位到 100。
 * 否则正转时 CH2 的 CCR 还留着 5，反转时 CH1 也写 5，两路占空比相同，
 * 电机两端没有压差，表现为“给了命令但不动”。
 */
static void Door_Motor_Run(DoorDirection direction, uint8_t speed)
{
    if (speed >= PWM_OFF_LEVEL)
    {
        speed = PWM_OFF_LEVEL - 1u;
    }

    switch (direction)
    {
    case DOOR_DIR_OPEN:         /* 开盖：CCW，CH1 / PC6 输出 PWM */
        TIM_SetCompare2(TIM3, PWM_OFF_LEVEL);
        TIM_SetCompare1(TIM3, speed);
        break;

    case DOOR_DIR_CLOSE:        /* 关盖：CW，CH2 / PC7 输出 PWM */
        TIM_SetCompare1(TIM3, PWM_OFF_LEVEL);
        TIM_SetCompare2(TIM3, speed);
        break;

    default:
        TIM_SetCompare1(TIM3, PWM_OFF_LEVEL);
        TIM_SetCompare2(TIM3, PWM_OFF_LEVEL);
        break;
    }
}

/*
 * 盖子电机周期处理，monitor_task 每 20ms 调用一次
 *
 * 处理顺序：
 *   1. 取走 HMI 写入的 set_case 命令（1 开盖，2 关盖）
 *   2. 采样开/关到位传感器
 *   3. 根据命令和当前状态启动、继续或停止电机
 *   4. 把 0/1/2 的盖子状态发布给 HMI
 */
void Door_Process(DoorDataDef *door)
{
    uint8_t  command;
    uint32_t now;

    /* set_case 只锁存一次，读完立即清零，避免同一条命令重复执行 */
    command = (uint8_t)sys.ctrl->set_case;
    sys.ctrl->set_case = 0;

    /* 每次处理都重新采样限位，状态机以最新硬件状态为准 */
    door->open_limit = DOOR_OPEN_LIMIT_GPIO;
    door->close_limit = DOOR_CLOSE_LIMIT_GPIO;
    now = OSTimeGet();

    /* 新命令优先：开盖命令直接切到 OPENING，关盖命令直接切到 CLOSING */
    if (command == DOOR_CMD_OPEN)
    {
        if (door->open_limit == DOOR_LIMIT_ACTIVE_LEVEL)
        {
            door->state = DOOR_STATE_OPEN;
        }
        else
        {
            door->state = DOOR_STATE_OPENING;
            door->move_start = now;
        }
    }
    else if (command == DOOR_CMD_CLOSE)
    {
        if (door->close_limit == DOOR_LIMIT_ACTIVE_LEVEL)
        {
            door->state = DOOR_STATE_CLOSED;
        }
        else
        {
            door->state = DOOR_STATE_CLOSING;
            door->move_start = now;
        }
    }

    /*
     * 状态机：
     *   OPENING：一直开，直到开到位或超时
     *   CLOSING：一直关，直到关到位或超时
     *   其它    ：保持停止，并根据限位修正状态
     */
    switch (door->state)
    {
    case DOOR_STATE_OPENING:
        if (door->open_limit == DOOR_LIMIT_ACTIVE_LEVEL)
        {
            Door_Motor_Run(DOOR_DIR_STOP, 0);
            door->state = DOOR_STATE_OPEN;
        }
        /* 10s 还没到开到位，认为限位或机构异常，停机并回到未知状态 */
        else if ((uint32_t)(now - door->move_start) > DOOR_MOVE_TIMEOUT_TICKS)
        {
            Door_Motor_Run(DOOR_DIR_STOP, 0);
            door->state = DOOR_STATE_UNKNOWN;
        }
        else
        {
            Door_Motor_Run(DOOR_DIR_OPEN, door->open_speed);
        }
        break;

    case DOOR_STATE_CLOSING:
        if (door->close_limit == DOOR_LIMIT_ACTIVE_LEVEL)
        {
            Door_Motor_Run(DOOR_DIR_STOP, 0);
            door->state = DOOR_STATE_CLOSED;
        }
        /* 10s 还没到关到位，认为限位或机构异常，停机并回到未知状态 */
        else if ((uint32_t)(now - door->move_start) > DOOR_MOVE_TIMEOUT_TICKS)
        {
            Door_Motor_Run(DOOR_DIR_STOP, 0);
            door->state = DOOR_STATE_UNKNOWN;
        }
        else
        {
            Door_Motor_Run(DOOR_DIR_CLOSE, door->close_speed);
        }
        break;

    default:
        Door_Motor_Run(DOOR_DIR_STOP, 0);
        if (door->open_limit == DOOR_LIMIT_ACTIVE_LEVEL)
        {
            door->state = DOOR_STATE_OPEN;
        }
        else if (door->close_limit == DOOR_LIMIT_ACTIVE_LEVEL)
        {
            door->state = DOOR_STATE_CLOSED;
        }
        else
        {
            door->state = DOOR_STATE_UNKNOWN;
        }
        break;
    }

    /* 发布给 HMI：1 开到位，0 关到位，2 中间位置 */
    if (door->open_limit == DOOR_LIMIT_ACTIVE_LEVEL)
    {
        sys.ctrl->case_state = 1;       /* 开到位 */
    }
    else if (door->close_limit == DOOR_LIMIT_ACTIVE_LEVEL)
    {
        sys.ctrl->case_state = 0;       /* 关到位 */
    }
    else
    {
        sys.ctrl->case_state = 2;       /* 中间位置 */
    }
}

/* -------------------------------------------------------------------------
 * 研磨罐锁，外部步进驱动器
 * ---------------------------------------------------------------------- */
/*
 * 读取驱动器电机状态
 *   输入寄存器 0x0000：0 停止，1 完成，2 执行中，3 过载，4 堵转
 *
 * 这里只读 1 个寄存器。原代码读 14 个；说明书示例只读 0x000A 个寄存器，
 * 寄存器表最大到 0x000E，读多了可能被驱动器判为非法地址。
 *
 * 读失败时 status_valid 清 0，Lock_Process 不会拿上一次的旧状态做判断。
 */
static int Tank_ReadStatus(LockDataDef *lock)
{
    if (eMBMasterReqReadInputRegister(TANK_SLAVE_ADDR, 0x0000u, 1u,
                                      TANK_READ_TIMEOUT_TICKS) != MB_MRE_NO_ERR)
    {
        lock->status_valid = 0;
        return -1;
    }

    lock->motor_state = (uint8_t)usMRegInBuf[TANK_SLAVE_ADDR - 1u][0];
    lock->status_valid = 1;
    return 0;
}

/*
 * 写绝对位置
 *   位置是 int32，Modbus 寄存器是 16 位，需要拆成高 16 位和低 16 位。
 *   负数先强制转成 uint32_t，再拆字，得到标准的二进制补码表示。
 *   寄存器顺序：0x0001 = 高字，0x0002 = 低字。
 */
static int Tank_SendPosition(int32_t position)
{
    USHORT   regs[2];
    uint32_t raw;

    raw = (uint32_t)position;
    regs[0] = (USHORT)(raw >> 16);
    regs[1] = (USHORT)(raw & 0xFFFFu);

    if (eMBMasterReqWriteMultipleHoldingRegister(TANK_SLAVE_ADDR,
                                                TANK_REG_ABS_POSITION,
                                                2u, regs,
                                                TANK_WRITE_TIMEOUT_TICKS) != MB_MRE_NO_ERR)
    {
        return -1;
    }

    return 0;
}

/*
 * 把 HMI 命令翻译成驱动器动作
 *   1 解锁        -> 绝对位置 4,890,000
 *   2 锁紧        -> 0x0029 写 1，启动归零，压到机械限位
 *   3 锁紧位转运行 -> 绝对位置 -2,620,000
 *   4 运行位回锁紧 -> 绝对位置 0
 */
static int Tank_SendCommand(uint8_t command)
{
    switch (command)
    {
    case TANK_CMD_UNLOCK:
        return Tank_SendPosition(TANK_POS_UNLOCK);

    case TANK_CMD_LOCK:
        /*
         * 锁紧不是普通位置命令，而是让驱动器归零：
         * 机构压到机械限位后，驱动器把当前位置记为 0，同时完成锁紧。
         */
        if (eMBMasterReqWriteHoldingRegister(TANK_SLAVE_ADDR, TANK_REG_HOMING,
                                             1u,
                                             TANK_WRITE_TIMEOUT_TICKS) != MB_MRE_NO_ERR)
        {
            return -1;
        }
        return 0;

    case TANK_CMD_LOCK_TO_RUN:
        return Tank_SendPosition(TANK_POS_RUN);

    case TANK_CMD_RUN_TO_LOCK:
        return Tank_SendPosition(TANK_POS_LOCK);

    default:
        return -1;
    }
}

/*
 * 命令执行完成后，转换成 HMI 使用的 lock_state
 *   0 未锁，1 已锁，2 运行位
 */
static uint8_t Tank_CommandToState(uint8_t command)
{
    switch (command)
    {
    case TANK_CMD_UNLOCK:
        return TANK_STATE_UNLOCKED;

    case TANK_CMD_LOCK:
    case TANK_CMD_RUN_TO_LOCK:
        return TANK_STATE_LOCKED;

    case TANK_CMD_LOCK_TO_RUN:
        return TANK_STATE_RUN_POSITION;

    default:
        return TANK_STATE_UNLOCKED;
    }
}

/*
 * 研磨罐锁周期处理，tank_lock_task 每 50ms 调用一次
 *
 * 重要：本函数内部会调用 FreeModbus 主站的阻塞接口，必须在任务里执行。
 *       原代码在 10kHz 的 TIM3 中断里调用，可能触发 OSSemPend/OSFlagPend
 *       在中断上下文的非法操作，严重时会挂死系统。
 *
 * 状态机：
 *   IDLE      ：空闲，等待 HMI 新命令
 *   SEND      ：把命令发给驱动器，失败最多重试 2 次
 *   WAIT_DONE ：轮询电机状态，先看到 2（执行中），再看到 1（完成）
 */
void Lock_Process(LockDataDef *lock)
{
    uint8_t  command;
    uint32_t now;

    /* 先刷新驱动器状态，保证后面的完成判断使用的是最新值 */
    (void)Tank_ReadStatus(lock);

    /* 取走 HMI 命令；新命令会覆盖尚未完成的旧命令 */
    command = (uint8_t)sys.ctrl->set_lock;
    sys.ctrl->set_lock = 0;

    if (command >= TANK_CMD_UNLOCK && command <= TANK_CMD_RUN_TO_LOCK)
    {
        lock->active_cmd = command;
        lock->fsm = TANK_FSM_SEND;
        lock->fault = TANK_FAULT_NONE;
        lock->saw_moving = 0;
        lock->retry = 0;
        lock->deadline = OSTimeGet() + TANK_CMD_TIMEOUT_TICKS;
    }

    now = OSTimeGet();

    switch (lock->fsm)
    {
    case TANK_FSM_SEND:
        /* 发送成功：进入等待完成，并设置 500ms 防旧状态误判时间 */
        if (Tank_SendCommand(lock->active_cmd) == 0)
        {
            lock->fsm = TANK_FSM_WAIT_DONE;
            lock->saw_moving = 0;
            lock->guard_time = now + TANK_START_GUARD_TICKS;
            lock->deadline = now + TANK_CMD_TIMEOUT_TICKS;
        }
        /* 发送失败：重试 2 次仍失败，判为通信故障 */
        else if (++lock->retry > TANK_MAX_RETRIES)
        {
            lock->fault = TANK_FAULT_COMM;
            lock->fsm = TANK_FSM_IDLE;
        }
        break;

    case TANK_FSM_WAIT_DONE:
        /*
         * 最近一次状态读取失败：不能使用旧状态，直接等超时。
         * 这样驱动器掉线时不会把上一次的“任务完成”误当成这次完成。
         */
        if (!lock->status_valid)
        {
            if ((int32_t)(now - lock->deadline) >= 0)
            {
                lock->fault = TANK_FAULT_COMM;
                lock->fsm = TANK_FSM_IDLE;
            }
            break;
        }

        /* 看到 2 说明驱动器确实开始执行本次任务 */
        if (lock->motor_state == MOTOR_STATE_EXECUTING)
        {
            lock->saw_moving = 1;
        }

        /* 3 过载：立即结束本次任务，记录故障 */
        if (lock->motor_state == MOTOR_STATE_OVERLOAD)
        {
            lock->fault = TANK_FAULT_OVERLOAD;
            lock->fsm = TANK_FSM_IDLE;
        }
        /* 4 堵转：立即结束本次任务，记录故障 */
        else if (lock->motor_state == MOTOR_STATE_STALL)
        {
            lock->fault = TANK_FAULT_STALL;
            lock->fsm = TANK_FSM_IDLE;
        }
        /*
         * 1 任务完成：
         *   - 如果已经看到过 2，说明是本次任务的完成
         *   - 如果没看到 2（动作太快或轮询错过），至少等过 500ms guard
         *     才接受完成，避免刚发命令就读到上一次的 1
         */
        else if (lock->motor_state == MOTOR_STATE_COMPLETE &&
                 (lock->saw_moving ||
                  (int32_t)(now - lock->guard_time) >= 0))
        {
            lock->lock_state = Tank_CommandToState(lock->active_cmd);
            lock->fsm = TANK_FSM_IDLE;
        }
        /* 20s 还没完成，判为超时 */
        else if ((int32_t)(now - lock->deadline) >= 0)
        {
            lock->fault = TANK_FAULT_TIMEOUT;
            lock->fsm = TANK_FSM_IDLE;
        }
        break;

    default:
        lock->fsm = TANK_FSM_IDLE;
        break;
    }

    /* 把锁状态同步到 HMI 读取的保持寄存器 */
    sys.ctrl->lock_state = lock->lock_state;
}
