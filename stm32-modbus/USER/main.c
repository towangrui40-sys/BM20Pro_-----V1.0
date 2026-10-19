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
extern uint8_t RxPacket[5];  // ������յ������ݰ�
short ction = 0;

SysDataDef sys;
DoorDataDef door;
LockDataDef lock;

void Init_Rtu_RegHoldingBuf(SysDataDef* dat,short *buf)
{
	memset(&sys,0,sizeof(sys));
	dat->ctrl = (CTRL_TypeDef *)&buf[0];
}
/////////////////////////UCOSII�����ջ����///////////////////////////////////
/////////////////////////��������///////////////////////////////////			   
//�����������ȼ�
#define START_TASK_PRIO      					14 //��ʼ��������ȼ�����Ϊ���
#define LED_TASK_PRIO       					13
#define DC_LED_TASK_PRIO							12
#define MONITOR_TASK_PRIO							11
#define CONTROL_TASK_PRIO							10

#define CH1_UD_TASK_PRIO							7	
#define HMI_INTERFACE_TASK_PRIO				4
#define COM_INTERFACE_TASK_PRIO				3



//���������ջ��С
#define START_STK_SIZE  					256
#define LED_STK_SIZE  		    		64
#define MONITOR_STK_SIZE					1024
#define CONTROL_STK_SIZE					256
#define DC_LED_STK_SIZE  		    	64
#define CH1_UD_STK_SIZE						256
#define HMI_INTERFACE_STK_SIZE		1024
#define COM_INTERFACE_STK_SIZE		1024


//�����ջ
OS_STK START_TASK_STK[START_STK_SIZE];
OS_STK LED_TASK_STK[LED_STK_SIZE];
OS_STK MONITOR_TASK_STK[MONITOR_STK_SIZE];
OS_STK CONTROL_TASK_STK[CONTROL_STK_SIZE];
OS_STK DC_LED_TASK_STK[DC_LED_STK_SIZE];
OS_STK CH1_UD_TASK_STK[CH1_UD_STK_SIZE];
OS_STK HMI_INTERFACE_TASK_STK[HMI_INTERFACE_STK_SIZE];
OS_STK COM_INTERFACE_TASK_STK[COM_INTERFACE_STK_SIZE];

//������
void start_task(void *pdata);
void led_task(void *pdata);	
void monitor_task(void *pdata);
void control_task(void *pdata);
void dc_led_task(void *pdata);	
void ch1_ud_task(void *pdata);
void hmi_interface_task(void *pdata);
void com_interface_task(void *pdata);

void TIM3_PWM_Init(u16 arr,u16 psc);	

int main(void)
{ 
	NVIC_Configuration(); 	 //����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	Init_Rtu_RegHoldingBuf(&sys,(short *)usRegHoldingBuf);
	 
	
	GPIO_Config();		  	 //��ʼ����LED���ӵ�Ӳ���ӿ�
	ADC1_Init();
	DAC1_Init();
	DC_Motor_Init(&door,&lock);
	OSInit();   
 	OSTaskCreate(start_task,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );//������ʼ����
	OSStart();
	memset(&door,0,sizeof(door));
	memset(&lock,0,sizeof(lock));
 }

 //��ʼ����
