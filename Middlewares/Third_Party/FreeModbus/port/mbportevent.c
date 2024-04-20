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
 * File: $Id: portevent.c,v 1.60 2013/08/13 15:07:05 Armink $
 */

/* 使用freertos 任务事件 */
/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "freertos.h"
#include "event_groups.h"
/* ----------------------- Variables ----------------------------------------*/
static EventGroupHandle_t xSlaveOsEvent = NULL;
/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBPortEventInit( void )
{
	xSlaveOsEvent = xEventGroupCreate();
    return TRUE;
}

BOOL
xMBPortEventPost( eMBEventType eEvent )
{
	BaseType_t xResult;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xResult = xEventGroupSetBitsFromISR(xSlaveOsEvent, 
										(uint32_t)eEvent,
										&xHigherPriorityTaskWoken);  //中断调用
	
	/* 消息被成功发出 */
	if( xResult != pdFAIL )
	{
		/* 如果 xHigherPriorityTaskWoken = pdTRUE，那么退出中断后切到当前最高优先级任务执行 */
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
    return TRUE;
}

BOOL
xMBPortEventGet( eMBEventType * eEvent )
{
	uint32_t recvedEvent;
	recvedEvent = xEventGroupWaitBits(xSlaveOsEvent, 	
										EV_READY | EV_FRAME_RECEIVED | EV_EXECUTE | EV_FRAME_SENT, 	
										pdTRUE,			//清零对应位
										pdFALSE,		//只要有任何一个事件就返回
										portMAX_DELAY); /* 设置事件 */
    switch (recvedEvent)
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
