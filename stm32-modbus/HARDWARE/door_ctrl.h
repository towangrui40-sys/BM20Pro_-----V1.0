#ifndef	__DOOR_CTRL_H_ 
#define __DOOR_CTRL_H_ 

#define DOOR_TYPE_WHEEL

#ifdef DOOR_TYPE_WHEEL
//wheel door
#define DOOR_UNLOCK_STATE_GPIO  GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0);
//wheel door 
#define	DOOR_LOCK_STATE_GPIO  	GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1);
  
//lock 锁紧与解锁到位传感器   
#define LOCK_LOCATION_STATE_GPIO  GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_13);
//lock 收到信号开始锁紧
#define	LOCK_FLAG_STATE_GPIO	GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_14);



#define WHEEL_CLOSE_DELAY 100
#endif 



#define DOOR_FRESH_TICK_1MS 10

#define CLOSE_DELAY 500	//500ms

#define OPEN_DELAY_TOTAL 2200	//2.4s unlock run time + wait time
#define OPEN_DELAY_WAIT 1500	//1.5s


#define DOOR_DELAY_TIME(x) (DOOR_FRESH_TICK_1MS*x)  

#define UNLOCKING  0
#define LOCKING	   1	 
#define READY      2

typedef struct
{
	short lock_action;
	short unlock_action;
	short lock_to_run;
	short run_to_lock;
	
	short cw_spd;  //正转速度
	short ccw_spd; //反转速度
	
	short lock_location;   //上锁位置  磁铁感应传感器
	short unlock_location; //开锁位置

	short fsm;  //状态机
	short rtu_data_action;
	short lock_state;  //锁状态
	short motor_state;
//	int    steps;
}DoorDataDef,LockDataDef;

void DC_Motor_Init(DoorDataDef  *door,LockDataDef  *lock);

void Tank_lock_Dispose(LockDataDef  *lock);
void Lock_Fresh_Timer(LockDataDef  *lock);

void Tank_Lock(LockDataDef  *lock);
void Tank_Unlock(LockDataDef  *lock);
void Tank_Lock_to_run(LockDataDef  *lock);
void Tank_Run_to_lock(LockDataDef  *lock);

void Door_Fresh_Timer(DoorDataDef  *door);
void Door_Dispose(DoorDataDef  *door);

#endif
