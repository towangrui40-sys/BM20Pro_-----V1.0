#include "sys.h"
#include "includes.h"


/* ----------------------- Defines ------------------------------------------*/
#define OS_ERR INT8U




static OS_FLAG_GRP 	* BusOsFlagGrp=0;



/* ----------------------- Start implementation -----------------------------*/
unsigned char 
busEventInit( void )
{
	OS_ERR xFlagErr = 0;  
	BusOsFlagGrp = OSFlagCreate(0,&xFlagErr);
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
busEventPost( eBUSEventType eEvent )
{
 //   xEventInQueue = TRUE;
 //   eQueuedEvent = eEvent;
 //   return TRUE;
	
	OS_ERR			xFlagErr;
	OS_FLAGS 		xFlag;
	
	xFlag = xFlag;
	
	xFlag = OSFlagPost(BusOsFlagGrp,eEvent,OS_FLAG_SET, &xFlagErr);

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
busEventGet( eBUSEventType * eEvent )
{
	OS_ERR		xGetFlagErr;
	OS_FLAGS 		xGetFlag;
	xGetFlag = OSFlagPend(	BusOsFlagGrp, 
							EV_BUS_INIT|EV_BUS_SET|EV_BUS_NULL, 	
							OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 
							50,
							&xGetFlagErr);
	if( xGetFlagErr == OS_ERR_NONE)
	{
		switch (xGetFlag)
		{
		case EV_BUS_INIT:
			*eEvent = EV_BUS_INIT;
			break;
		case EV_BUS_SET:
			*eEvent = EV_BUS_SET;
			break;
		case EV_BUS_NULL:
			*eEvent = EV_BUS_NULL;
			break;		
		}
		return TRUE;	
	}
	else
	{
		return FALSE;
	}
}
