#include "includes.h"
#include "mb.h"
#include "mb_m.h"
#include "user_mb_app.h"

 /***********************Version record********************************
 
 Version:V1.0
 date:2023/03/15
 author:Jason
 Note:
 1.Initial creation 

**********************************************************************/

u8 bTemp;
extern uint8_t RxPacket[5];
short ction = 0;

SysDataDef sys;
DoorDataDef door;
LockDataDef lock;

void Init_Rtu_RegHoldingBuf(SysDataDef* dat,short *buf)
{
	memset(&sys,0,sizeof(sys));
	dat->ctrl = (CTRL_TypeDef *)&buf[0];
}
			   
#define START_TASK_PRIO      					14
#define DC_LED_TASK_PRIO						12
#define MONITOR_TASK_PRIO						11
#define SENSOR_TASK_PRIO						9

/* 罐锁任务优先级：低于 hmi/com 任务，阻塞等待 Modbus 应答时让主站任务运行 */
#define TANK_LOCK_TASK_PRIO					7	
#define HMI_INTERFACE_TASK_PRIO				4
#define COM_INTERFACE_TASK_PRIO				3



#define START_STK_SIZE  				256
#define MONITOR_STK_SIZE				1024
#define SENSOR_STK_SIZE					256
#define DC_LED_STK_SIZE  		    	64
/* 罐锁任务栈：Lock_Process 内部会调用 FreeModbus，栈比原来的 256 加大到 512 */
#define TANK_LOCK_STK_SIZE			512
#define HMI_INTERFACE_STK_SIZE		1024
#define COM_INTERFACE_STK_SIZE		1024


OS_STK START_TASK_STK[START_STK_SIZE];
OS_STK MONITOR_TASK_STK[MONITOR_STK_SIZE];
OS_STK SENSOR_TASK_STK[SENSOR_STK_SIZE];
OS_STK DC_LED_TASK_STK[DC_LED_STK_SIZE];
OS_STK TANK_LOCK_TASK_STK[TANK_LOCK_STK_SIZE];
OS_STK HMI_INTERFACE_TASK_STK[HMI_INTERFACE_STK_SIZE];
OS_STK COM_INTERFACE_TASK_STK[COM_INTERFACE_STK_SIZE];

void start_task(void *pdata);
void monitor_task(void *pdata);
void sensor_task(void *pdata);
void dc_led_task(void *pdata);	
void tank_lock_task(void *pdata);
void hmi_interface_task(void *pdata);
void com_interface_task(void *pdata);

int main(void)
{ 
	NVIC_Configuration();
	Init_Rtu_RegHoldingBuf(&sys,(short *)usRegHoldingBuf);
	GPIO_Config();
	ADC1_Init();
	DAC1_Init();
	Motor_Init(&door,&lock);   /* 初始化盖子电机、罐锁结构和 TIM3 PWM */
	OSInit();   
 	OSTaskCreate(start_task,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );//������ʼ����
	OSStart();
	memset(&door,0,sizeof(door));
	memset(&lock,0,sizeof(lock));
 }

void start_task(void *pdata)
{
	OS_CPU_SR cpu_sr=0;
	pdata = pdata; 
	delay_init();
	
	OS_ENTER_CRITICAL();
	
 	OSTaskCreate(hmi_interface_task,(void *)0,(OS_STK*)&HMI_INTERFACE_TASK_STK[HMI_INTERFACE_STK_SIZE-1],HMI_INTERFACE_TASK_PRIO);	
 	OSTaskCreate(com_interface_task,(void *)0,(OS_STK*)&COM_INTERFACE_TASK_STK[COM_INTERFACE_STK_SIZE-1],COM_INTERFACE_TASK_PRIO);
	OSTaskCreate(tank_lock_task,(void *)0,(OS_STK*)&TANK_LOCK_TASK_STK[TANK_LOCK_STK_SIZE-1],TANK_LOCK_TASK_PRIO);	
	OSTaskCreate(monitor_task,(void *)0,(OS_STK*)&MONITOR_TASK_STK[MONITOR_STK_SIZE-1],MONITOR_TASK_PRIO);
	OSTaskCreate(sensor_task,(void *)0,(OS_STK*)&SENSOR_TASK_STK[SENSOR_STK_SIZE-1],SENSOR_TASK_PRIO);
	OSTaskCreate(dc_led_task,(void *)0,(OS_STK*)&DC_LED_TASK_STK[DC_LED_STK_SIZE-1],DC_LED_TASK_PRIO);
	OSTaskSuspend(START_TASK_PRIO);
	OS_EXIT_CRITICAL();
}	  


