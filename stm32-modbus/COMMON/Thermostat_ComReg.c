#include "Thermostat_ComReg.h"
#include "mb_m.h"

#define TMS_RTU_ADDR 4
#define TMS_VER_REGADDR 21
#define TMS_SRUN_REGADDR 27

typedef struct
{
	TMS_RTU_TypeDef *reg;
}TMS_Def;

TMS_Def tms;

void Init_Thermpstat_RegImage(unsigned short *holdreg)
{
	tms.reg = (TMS_RTU_TypeDef *)holdreg;
}


short Dispose_Thermpstat(TMS_TypeDef * str)
{
	eMBMasterReqErrCode err;
	eTMSEventType event;
    if( TmsEventGet( &event ) == TRUE )
	{
		switch(event)
		{
			case EV_THERMOSTAT_INIT:
				err= MB_MRE_NO_ERR;

				str->work_mode= 1;
			break;
			case EV_THERMOSTAT_SET_TEMP:
				err = eMBMasterReqWriteHoldingRegister(TMS_RTU_ADDR,TMS_RTU_MEM(TMS_ADDR_SP1),str->set_temp,200);
			break;
			case EV_THERMOSTAT_SET_RUN:
				err = eMBMasterReqWriteHoldingRegister(TMS_RTU_ADDR,TMS_SRUN_REGADDR,str->work_mode,200);
			break;
			default:
			break;
		}		
	}	
	else
	{
		err =eMBMasterReqReadHoldingRegister(TMS_RTU_ADDR,TMS_RTU_MEM(TMS_ADDR_PV),TMS_ADDR_HREG_TOTAL,200);
		if(err == MB_MRE_NO_ERR)
		{
			str->ver = 7197;
			str->get_temp = tms.reg->pv;
			str->set_temp = tms.reg->sv;
			str->state = tms.reg->state;
			str->err = tms.reg->err;		
		}
	
	}

	return err;
}

