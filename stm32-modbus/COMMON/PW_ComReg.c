#include "PW_ComReg.h"
#include "mb_m.h"

#define PW_RTU_ADDR 5
#define PW_RUN_REGADDR 101
#define PW_SET_CUR_REGADDR 115

typedef struct
{
	PW_RTU_TypeDef *reg;
}PW_Def;

PW_Def pw;

void Init_PW_RegImage(unsigned short *holdreg)
{
	pw.reg = (PW_RTU_TypeDef *)holdreg;
}


short Dispose_PW(PW_TypeDef * str)
{
	eMBMasterReqErrCode err;
	ePWEventType event;
    if( pwEventGet( &event ) == TRUE )
	{
		switch(event)
		{
			case EV_PW_INIT:
				err= MB_MRE_NO_ERR;
				str->work_mode= 1;
			break;
			case EV_PW_SET_CUR:
				err = eMBMasterReqWriteHoldingRegister(PW_RTU_ADDR,PW_SET_CUR_REGADDR,str->set_cur,200);
			break;
			case EV_PW_SET_RUN:
				err = eMBMasterReqWriteHoldingRegister(PW_RTU_ADDR,PW_RUN_REGADDR,str->work_mode,200);			
			break;
			default:
				err =eMBMasterReqReadHoldingRegister(PW_RTU_ADDR,PW_ADDR_STATE,PW_ADDR_HREG_TOTAL,200);	
				str->ver = 1004;
				str->get_cur = pw.reg->ocur;
				str->state = pw.reg->state;
				str->err = pw.reg->err;
			break;
		}		
	}	
	else
	{
		err =eMBMasterReqReadHoldingRegister(PW_RTU_ADDR,PW_ADDR_STATE,PW_ADDR_HREG_TOTAL,200);
		str->ver = 1004;
		str->get_cur = pw.reg->ocur;
		str->state = pw.reg->state;
		str->err = pw.reg->err;	
	}
	return err;
}

