void hmi_interface_task(void *pdata)
{
	delay_ms(5000);
	#if ( (MB_RTU_ENABLED | MB_ASCII_ENABLED) > 0)	
	eMBInit(MB_RTU, 0x03, 1, 115200, MB_PAR_NONE);
	eMBEnable();	
#endif	
	while(1)
	{	
		eMBPoll();
	}
}

void com_interface_task(void *pdata)
{
#if ( (MB_MASTER_RTU_ENABLED | MB_MASTER_ASCII_ENABLED) > 0)	
	eMBMasterInit(MB_RTU, 2, 9600, MB_PAR_NONE);
	eMBMasterEnable();	
#endif	
	while(1)
	{
	 	eMBMasterPoll();
	}
}


/*
 * 研磨罐锁任务
 * 所有 Modbus 主站请求都在这里执行，不能放到 TIM3 中断里。
 * Lock_Process() 内部会阻塞等待驱动器应答，所以本任务优先级要低于
 * com_interface_task，让主站轮询任务有机会处理串口应答。
 */
void tank_lock_task(void *pdata)
{
	delay_ms(1000);
	while(1)
	{
		/* 读驱动器状态、发位置/归零命令、等待任务完成 */
		Lock_Process(&lock);
		delay_ms(TANK_POLL_MS);
	}
}

/* -------------------------------------------------------------------------
 * LED/蜂鸣器提示
 *
 * 原实现用 6 次 for 循环 + delay_ms(200)，一次提示会阻塞 2.4 秒：
 *   - 提示期间 HMI 发来的新 set_led 要等提示结束才处理
 *   - 提示结束时直接把 set_led 清 0，可能把新命令一起清掉
 *
 * 这里改成非阻塞状态机：任务每 10ms 推进一步，提示效果不变，
 * 但新命令可以立即打断旧提示。
 * ---------------------------------------------------------------------- */
#define LED_CMD_RED             1u      /* HMI 命令：红灯 */
#define LED_CMD_GREEN           2u      /* HMI 命令：绿灯 */
#define LED_BLINK_COUNT         6u      /* 闪烁次数 */
#define LED_HALF_PERIOD_MS      200u    /* 亮 200ms、灭 200ms */
#define LED_HALF_PERIOD_TICKS   ((uint32_t)LED_HALF_PERIOD_MS * OS_TICKS_PER_SEC / 1000u)
#define LED_RUN_HALF_PERIOD_MS  500u    /* 运行灯亮 500ms、灭 500ms */
#define LED_RUN_HALF_PERIOD_TICKS ((uint32_t)LED_RUN_HALF_PERIOD_MS * OS_TICKS_PER_SEC / 1000u)
#define LED_TASK_POLL_MS        10u     /* 任务轮询周期 */

typedef struct
{
    uint8_t  active;        /* 1=正在提示，0=空闲 */
    uint8_t  led;           /* 1=红灯，2=绿灯 */
    uint8_t  on;            /* 当前亮/灭状态 */
    uint8_t  toggles_left;  /* 还剩多少次亮灭切换 */
    uint32_t next_tick;     /* 下一次切换的 OSTimeGet() tick */
} LedPatternDef;

/* 关闭所有指示灯和蜂鸣器 */
static void Led_AllOff(void)
{
    DC_CTRL1(OFF);
    DC_CTRL2(OFF);
    DC_CTRL3(OFF);
    BEEP_OUT(OFF);
}

