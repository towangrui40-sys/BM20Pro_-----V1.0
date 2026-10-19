/*
 * FreeModbus Libary: RT-Thread Port
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
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
 * File: $Id: portevent_m.c v 1.60 2013/08/13 15:07:05 Armink add Master Functions$
 */

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mb_m.h"
#include "mbport.h"
#include "port.h"

#if ( (MB_MASTER_RTU_ENABLED | MB_MASTER_ASCII_ENABLED) > 0)
/* ----------------------- Defines ------------------------------------------*/
#define OS_ERR INT8U
/* ----------------------- Variables ----------------------------------------*/

static OS_FLAG_GRP 	* xMasterOsFlagGrp=0;
static OS_EVENT 	*	xMasterRunRes=0;

/* ----------------------- Start implementation -----------------------------*/
//BOOL
//xMBMasterPortEventInit( void )
//{
//    xMasterEventInQueue = FALSE;
//    return TRUE;
//}
//BOOL
//xMBMasterPortEventPost( eMBMasterEventType eEvent )
//{
//    xMasterEventInQueue = TRUE;
//    eMasterQueuedEvent = eEvent;
//    return TRUE;
//}
//BOOL
//xMBMasterPortEventGet( eMBMasterEventType * eEvent )
//{
//    BOOL            xEventHappened = FALSE;

