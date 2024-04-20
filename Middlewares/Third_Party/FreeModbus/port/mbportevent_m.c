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
#include "freertos.h"
#include "event_groups.h"
#include "semphr.h"


#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0
/* ----------------------- Defines ------------------------------------------*/
/* ----------------------- Variables ----------------------------------------*/
static SemaphoreHandle_t   xMasterRunRes = NULL;
static EventGroupHandle_t  xMasterOsEvent = NULL;
static EventGroupHandle_t  xMasterOsWaitRequestFinishEvent = NULL;


/* ----------------------- Start implementation -----------------------------*/
/**
  *****************************************************************************
  * @brief  创建两个事件组，一个为运行状态，一个为完成返回状态。
  * @note
  * @param None
  * @retval None
  * @author
  * @date
  *****************************************************************************
  */
BOOL
xMBMasterPortEventInit( void )
{
    xMasterOsEvent = xEventGroupCreate();
    xMasterOsWaitRequestFinishEvent = xEventGroupCreate();
    return TRUE;
}

/**
  *****************************************************************************
  * @brief  定时器调用，改变状态。
  * @note
  * @param None
  * @retval None
  * @author
  * @date
  *****************************************************************************
  */
BOOL
xMBMasterPortEventPostFromISR( eMBMasterEventType eEvent )
{
    BaseType_t  xResult;
    BaseType_t  xHigherPriorityTaskWoken = pdFALSE;
    xResult = xEventGroupSetBitsFromISR( xMasterOsEvent,	( EventBits_t )eEvent, &xHigherPriorityTaskWoken );
    if( xResult != pdFAIL )
    {
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
    return TRUE;
}

/**
  *****************************************************************************
  * @brief  普通函数调用，改变状态。
  * @note
  * @param None
  * @retval None
  * @author
  * @date
  *****************************************************************************
  */
BOOL
xMBMasterPortEventPost( eMBMasterEventType eEvent )
{
    BaseType_t  xResult;
    BaseType_t  xHigherPriorityTaskWoken = pdFALSE;
    if( eEvent == EV_MASTER_FRAME_SENT )
    {
        xEventGroupClearBits( xMasterOsWaitRequestFinishEvent,
                              EV_MASTER_PROCESS_SUCESS | EV_MASTER_ERROR_RESPOND_TIMEOUT | EV_MASTER_ERROR_RECEIVE_DATA | EV_MASTER_ERROR_EXECUTE_FUNCTION );
    }
    if ((__get_IPSR() != 0U)) {
		xResult = xEventGroupSetBitsFromISR( xMasterOsEvent,	( EventBits_t )eEvent, &xHigherPriorityTaskWoken );
		if( xResult != pdFAIL )
		{
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		}
    }else{
    	xEventGroupSetBits( xMasterOsEvent,	( EventBits_t )eEvent );
    }
    return TRUE;
}

/**
  *****************************************************************************
  * @brief  获取当前运行状态
  * @note
  * @param None
  * @retval None
  * @author
  * @date
  *****************************************************************************
  */
BOOL
xMBMasterPortEventGet( eMBMasterEventType * eEvent )
{
    uint32_t  recvedEvent;
    /* waiting forever OS event */
    recvedEvent = xEventGroupWaitBits( xMasterOsEvent,
                                       EV_MASTER_READY | EV_MASTER_FRAME_RECEIVED | EV_MASTER_EXECUTE | EV_MASTER_FRAME_SENT | EV_MASTER_ERROR_PROCESS,
                                       pdTRUE,
                                       pdFALSE,
                                       10000 );

    /* the enum type couldn't convert to int type */
    if(recvedEvent == 0){
    	vMBMasterRunResRelease( );
    }else if( recvedEvent & EV_MASTER_READY ){
        *eEvent = EV_MASTER_READY;
    }else if( recvedEvent & EV_MASTER_FRAME_RECEIVED ){
        *eEvent = EV_MASTER_FRAME_RECEIVED;
    }else if( recvedEvent & EV_MASTER_EXECUTE ){
        *eEvent = EV_MASTER_EXECUTE;
    }else if( recvedEvent & EV_MASTER_FRAME_SENT ){
        *eEvent = EV_MASTER_FRAME_SENT;
    }else if( recvedEvent & EV_MASTER_ERROR_PROCESS ){
        *eEvent = EV_MASTER_ERROR_PROCESS;
    }
    return TRUE;
}
/**
 * This function is initialize the OS resource for modbus master.
 * Note:The resource is define by OS.If you not use OS this function can be empty.
 *
 */
void vMBMasterOsResInit( void )
{
    xMasterRunRes = xSemaphoreCreateBinary();
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
    if( xSemaphoreTake( xMasterRunRes, lTimeOut ) == pdTRUE ){
        return TRUE;
    }else{
        return FALSE;
    }
}

/**
 * This function is release Mobus Master running resource.
 * Note:The resource is define by Operating System.If you not use OS this function can be empty.
 *
 */
void vMBMasterRunResRelease( void )
{
    /* release resource */
    xSemaphoreGive( xMasterRunRes );
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
void vMBMasterErrorCBRespondTimeout( UCHAR ucDestAddress, const UCHAR* pucPDUData,
                                     USHORT ucPDULength )
{
    /**
     * @note This code is use OS's event mechanism for modbus master protocol stack.
     * If you don't use OS, you can change it.
     */
    xEventGroupSetBits( xMasterOsWaitRequestFinishEvent, EV_MASTER_ERROR_RESPOND_TIMEOUT );
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
void vMBMasterErrorCBReceiveData( UCHAR ucDestAddress, const UCHAR* pucPDUData,
                                  USHORT ucPDULength )
{
    /**
     * @note This code is use OS's event mechanism for modbus master protocol stack.
     * If you don't use OS, you can change it.
     */
    xEventGroupSetBits( xMasterOsWaitRequestFinishEvent, EV_MASTER_ERROR_RECEIVE_DATA );
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
void vMBMasterErrorCBExecuteFunction( UCHAR ucDestAddress, const UCHAR* pucPDUData,
                                      USHORT ucPDULength )
{
    /**
     * @note This code is use OS's event mechanism for modbus master protocol stack.
     * If you don't use OS, you can change it.
     */
    xEventGroupSetBits( xMasterOsWaitRequestFinishEvent, EV_MASTER_ERROR_EXECUTE_FUNCTION );
    /* You can add your code under here. */

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
    xEventGroupSetBits( xMasterOsWaitRequestFinishEvent, EV_MASTER_PROCESS_SUCESS );
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
    uint32_t  recvedEvent;
    /* waiting for OS event */
    recvedEvent = xEventGroupWaitBits( xMasterOsWaitRequestFinishEvent,
                                       EV_MASTER_PROCESS_SUCESS | EV_MASTER_ERROR_RESPOND_TIMEOUT | EV_MASTER_ERROR_RECEIVE_DATA | EV_MASTER_ERROR_EXECUTE_FUNCTION,
                                       pdTRUE,
                                       pdFALSE,
									   MB_MASTER_TIMEOUT_MS_RESPOND + 10);

    switch( recvedEvent )
    {
    case EV_MASTER_PROCESS_SUCESS:
        break;
    case EV_MASTER_ERROR_RESPOND_TIMEOUT:
        eErrStatus = MB_MRE_TIMEDOUT;
        break;
    case EV_MASTER_ERROR_RECEIVE_DATA:
        eErrStatus = MB_MRE_REV_DATA;
        break;
    case EV_MASTER_ERROR_EXECUTE_FUNCTION:
        eErrStatus = MB_MRE_EXE_FUN;
        break;
    default:
        assert_param( eErrStatus );
        vMBMasterRunResRelease();
        eErrStatus = MB_MRE_TIMEDOUT;
        break;
    }
    return eErrStatus;
}

#endif