/* 按颜色输出亮/灭，未使用的灯保持关闭 */
static void Led_Apply(uint8_t led, uint8_t on)
{
    if (led == LED_CMD_RED)
    {
        DC_CTRL1(on ? ON : OFF);
        DC_CTRL2(OFF);
    }
    else if (led == LED_CMD_GREEN)
    {
        DC_CTRL2(on ? ON : OFF);
        DC_CTRL1(OFF);
    }
    else
    {
        DC_CTRL1(OFF);
        DC_CTRL2(OFF);
    }

    DC_CTRL3(OFF);
    BEEP_OUT(on ? ON : OFF);
}

/* 启动一轮提示：先亮，之后每 200ms 翻转一次 */
static void Led_Start(LedPatternDef *p, uint8_t led)
{
    p->active = 1;
    p->led = led;
    p->on = 1;
    p->toggles_left = (LED_BLINK_COUNT * 2u) - 1u;
    p->next_tick = OSTimeGet() + LED_HALF_PERIOD_TICKS;
    Led_Apply(led, 1);
}

/* 每 10ms 调一次，到时间就翻转一次亮灭状态 */
static void Led_Update(LedPatternDef *p)
{
    uint32_t now;

    if (p->active == 0u)
    {
        return;
    }

    now = OSTimeGet();
    if ((int32_t)(now - p->next_tick) < 0)
    {
        return;
    }

    p->next_tick += LED_HALF_PERIOD_TICKS;

    if (p->on)
    {
        p->on = 0;
        Led_Apply(p->led, 0);
    }
    else
    {
        p->on = 1;
        Led_Apply(p->led, 1);
    }

    if (p->toggles_left > 0u)
    {
        p->toggles_left--;
        if (p->toggles_left == 0u)
        {
            p->active = 0;
            Led_AllOff();
        }
    }
}

/*
 * LED/蜂鸣器任务
 *   - LED_RUN 每 500ms 翻转一次，作为运行指示灯
 *   - set_led 控制红/绿提示灯和蜂鸣器
 *
 *   set_led = 1：红灯闪 6 次
 *   set_led = 2：绿灯闪 6 次
 *   其它非 0 值：关闭所有灯
 *
 * 提示过程中收到新命令会立即切换，不再等 2.4 秒的 for 循环结束。
 */
void dc_led_task(void *pdata)
{
    LedPatternDef pattern;
    uint8_t command;
    uint8_t run_on = 1;
    uint32_t run_next_tick;
    uint32_t now;
    OS_CPU_SR cpu_sr = 0;

    memset(&pattern, 0, sizeof(pattern));
    Led_AllOff();
    LED_RUN(ON);
    run_next_tick = OSTimeGet() + LED_RUN_HALF_PERIOD_TICKS;
    delay_ms(200);

    while (1)
    {
        /*
         * 读命令和清命令放在同一个临界区里，避免 HMI 恰好在这两步之间
         * 写入新命令、又被这里清掉。
         */
        OS_ENTER_CRITICAL();
        command = (uint8_t)sys.ctrl->set_led;
        sys.ctrl->set_led = 0;
        OS_EXIT_CRITICAL();

        if (command == LED_CMD_RED || command == LED_CMD_GREEN)
        {
            Led_Start(&pattern, command);
        }
        else if (command != 0u)
        {
            pattern.active = 0;
            Led_AllOff();
        }

        Led_Update(&pattern);

        /* 运行指示灯：500ms 亮、500ms 灭，和提示灯互不影响 */
        now = OSTimeGet();
        if ((int32_t)(now - run_next_tick) >= 0)
        {
            run_next_tick += LED_RUN_HALF_PERIOD_TICKS;
            run_on = !run_on;
            LED_RUN(run_on ? ON : OFF);
        }

        delay_ms(LED_TASK_POLL_MS);
    }
}

