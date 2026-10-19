#include "sys.h"
#include "includes.h"

/* ----------------------- Defines ------------------------------------------*/
#define OS_ERR INT8U


static OS_FLAG_GRP 	* tmsOsFlagGrp=0;


/* ----------------------- Start implementation -----------------------------*/
unsigned char 
TmsEventInit(void)
{
	OS_ERR xFlagErr = 0; 
	tmsOsFlagGrp = OSFlagCreate(0,&xFlagErr);
	if(xFlagErr == OS_ERR_NONE)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
unsigned char 
TmsEventPost( eTMSEventType eEvent)
{
	OS_ERR			xFlagErr;
	OS_FLAGS 		xFlag;
	
	xFlag = xFlag;

	xFlag = OSFlagPost(tmsOsFlagGrp,eEvent,OS_FLAG_SET, &xFlagErr);

	if(xFlagErr == OS_ERR_NONE)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

unsigned char 
TmsEventGet( eTMSEventType * eEvent )
{
	OS_ERR		xGetFlagErr;
	OS_FLAGS 		xGetFlag;
	xGetFlag = OSFlagPend(	tmsOsFlagGrp, 
							EV_THERMOSTAT_NULL|EV_THERMOSTAT_INIT|EV_THERMOSTAT_SET_TEMP|EV_THERMOSTAT_SET_RUN, 
							OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 
							50,
							&xGetFlagErr);	

	if( xGetFlagErr == OS_ERR_NONE)
	{
		switch (xGetFlag)
		{
		case EV_THERMOSTAT_NULL:
			*eEvent = EV_THERMOSTAT_NULL;
			break;
		case EV_THERMOSTAT_INIT:
			*eEvent = EV_THERMOSTAT_INIT;
			break;
		case EV_THERMOSTAT_SET_TEMP:
			*eEvent = EV_THERMOSTAT_SET_TEMP;
			break;
		case EV_THERMOSTAT_SET_RUN:
			*eEvent = EV_THERMOSTAT_SET_RUN;
			break;
		}
		return TRUE;	
	}
	else
	{
		return FALSE;
	}
}
