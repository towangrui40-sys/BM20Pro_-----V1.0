#include "includes.h"
#include "mb.h"
#include "mb_m.h"
#include "user_mb_app.h"

extern SysDataDef sys;

#define DC_MOTOR_CW 1	//cw lock
#define DC_MOTOR_CCW 0	//ccw unlock
#define DC_MOTOR_RUN 1
#define DC_MOTOR_STOP 0
//PWM输出初始化
//arr：自动重装值
//psc：时钟预分频数
void TIM3_PWM_Init(u16 arr,u16 psc)
{  
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	 80K
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值  不分频
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
	

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; //选择定时器模式:TIM脉冲宽度调制模式2
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
	TIM_OCInitStructure.TIM_Pulse = 0; //设置待装入捕获比较寄存器的脉冲值
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low; //输出极性:TIM输出比较极性高
	
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);  //根据TIM_OCInitStruct中指定的参数初始化外设TIMx
	TIM_OC2Init(TIM3, &TIM_OCInitStructure);  //根据TIM_OCInitStruct中指定的参数初始化外设TIMx
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);  //根据TIM_OCInitStruct中指定的参数初始化外设TIMx
	TIM_OC4Init(TIM3, &TIM_OCInitStructure);  //根据TIM_OCInitStruct中指定的参数初始化外设TIMx
	
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);  //使能TIMx在CCR2上的预装载寄存器
	TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);  //使能TIMx在CCR2上的预装载寄存器
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);  //使能TIMx在CCR2上的预装载寄存器
	TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);  //使能TIMx在CCR2上的预装载寄存器
	
	TIM_ARRPreloadConfig(TIM3, ENABLE); //使能TIMx在ARR上的预装载寄存器
	
//	TIM_CtrlPWMOutputs(TIM3,ENABLE); 	
	
	TIM_ClearFlag(TIM3, TIM_FLAG_Update);  
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //清中断标志位	
	TIM_ITConfig(TIM3,TIM_IT_Update,  ENABLE  );//使能更新触发TIM中断

	/* Configure one bit for preemption priority */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1); //两级占先 八级副优先
	//中断分组设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 4;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器

	TIM_Cmd(TIM3, ENABLE);  //使能TIMx外设
}

void DC_Motor_Init(DoorDataDef  *door,LockDataDef  *lock)
{
	Init_Motor_Gpio();   			//PWM INIT
	Init_Door_State_Gpio();  //sensor input  Init
	
	TIM3_PWM_Init(100-1,72-1);//10K DC MOTOR
	
	TIM_SetCompare1(TIM3,100);
	TIM_SetCompare2(TIM3,100);
	TIM_SetCompare3(TIM3,100);
	TIM_SetCompare4(TIM3,100);
	
	door->ccw_spd = 5;
	door->cw_spd = 5;
	door->lock_action = 0;
	door->unlock_action = 0;
	
	lock->ccw_spd = 5;
	lock->cw_spd = 5;
	lock->lock_action = 0;
	lock->unlock_action = 0;
}

static void DC_Motor_Run(unsigned char run,unsigned char dir,unsigned char spd)
{
	if(run == DC_MOTOR_RUN)
	{
		if(dir==DC_MOTOR_CW) 	TIM_SetCompare2(TIM3,spd);//cw lock
		else TIM_SetCompare1(TIM3,spd);//ccw unlock 	
	}
	else
	{
		TIM_SetCompare1(TIM3,100);
		TIM_SetCompare2(TIM3,100);
	}
}

static void Door_Unlock(DoorDataDef  *door)
{
	if(door->unlock_location == 1) //open
	{
		DC_Motor_Run(DC_MOTOR_STOP,DC_MOTOR_CCW,0);
		door->unlock_action  = 0;
	}
	else
	{
		DC_Motor_Run(DC_MOTOR_RUN,DC_MOTOR_CCW,door->ccw_spd);//ccw open 
	}
}

static void Door_Lock(DoorDataDef  *door)
{
	if(door->lock_location == 1) //locked
	{
		DC_Motor_Run(DC_MOTOR_STOP,DC_MOTOR_CW,0);	
		door->lock_action  = 0;		
	}	
	else
	{
		DC_Motor_Run(DC_MOTOR_RUN,DC_MOTOR_CW,door->cw_spd);		
	}	

}

