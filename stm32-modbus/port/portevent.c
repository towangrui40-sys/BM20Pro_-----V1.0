/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: portevent.c,v 1.1 2006/08/22 21:35:13 wolti Exp $
 */

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- Defines ------------------------------------------*/
#define OS_ERR INT8U

/* ----------------------- Variables ----------------------------------------*/
//static eMBEventType eQueuedEvent;
//static BOOL     xEventInQueue;
//static struct OS_EVENT   *  xSlaveOsEvent;

static OS_FLAG_GRP 	* xSlaveOsFlagGrp=0;
//static OS_EVENT 	*	xSlaveOsEvent=0;

/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBPortEventInit( void )
{
	OS_ERR xFlagErr = 0;  
	xSlaveOsFlagGrp = OSFlagCreate(0,&xFlagErr);
	if(xFlagErr == OS_ERR_NONE)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
	//xEventInQueue = FALSE;//old
    //return TRUE;
}

BOOL
xMBPortEventPost( eMBEventType eEvent )
{
 //   xEventInQueue = TRUE;
 //   eQueuedEvent = eEvent;
 //   return TRUE;
	
	OS_ERR			xFlagErr;
	OS_FLAGS 		xFlag;
	
	xFlag = xFlag;
	
	xFlag = OSFlagPost(xSlaveOsFlagGrp,eEvent,OS_FLAG_SET, &xFlagErr);

	if(xFlagErr == OS_ERR_NONE)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL
xMBPortEventGet( eMBEventType * eEvent )
{
//    BOOL            xEventHappened = FALSE;

//    if( xEventInQueue )
//    {
//        *eEvent = eQueuedEvent;
//        xEventInQueue = FALSE;
//        xEventHappened = TRUE;
//    }
//    return xEventHappened;
	OS_ERR		xGetFlagErr;
	OS_FLAGS 		xGetFlag;
	xGetFlag = OSFlagPend(	xSlaveOsFlagGrp, 
							EV_READY|EV_FRAME_RECEIVED|EV_EXECUTE|EV_FRAME_SENT, 	
							OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 
							10,
							&xGetFlagErr);
	if( xGetFlagErr == OS_ERR_NONE)
	{
		switch (xGetFlag)
		{
		case EV_READY:
			*eEvent = EV_READY;
			break;
		case EV_FRAME_RECEIVED:
			*eEvent = EV_FRAME_RECEIVED;
			break;
		case EV_EXECUTE:
			*eEvent = EV_EXECUTE;
			break;
		case EV_FRAME_SENT:
			*eEvent = EV_FRAME_SENT;
			break;
		}
		return TRUE;	
	}
	else
	{
		return FALSE;
	}
}
