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
#define LED_TASK_PRIO       					13
#define DC_LED_TASK_PRIO						12
#define MONITOR_TASK_PRIO						11
#define CONTROL_TASK_PRIO						10

/* 罐锁任务优先级：低于 hmi/com 任务，阻塞等待 Modbus 应答时让主站任务运行 */
#define TANK_LOCK_TASK_PRIO					7	
#define HMI_INTERFACE_TASK_PRIO				4
#define COM_INTERFACE_TASK_PRIO				3



#define START_STK_SIZE  				256
#define LED_STK_SIZE  		    		64
#define MONITOR_STK_SIZE				1024
#define CONTROL_STK_SIZE				256
#define DC_LED_STK_SIZE  		    	64
/* 罐锁任务栈：Lock_Process 内部会调用 FreeModbus，栈比原来的 256 加大到 512 */
#define TANK_LOCK_STK_SIZE			512
#define HMI_INTERFACE_STK_SIZE		1024
#define COM_INTERFACE_STK_SIZE		1024


OS_STK START_TASK_STK[START_STK_SIZE];
OS_STK LED_TASK_STK[LED_STK_SIZE];
OS_STK MONITOR_TASK_STK[MONITOR_STK_SIZE];
OS_STK CONTROL_TASK_STK[CONTROL_STK_SIZE];
OS_STK DC_LED_TASK_STK[DC_LED_STK_SIZE];
OS_STK TANK_LOCK_TASK_STK[TANK_LOCK_STK_SIZE];
OS_STK HMI_INTERFACE_TASK_STK[HMI_INTERFACE_STK_SIZE];
OS_STK COM_INTERFACE_TASK_STK[COM_INTERFACE_STK_SIZE];

void start_task(void *pdata);
void led_task(void *pdata);	
void monitor_task(void *pdata);
void control_task(void *pdata);
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
	OSTaskCreate(control_task,(void *)0,(OS_STK*)&CONTROL_TASK_STK[CONTROL_STK_SIZE-1],CONTROL_TASK_PRIO);
	OSTaskCreate(led_task,(void *)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);
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
    OS_CPU_SR cpu_sr = 0;

    memset(&pattern, 0, sizeof(pattern));
    Led_AllOff();
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
        delay_ms(LED_TASK_POLL_MS);
    }
}

void monitor_task(void *pdata)
{
	while(1)
	{
		/* 每 20ms 处理一次盖子电机：读限位、开/关盖、更新 case_state */
		Door_Process(&door);
		
		/* 读取把手/磁铁传感器，发布给 HMI */
		if(TANK_LOCK == 0)
			sys.ctrl->tank_lock = 0;
		else
			sys.ctrl->tank_lock = 1;

		delay_ms(20);
	}
}
void control_task(void *pdata)
{
	float temp = 0;
	Dac1_Set_Vol((u16)temp);
	delay_ms(1000);
	while(1)
	{
		//ac out
		if(sys.ctrl->set_ac1 == 1) AC_CTRL1(ON);
		else AC_CTRL1(OFF);
		if(sys.ctrl->set_ac2 == 1) AC_CTRL2(ON);
		else AC_CTRL2(OFF);
		if(sys.ctrl->set_ac3 == 1) AC_CTRL3(ON);
		else AC_CTRL3(OFF);	
	//get load current
		delay_ms(50);
	}
}

void led_task(void *pdata)
{
	static short delay=500;
	while(1)
	{	
		LED_RUN(ON);
		delay_ms(delay);
		LED_RUN(OFF);
		delay_ms(delay);
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