void Door_Fresh_Timer(DoorDataDef  *door)
{ 
	if(sys.ctrl->set_case == 1) door->unlock_action=1;
	else if(sys.ctrl->set_case == 2) door->lock_action=1;
	sys.ctrl->set_case = 0;
	//fresh state
	door->unlock_location = DOOR_UNLOCK_STATE_GPIO;    //盖开到位
	door->lock_location = DOOR_LOCK_STATE_GPIO;	      //盖关到位
	
	if(door->unlock_location == 0) //盖开到位
		sys.case_state = 1;
	else if(door->lock_location == 0)  //盖关到位
		sys.case_state = 0;
	else
		sys.case_state = 2;
	
}


void Door_Dispose(DoorDataDef  *door)
{
	if(door->unlock_action == 1)
	{
		Door_Unlock(door);//OPEN
	}
	else if(door->lock_action == 1)
	{
		Door_Lock(door);//CLOSE
	}
	else
	{
		DC_Motor_Run(DC_MOTOR_STOP,DC_MOTOR_CW,0);
	}

}


void Send_Absolute_Position(int position)  //发送绝对位置
{
	USHORT sup[] = {0,0};
	sup[0] = position>>16;
	sup[1] = position& 0xffff;
  eMBMasterReqWriteMultipleHoldingRegister(1,1,2,sup,200);
}


//研磨罐锁紧装置
#define UNLOCK_ACTION	  1
#define LOCK_ACTION		 	2	 
#define LOCK_TO_RUN     3
#define RUN_TO_LOCK   	4
void Lock_Fresh_Timer(LockDataDef  *lock)
{
	if(sys.ctrl->set_lock == UNLOCK_ACTION)
	{
		lock->unlock_action = 1;
	}
	else if(sys.ctrl->set_lock == LOCK_ACTION) 	
	{
		lock->lock_action = 1;
	}
	else if(sys.ctrl->set_lock == LOCK_TO_RUN) 	
	{
		lock->lock_to_run = 1;
	}
	else if(sys.ctrl->set_lock == RUN_TO_LOCK) 	
	{
		lock->run_to_lock = 1;
	}
	sys.ctrl->set_lock = 0;
	//fresh state
//	lock->lock_location = LOCK_LOCATION_STATE_GPIO;	  //盖关到位
	Tank_lock_Dispose(lock);
}

#define on  1
#define off 0

void Tank_lock_Dispose(LockDataDef  *lock)
{
 Tank_Lock(lock);
 Tank_Unlock(lock);
 Tank_Lock_to_run(lock);
 Tank_Run_to_lock(lock);
}

void Tank_Lock(LockDataDef  *lock)
{
		if(lock->lock_action == 1)  //关锁
		{
			if(lock->fsm == 0)
			{
				eMBMasterReqWriteHoldingRegister(1,0x29,on,200);
				if(lock->motor_state == 2)
				{
					lock->fsm = 2;
				}			
			}
			if(lock->fsm == 2)
			{
				if(lock->motor_state == 1)
				{
					lock->fsm = 3;
				}
			}
			if(lock->fsm == 3)
			{
				lock->lock_action = 0;
				lock->lock_state = LOCKING;
				lock->fsm = 0;
			}
		}
}
void Tank_Unlock(LockDataDef  *lock)
{
	if(lock->unlock_action == 1)
	{
			if(lock->fsm == 0)
			{
				Send_Absolute_Position(4890000);
				if(lock->motor_state == 2)
				{
					lock->fsm = 2;
				}			
			}
			if(lock->fsm == 2)
			{
				if(lock->motor_state == 1)
				{
					lock->fsm = 3;
				}
			}
			if(lock->fsm == 3)
			{
				lock->unlock_action = 0;
				lock->lock_state = UNLOCKING;
				lock->fsm = 0;
			}
	}
}
void Tank_Lock_to_run(LockDataDef  *lock)
{
	if(lock->lock_to_run == 1)
	{
			if(lock->fsm == 0)
			{
				Send_Absolute_Position(-2620000);
				if(lock->motor_state == 2)
				{
					lock->fsm = 2;
				}				
			}
			if(lock->fsm == 2)
			{
				if(lock->motor_state == 1)
				{
					lock->fsm = 3;
				}
			}
			if(lock->fsm == 3)
			{
				lock->lock_to_run = 0;
				lock->lock_state = READY;
				lock->fsm = 0;
			}
	}	
}
void Tank_Run_to_lock(LockDataDef  *lock)
{
	if(lock->run_to_lock== 1)
	{
			if(lock->fsm == 0)
			{
				Send_Absolute_Position(0);
				lock->fsm = 1;
			}
			if(lock->fsm == 1)
			{
				if(lock->motor_state == 1)
				{
					lock->fsm = 3;
				}
			}
			if(lock->fsm == 3)
			{
				lock->run_to_lock = 0;
				lock->lock_state = LOCKING;
				lock->fsm = 0;
			}
	}
}

	
