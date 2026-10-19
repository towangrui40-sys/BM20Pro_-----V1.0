#ifndef __RTU_PROCESS_H_
#define __RTU_PROCESS_H_


//TRU CMD 
typedef enum
{
    RTU_IDlE = 1000,    
}RTU_StateType;

void RTU_Send_Task_Handler(void);

void RTU_Cmd_Pending(unsigned short  cmd,unsigned short dat);
#endif
