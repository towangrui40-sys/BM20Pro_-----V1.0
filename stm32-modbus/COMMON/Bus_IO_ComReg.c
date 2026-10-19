#include "Bus_IO_ComReg.h"
#include "mb_m.h"

#define BUS_IO_RTU_ADDR 3

typedef struct
{
	BUS_IO_TypeDef *reg;
}BUS_Def;

BUS_Def bus;

void Init_BusIO_RegImage(unsigned short *holdreg)
{
	bus.reg = (BUS_IO_TypeDef *)holdreg;
}


short Dispose_BusIO(BUS_IO_TypeDef * str)
{
	eMBMasterReqErrCode err;
	eBUSEventType event;
    if( busEventGet( &event ) == TRUE )
	{
		switch(event)
		{
			case EV_BUS_INIT:
				err= MB_MRE_NO_ERR;
			break;
			case EV_BUS_SET:
				err = eMBMasterReqWriteMultipleHoldingRegister(BUS_IO_RTU_ADDR,BUS_ADDR_COOL_FAN1,BUS_ADDR_HREG_TOTAL-BUS_ADDR_COOL_FAN1,(unsigned short *)&str->cfan1,200);
			break;
			default:
				err =eMBMasterReqReadHoldingRegister(BUS_IO_RTU_ADDR,0,BUS_ADDR_HREG_TOTAL,200);	
			break;
		}		
	}	
	else
	{
		err =eMBMasterReqReadHoldingRegister(3,0,BUS_ADDR_HREG_TOTAL,200);	
		memcpy(str,bus.reg,BUS_ADDR_HREG_TOTAL*2);
	}

	return err;
}

