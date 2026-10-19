#include "sys.h"
#include "includes.h"

/* ----------------------- Defines ------------------------------------------*/
#define OS_ERR INT8U


static OS_FLAG_GRP 	* pwOsFlagGrp=0;


/* ----------------------- Start implementation -----------------------------*/
unsigned char 
pwEventInit(void)
{
	OS_ERR xFlagErr = 0; 
	pwOsFlagGrp = OSFlagCreate(0,&xFlagErr);
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
pwEventPost( ePWEventType eEvent)
{
	OS_ERR			xFlagErr;
	OS_FLAGS 		xFlag;
	
	xFlag = xFlag;

	xFlag = OSFlagPost(pwOsFlagGrp,eEvent,OS_FLAG_SET, &xFlagErr);

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
pwEventGet( ePWEventType * eEvent )
{
	OS_ERR		xGetFlagErr;
	OS_FLAGS 		xGetFlag;
	xGetFlag = OSFlagPend(	pwOsFlagGrp, 
							EV_PW_NULL|EV_PW_INIT|EV_PW_SET_CUR|EV_PW_SET_RUN, 
							OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 
							50,
							&xGetFlagErr);	

	if( xGetFlagErr == OS_ERR_NONE)
	{
		switch (xGetFlag)
		{
		case EV_PW_NULL:
			*eEvent = EV_PW_NULL;
			break;
		case EV_PW_INIT:
			*eEvent = EV_PW_INIT;
			break;
		case EV_PW_SET_CUR:
			*eEvent = EV_PW_SET_CUR;
			break;
		case EV_PW_SET_RUN:
			*eEvent = EV_PW_SET_RUN;
			break;
		}
		return TRUE;	
	}
	else
	{
		return FALSE;
	}
}