//    if( xMasterEventInQueue )
//    {
//        *eEvent = eMasterQueuedEvent;
//        xMasterEventInQueue = FALSE;
//        xEventHappened = TRUE;
//    }
//    return xEventHappened;
//}
BOOL
xMBMasterPortEventInit( void )
{
	OS_ERR xFlagErr = 0; 
	
	//OSFlagCreate(&xMasterOsFlagGrp, "Master Flag Grp", 0,&xFlagErr);
	xMasterOsFlagGrp = OSFlagCreate(0,&xFlagErr);
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
xMBMasterPortEventPost( eMBMasterEventType eEvent )
{
	OS_ERR			xFlagErr;
	OS_FLAGS 		xFlag;
	
	xFlag = xFlag;
	
	xFlag = OSFlagPost(xMasterOsFlagGrp,eEvent,OS_FLAG_SET, &xFlagErr);

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
xMBMasterPortEventGet( eMBMasterEventType * eEvent )
{

	OS_ERR		xGetFlagErr;
	OS_FLAGS 		xGetFlag;
	
	
	xGetFlag = OSFlagPend(	xMasterOsFlagGrp, 
							EV_MASTER_READY|EV_MASTER_FRAME_RECEIVED|EV_MASTER_EXECUTE|EV_MASTER_FRAME_SENT|EV_MASTER_ERROR_PROCESS, 	
							OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 
							10,
							&xGetFlagErr);
	
	if( xGetFlagErr == OS_ERR_NONE)
	{
		switch ( xGetFlag )
		{
			case EV_MASTER_READY:
				*eEvent = EV_MASTER_READY;
				break;
			case EV_MASTER_FRAME_RECEIVED:
				*eEvent = EV_MASTER_FRAME_RECEIVED;
				break;
			case EV_MASTER_EXECUTE:
				*eEvent = EV_MASTER_EXECUTE;
				break;
			case EV_MASTER_FRAME_SENT:
				*eEvent = EV_MASTER_FRAME_SENT;
				break;
			case EV_MASTER_ERROR_PROCESS:
				*eEvent = EV_MASTER_ERROR_PROCESS;
				break;
		}
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/**
 * This is modbus master request process success callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 */
void vMBMasterCBRequestScuuess( void ) 
{
    /**
     * @note This code is use OS's event mechanism for modbus master protocol stack.
     * If you don't use OS, you can change it.
     */

	
	OS_ERR			xFlagErr;
	OS_FLAGS 		xFlag;
	xFlag = xFlag;
	
	xFlag = OSFlagPost(xMasterOsFlagGrp,EV_MASTER_PROCESS_SUCESS,OS_FLAG_SET, &xFlagErr);

    /* You can add your code under here. */

}

/**
 * This function is wait for modbus master request finish and return result.
 * Waiting result include request process success, request respond timeout,
 * receive data error and execute function error.You can use the above callback function.
 * @note If you are use OS, you can use OS's event mechanism. Otherwise you have to run
 * much user custom delay for waiting.
 *
 * @return request error code
 */



eMBMasterReqErrCode eMBMasterWaitRequestFinish( void ) 
{
    eMBMasterReqErrCode    eErrStatus = MB_MRE_NO_ERR;
	OS_ERR			xFlagErr;
	OS_FLAGS 		xFinishFlag;	


	xFinishFlag = OSFlagPend(	xMasterOsFlagGrp, 
								EV_MASTER_PROCESS_SUCESS|EV_MASTER_ERROR_RESPOND_TIMEOUT|EV_MASTER_ERROR_RECEIVE_DATA|EV_MASTER_ERROR_EXECUTE_FUNCTION,								
	                            OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 
								0,
								&xFlagErr);		
	
    switch (xFinishFlag)
    {
		case EV_MASTER_PROCESS_SUCESS:
			break;
		case EV_MASTER_ERROR_RESPOND_TIMEOUT:
		{
			eErrStatus = MB_MRE_TIMEDOUT;
			break;
		}
		case EV_MASTER_ERROR_RECEIVE_DATA:
		{
			eErrStatus = MB_MRE_REV_DATA;
			break;
		}
		case EV_MASTER_ERROR_EXECUTE_FUNCTION:
		{
			eErrStatus = MB_MRE_EXE_FUN;
			break;
		}
    }
    return eErrStatus;
}



/**
 * This is modbus master respond timeout error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
void vMBMasterErrorCBRespondTimeout(UCHAR ucDestAddress, const UCHAR* pucPDUData,
        USHORT ucPDULength) {
    /**
     * @note This code is use OS's event mechanism for modbus master protocol stack.
     * If you don't use OS, you can change it.
     */

	OS_ERR			xFlagErr;
	OS_FLAGS 		xFlag;
	xFlag = xFlag;	
	xFlag = OSFlagPost( xMasterOsFlagGrp,
						EV_MASTER_ERROR_RESPOND_TIMEOUT,
						OS_FLAG_SET, 
						&xFlagErr);			

    /* You can add your code under here. */

}

/**
 * This is modbus master receive data error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
void vMBMasterErrorCBReceiveData(UCHAR ucDestAddress, const UCHAR* pucPDUData,
        USHORT ucPDULength) {
    /**
     * @note This code is use OS's event mechanism for modbus master protocol stack.
     * If you don't use OS, you can change it.
     */
			
	OS_ERR			xFlagErr;
	OS_FLAGS 		xFlag;
	xFlag = xFlag;	
	xFlag = OSFlagPost(	xMasterOsFlagGrp, 
						EV_MASTER_ERROR_RECEIVE_DATA, 
						OS_FLAG_SET, 
						&xFlagErr);	

    /* You can add your code under here. */

}

/**
 * This is modbus master execute function error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
void vMBMasterErrorCBExecuteFunction(UCHAR ucDestAddress, const UCHAR* pucPDUData,
        USHORT ucPDULength) {
    /**
     * @note This code is use OS's event mechanism for modbus master protocol stack.
     * If you don't use OS, you can change it.
     */
			
	OS_ERR			xFlagErr;
	OS_FLAGS 		xFlag;
	xFlag = xFlag;	
	xFlag = OSFlagPost(	xMasterOsFlagGrp, 
						EV_MASTER_ERROR_EXECUTE_FUNCTION, 
						OS_FLAG_SET, 
						&xFlagErr);			

    /* You can add your code under here. */

}

/**
 * This function is initialize the OS resource for modbus master.
 * Note:The resource is define by OS.If you not use OS this function can be empty.
 *
 */
void vMBMasterOsResInit( void )
{
	OS_ERR			xFlagErr;
	xFlagErr =xFlagErr;
	xMasterRunRes = OSSemCreate(1);
//	if(xFlagErr == OS_ERR_NONE)
//	{
//		printf("OSSemCreate OK!\r\n");
//	}
//	else
//	{
//		printf("OSSemCreate OK!\r\n");
//	}
	
}

/**
 * This function is take Mobus Master running resource.
 * Note:The resource is define by Operating System.If you not use OS this function can be just return TRUE.
 *
 * @param lTimeOut the waiting time.
 *
 * @return resource taked result
 */
BOOL xMBMasterRunResTake( LONG lTimeOut )
{
    /*If waiting time is -1 .It will wait forever */

	OS_ERR			xFlagErr;
	//CPU_TS			MasterTS;
	
	OSSemPend(xMasterRunRes, lTimeOut, &xFlagErr);
	
	if(xFlagErr == OS_ERR_NONE)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
//	return TRUE;
}

/**
 * This function is release Mobus Master running resource.
 * Note:The resource is define by Operating System.If you not use OS this function can be empty.
 *
 */

void vMBMasterRunResRelease( void )
{
    /* release resource */
	OS_ERR			xFlagErr;
	xFlagErr = xFlagErr;
	xFlagErr=OSSemPost(xMasterRunRes);	
}


#endif

