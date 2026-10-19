#include "includes.h"

#define SLAVE_ADDR_START	(ADDR_HREG_TOTAL+(SMAPP_HREG_TOTAL*SM_DEVICE_NUM))
#define SLAVE_ADDR_END		(ADDR_HREG_TOTAL*2+(SMAPP_HREG_TOTAL*SM_DEVICE_NUM)*2)

void Slave_Ctrl_allback_Handler(u16 index,u16 data)
{
	u16 cmd = 0;
	if(index < SLAVE_ADDR_START) return;
	if(index > SLAVE_ADDR_END) return;
	cmd = index- SLAVE_ADDR_START;
	
	RTU_Cmd_Pending(cmd,data);
}
