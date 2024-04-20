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
 * File: $Id: porttimer_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions$
 */

/* ----------------------- Platform includes --------------------------------*/
#include <port.h>
#include "mb.h"
#include "mb_m.h"
#include "mbport.h"
#include "freertos.h"
#include "timers.h"

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0


static TimerHandle_t mbm_timer; /* 软定时器 */
static uint32_t mbm_t35timeout;

/* ----------------------- static functions ---------------------------------*/
static void mbmprvTIMERExpiredISR(TimerHandle_t xTimer);

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortTimersInit(USHORT usTimeOut50us)
{
	mbm_t35timeout = pdMS_TO_TICKS(50 * usTimeOut50us/1000 +1);
	mbm_timer = xTimerCreate( "mbm_time",       // Just a text name, not used by the kernel.
				mbm_t35timeout,   // The timer period in ticks.
				pdTRUE,        // The timers will auto-reload themselves when they expire.
				( void * ) 0,  // Assign each timer a unique id equal to its array index.
				mbmprvTIMERExpiredISR // Each timer calls the same callback when it expires.
			  	  );
	return TRUE;
}

void vMBMasterPortTimersT35Enable()
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	vMBMasterSetCurTimerMode(MB_TMODE_T35);
	if ((__get_IPSR() != 0U)) {

		if(xTimerChangePeriodFromISR(mbm_timer, mbm_t35timeout, &xHigherPriorityTaskWoken) != pdPASS){
		}
		if(xTimerStartFromISR(mbm_timer,&xHigherPriorityTaskWoken) != pdPASS){
		}
	}else{
		xTimerChangePeriod(mbm_timer,mbm_t35timeout,2);
		xTimerStart(mbm_timer,2);
	}
}

void vMBMasterPortTimersConvertDelayEnable()
{
    uint32_t timer_tick = pdMS_TO_TICKS(MB_MASTER_DELAY_MS_CONVERT);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//    /* Set current timer mode, don't change it.*/
    vMBMasterSetCurTimerMode(MB_TMODE_CONVERT_DELAY);

	if ((__get_IPSR() != 0U)) {
		if(xTimerChangePeriodFromISR(mbm_timer, timer_tick, &xHigherPriorityTaskWoken) != pdPASS){
		}
		if(xTimerStartFromISR(mbm_timer,&xHigherPriorityTaskWoken) != pdPASS){
		}
	}else{
		xTimerChangePeriod(mbm_timer,timer_tick,2);
		xTimerStart(mbm_timer,2);
	}
}

void vMBMasterPortTimersRespondTimeoutEnable()
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	  uint32_t timer_tick = pdMS_TO_TICKS(MB_MASTER_TIMEOUT_MS_RESPOND);
    /* Set current timer mode, don't change it.*/
    vMBMasterSetCurTimerMode(MB_TMODE_RESPOND_TIMEOUT);

	if ((__get_IPSR() != 0U)) {
		if(xTimerChangePeriodFromISR(mbm_timer, timer_tick, &xHigherPriorityTaskWoken) != pdPASS){
		}
		if(xTimerStartFromISR(mbm_timer,&xHigherPriorityTaskWoken) != pdPASS){
		}
	}else{
		xTimerChangePeriod(mbm_timer,timer_tick,2);
		xTimerStart(mbm_timer,2);
	}
}

void vMBMasterPortTimersDisable()
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if ((__get_IPSR() != 0U)) {
		if(xTimerStopFromISR(mbm_timer,&xHigherPriorityTaskWoken) != pdPASS){
		}
	}else{
		xTimerStop(mbm_timer,2);
	}
}

void mbmprvTIMERExpiredISR(TimerHandle_t xTimer)
{
    (void) pxMBMasterPortCBTimerExpired();
}

#endif
