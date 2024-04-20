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
 * File: $Id: porttimer.c,v 1.60 2013/08/13 15:07:05 Armink $
 */

/* ----------------------- Platform includes --------------------------------*/
#include <port.h>

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "freertos.h"
#include "timers.h"

static TimerHandle_t mb_timer; /* 软定时器 */
static uint32_t mb_timeout;
static void prvvTIMERExpiredISR (TimerHandle_t xTimer);

BOOL xMBPortTimersInit(USHORT usTim1Timerout50us)
{
	mb_timeout = pdMS_TO_TICKS(50 * usTim1Timerout50us/1000 +1);
	mb_timer = xTimerCreate( "mb_time",       // Just a text name, not used by the kernel.
							mb_timeout,   // The timer period in ticks.
							pdTRUE,        // The timers will auto-reload themselves when they expire.
							( void * ) 0,  // Assign each timer a unique id equal to its array index.
							prvvTIMERExpiredISR // Each timer calls the same callback when it expires.
							  );
	return TRUE;
}

void vMBPortTimersEnable()
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if ((__get_IPSR() != 0U)) {
		xTimerStartFromISR(mb_timer,&xHigherPriorityTaskWoken);
	}else{
		xTimerStart(mb_timer,1);
	}
}

void vMBPortTimersDisable()
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if ((__get_IPSR() != 0U)) {
		xTimerStopFromISR(mb_timer,&xHigherPriorityTaskWoken);
	}else{
		xTimerStop(mb_timer,1);
	}
}
void prvvTIMERExpiredISR (TimerHandle_t xTimer){
	pxMBPortCBTimerExpired();
}