void start_task(void *pdata)
{
	OS_CPU_SR cpu_sr=0;
	pdata = pdata; 
	delay_init();	     //��ʱ��ʼ��	 
	
	OS_ENTER_CRITICAL();			//�����ٽ���(�޷����жϴ��)   
	
 	OSTaskCreate(hmi_interface_task,(void *)0,(OS_STK*)&HMI_INTERFACE_TASK_STK[HMI_INTERFACE_STK_SIZE-1],HMI_INTERFACE_TASK_PRIO);	
 	OSTaskCreate(com_interface_task,(void *)0,(OS_STK*)&COM_INTERFACE_TASK_STK[COM_INTERFACE_STK_SIZE-1],COM_INTERFACE_TASK_PRIO);
	OSTaskCreate(ch1_ud_task,(void *)0,(OS_STK*)&CH1_UD_TASK_STK[CH1_UD_STK_SIZE-1],CH1_UD_TASK_PRIO);
	OSTaskCreate(monitor_task,(void *)0,(OS_STK*)&MONITOR_TASK_STK[MONITOR_STK_SIZE-1],MONITOR_TASK_PRIO);
	OSTaskCreate(control_task,(void *)0,(OS_STK*)&CONTROL_TASK_STK[CONTROL_STK_SIZE-1],CONTROL_TASK_PRIO);
	OSTaskCreate(led_task,(void *)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);
	OSTaskCreate(dc_led_task,(void *)0,(OS_STK*)&DC_LED_TASK_STK[DC_LED_STK_SIZE-1],DC_LED_TASK_PRIO);
	OSTaskSuspend(START_TASK_PRIO);	//������ʼ����.
	OS_EXIT_CRITICAL();				//�˳��ٽ���(���Ա��жϴ��)
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


#define on  0xFF00
#define off 0x0000
void ch1_ud_task(void *pdata)
{
	delay_ms(1000);
	while(1)
	{
		eMBMasterReqReadInputRegister(1,0,0x0E,100);
		lock.motor_state = usMRegInBuf[0][0];
		delay_ms(100);
	}
}

void dc_led_task(void *pdata)
{
	static int i;
	delay_ms(200);
	while(1)
	{
			/***********LED control***********/
		if(sys.ctrl->set_led == 1)  //RED
		{
			for(i=0;i<=5;i++)
			{
				DC_CTRL1(ON);
				BEEP_OUT(ON);
				delay_ms(200);
				DC_CTRL1(OFF);
				BEEP_OUT(OFF);
				delay_ms(200);
			}
			sys.ctrl->set_led = 0;
		}
		else if(sys.ctrl->set_led == 2)  //GREEN
		{
			for(i=0;i<=5;i++)
			{
				DC_CTRL2(ON);
				BEEP_OUT(ON);
				delay_ms(200);
				DC_CTRL2(OFF);
				BEEP_OUT(OFF);
				delay_ms(200);
			}
			sys.ctrl->set_led = 0;
		}
		else   //OFF
		{
			DC_CTRL1(OFF);
			DC_CTRL2(OFF);
			DC_CTRL3(OFF);
			delay_ms(500);
		}
	}
}

void monitor_task(void *pdata)
{

	Init_Wheel_Gpio();   //������������ʼ��	
	while(1)
	{
		Door_Dispose(door);
		
		if(TANK_LOCK == 0)
			sys.ctrl->tank_lock = 0;
		else
			sys.ctrl->tank_lock = 1;

		delay_ms(20);
	}
}
void control_task(void *pdata)
{
	static float adc_ch2=0;
	static short vol_temp=0;
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

//LED����
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

void TIM3_IRQHandler(void)
{
	static short wheel_r = 0;
	static short wheel_l = 0;
	static short wheel_p = 0;
	if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) //���ָ����TIM�жϷ������:TIM �ж�Դ 
	{
		
		Lock_Fresh_Timer(&lock);
		Door_Fresh_Timer(&door);	
		
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update  );  //���TIMx���жϴ�����λ:TIM �ж�Դ 
		
		wheel_r = WHEEL_RIGHT_HALL;
		wheel_l = WHEEL_LEFT_HALL;
		wheel_p = WHEEL_PISITION_HALL;
		if(wheel_r == 0 && wheel_l == 1 && wheel_p == 0) 
		{
			sys.ctrl->location_test = 0x01;
		}
		if(wheel_r == 1 && wheel_l == 0 && wheel_p == 0) 
		{
			sys.ctrl->location_test = 0x02;
		}
		if(wheel_r == 1 && wheel_l == 1 && wheel_p == 1)
		{
			sys.ctrl->location_test = 0x00;
		}
	}
}

