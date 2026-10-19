#include "sys.h"
#include "string.h"
#include "usart.h"
#include "includes.h"

extern SysDataDef sys;

void RTU_Cmd_Pending(u16 cmd,u16 dat)
{
	rtu_structure.cmd[rtu_structure.pending] = cmd;
	rtu_structure.cmd_data[rtu_structure.pending] = dat;
	rtu_structure.pending = ++rtu_structure.pending%MAX_BUFFER_NUMBER;
}
void RTU_Send_Cmd(u8 index)
{
	static u8 num =0;
	switch(rtu_structure.cmd[index])
	{
		case RTU_IDlE:
			switch(num)
			{
				case 0:
					rtu_protocol_format_rd(0,25);
				break;
				case 1:
					rtu_protocol_format_rd(25,39);
				break;
//				case 2:
//					rtu_protocol_format_rd(64,39);
//				break;
				default:
					rtu_protocol_format_rd(0,25);
					break;
			}				
			num++;
			if(num >2) num =0;
			break;
		default: 
			rtu_protocol_format_wr(rtu_structure.cmd[index],1,&rtu_structure.cmd_data[index]);
			break;
	}
}
void RTU_Send_Task_Handler(void)
{

	if(rtu_structure.event == C_EV_REPEAT)
	{
		if(rtu_structure.repeat == 0)
		{
			BEEP_OUT(ON);
			rtu_structure.addr = 0;
			rtu_structure.funcode = 0;
			rtu_structure.event = C_EV_READY;			
		}
		else
		{
			rtu_structure.processed = --rtu_structure.processed % MAX_BUFFER_NUMBER;
			RTU_Send_Cmd(rtu_structure.processed);
			rtu_structure.event = C_EV_EXECUTE;	
			//rtu_structure.cmd[rtu_structure.processed] = RTU_IDlE;
			rtu_structure.processed = ++rtu_structure.processed % MAX_BUFFER_NUMBER;				
		}
		if(rtu_structure.repeat>0) rtu_structure.repeat--;			
	}
	if(rtu_structure.event!=C_EV_READY)
	return;	
	if(rtu_structure.processed == rtu_structure.pending)
	{
		RTU_Cmd_Pending(RTU_IDlE,0);
	}				
	//start process send cmd
	rtu_structure.event = C_EV_FRAME_SENT;	
	RTU_Send_Cmd(rtu_structure.processed);
	rtu_structure.repeat = 3;
	rtu_structure.event = C_EV_EXECUTE;	
	//rtu_structure.cmd[rtu_structure.processed] = RTU_IDlE;
	rtu_structure.processed = ++rtu_structure.processed % MAX_BUFFER_NUMBER;	
}
//dispose receive_data
void API_Callback_Handler(API_StructDef* request, API_Data_StructDef* response_data)
{
//	unsigned short  addr=0;    
	unsigned short  index=0;  
	short* dat = (short *)request->Data;   
	short* get = (short *)sys.ctrl_ch2;
	
	response_data = 0;   
	//addr = RTU_MEM(request->addr);
	switch(request->funcode)
	{
		case RTU_FUNC_CODE_RD:
			for(index = 0;index<request->len;index++)
			{
				get[request->addr+index] = *dat++;
			} 				
		break;
		case RTU_FUNC_CODE_WR:
			break;
		case RTU_FUNC_CODE_CWR:
			
		break;
		case RTU_FUNC_CODE_ERROR:
			
			break;
		default: break;
	}
}