/*
 * AC 输出处理
 * 由 monitor_task 每 50ms 调用一次。
 * 这里只做短小的 GPIO 输出，不会阻塞，所以不需要单独开任务。
 */
static void Control_Process(void)
{
	if(sys.ctrl->set_ac1 == 1) AC_CTRL1(ON);
	else AC_CTRL1(OFF);

	if(sys.ctrl->set_ac2 == 1) AC_CTRL2(ON);
	else AC_CTRL2(OFF);

	if(sys.ctrl->set_ac3 == 1) AC_CTRL3(ON);
	else AC_CTRL3(OFF);
}

/*
 * 监控任务
 *   10ms 基准节拍：
 *     - 每 10ms 处理盖子电机和把手传感器
 *     - 每 50ms 处理一次 AC 输出
 *
 * Door_Process/Control_Process 都是非阻塞的短函数，可以放在同一任务里。
 * 罐锁的 Lock_Process 内部会阻塞等 Modbus，所以仍然保留独立任务。
 */
void monitor_task(void *pdata)
{
	uint8_t control_div = 0;

	while(1)
	{
		/* 盖子电机：读限位、开/关盖、更新 case_state */
		Door_Process(&door);

		/* 50ms 分频：10ms * 5 */
		if(++control_div >= 5)
		{
			control_div = 0;
			Control_Process();
		}

		/* 读取把手/磁铁传感器，发布给 HMI */
		if(TANK_LOCK == 0)
			sys.ctrl->tank_lock = 0;
		else
			sys.ctrl->tank_lock = 1;

		delay_ms(10);
	}
}

/*
 * 加速度计任务
 * 每 10ms 读一次 LIS302DL，把 x/y/z 原始值发布到保持寄存器，
 * 供 HMI 的震动过大保护使用。
 */
void sensor_task(void *pdata)
{
	MEMS_TYPEDEF mems;
	uint8_t ret;
	uint8_t errors = 0;

	delay_ms(500);

	/* 传感器没插好或型号不对时，每 500ms 重试一次初始化 */
	while(LIS302DL_Init() != 0)
	{
		delay_ms(500);
	}

	while(1)
	{
		ret = LIS302DL_Output(&mems);
		if(ret == 0)
		{
			/* 读到新数据才更新，避免把旧值反复发布 */
			sys.ctrl->x = mems.x;
			sys.ctrl->y = mems.y;
			sys.ctrl->z = mems.z;
			errors = 0;
		}
		else if(ret == 2)
		{
			/* 连续 10 次 I2C 错误，重新初始化总线 */
			if(++errors >= 10)
			{
				errors = 0;
				while(LIS302DL_Init() != 0)
				{
					delay_ms(500);
				}
			}
		}

		delay_ms(10);
	}
}

/*
 * TIM3 更新中断，10kHz
 * 只做轮盘霍尔采样，不再调用门/锁状态机，也不做任何 Modbus 请求。
 * 原来的 Lock_Fresh_Timer()/Door_Fresh_Timer() 已经移到任务中执行。
 */
void TIM3_IRQHandler(void)
{
	static short wheel_r = 0;
	static short wheel_l = 0;
	static short wheel_p = 0;
	if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update  ); 
		
		/* 采样三个轮盘霍尔，组合出 HMI 使用的 location_test 值 */
		wheel_r = WHEEL_RIGHT_HALL;
		wheel_l = WHEEL_LEFT_HALL;
		wheel_p = WHEEL_PISITION_HALL;
		/* 1 号工位 */
		if(wheel_r == 0 && wheel_l == 1 && wheel_p == 0) 
		{
			sys.ctrl->location_test = 0x01;
		}
		/* 2 号工位 */
		if(wheel_r == 1 && wheel_l == 0 && wheel_p == 0) 
		{
			sys.ctrl->location_test = 0x02;
		}
		/* 回零/中间位置 */
		if(wheel_r == 1 && wheel_l == 1 && wheel_p == 1)
		{
			sys.ctrl->location_test = 0x00;
		}
	}
}

