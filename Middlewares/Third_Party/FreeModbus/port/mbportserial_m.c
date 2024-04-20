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
 * File: $Id: portserial_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions $
 */

#include "port.h"
#include "usart.h"
#include "stm32f4xx_hal_uart.h"
/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* -----------------------Config Defines -------------------------------------*/
#define mbmUART huart1

/* ----------------------- Defines ------------------------------------------*/

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0
/* ----------------------- Static variables ---------------------------------*/

/* ----------------- ------ Defines ------------------------------------------*/
/* serial transmit event */
#define EVENT_SERIAL_TRANS_START    (1<<0)

/* ----------------------- static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortSerialInit(UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits,
		eMBParity eParity) {
	/**
	 * set 485 mode receive and transmit control IO
	 * @note MODBUS_SLAVE_RT_CONTROL_PIN_INDEX need be defined by user
	 */
	BOOL bInitialized = TRUE;

	mbmUART.Instance = USART1;
	mbmUART.Init.BaudRate = ulBaudRate;

	switch (eParity) {
	case MB_PAR_NONE:
		mbmUART.Init.Parity = UART_PARITY_NONE;
		break;
	case MB_PAR_ODD:
		mbmUART.Init.Parity = UART_PARITY_ODD;
		break;
	case MB_PAR_EVEN:
		mbmUART.Init.Parity = UART_PARITY_EVEN;
		break;
	}
	switch (ucDataBits) {
	case 8:
		if (eParity == MB_PAR_NONE)
			mbmUART.Init.WordLength = UART_WORDLENGTH_8B;
		else
			mbmUART.Init.WordLength = UART_WORDLENGTH_9B;
		break;
	case 7:
		break;
	default:
		bInitialized = FALSE;
	}
	if (bInitialized) {
		ENTER_CRITICAL_SECTION();
		mbmUART.Init.StopBits = UART_STOPBITS_1;
		mbmUART.Init.Mode = UART_MODE_TX_RX;
		mbmUART.Init.HwFlowCtl = UART_HWCONTROL_NONE;
		mbmUART.Init.OverSampling = UART_OVERSAMPLING_16;
		EXIT_CRITICAL_SECTION();
	}
	if (HAL_UART_Init(&mbmUART) != HAL_OK) {
		Error_Handler();
	}
	__HAL_UART_ENABLE_IT(&mbmUART, UART_IT_RXNE);

	return bInitialized;
}

void vMBMasterPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable) {
	ENTER_CRITICAL_SECTION();
	if (xRxEnable) {
		__HAL_UART_CLEAR_FLAG(&mbmUART, UART_IT_RXNE);
		__HAL_UART_ENABLE_IT(&mbmUART, UART_IT_RXNE);
	} else {
		__HAL_UART_DISABLE_IT(&mbmUART, UART_IT_RXNE);
	}
	if (xTxEnable) {
		__HAL_UART_ENABLE_IT(&mbmUART, UART_IT_TC);
	} else {
		__HAL_UART_DISABLE_IT(&mbmUART, UART_IT_TC);
	}
	EXIT_CRITICAL_SECTION();
}

void vMBMasterPortClose(void) {
	__HAL_UART_DISABLE(&mbmUART);
}

BOOL xMBMasterPortSerialPutByte(CHAR ucByte) {
	mbmUART.Instance->DR = (uint8_t) ucByte;
	return TRUE;
}

BOOL xMBMasterPortSerialGetByte(CHAR *pucByte) {
	*pucByte = (uint8_t) (mbmUART.Instance->DR & (uint8_t) 0x00FF);
	return TRUE;
}
void USART1_IRQHandler(void)
{
	if (__HAL_UART_GET_IT_SOURCE(&mbmUART, UART_IT_TC)) {
		if (__HAL_UART_GET_FLAG(&mbmUART, UART_FLAG_TC)) {
			pxMBMasterFrameCBTransmitterEmpty();
		}
	} else if (__HAL_UART_GET_IT_SOURCE(&mbmUART, UART_IT_RXNE)) {
		if (__HAL_UART_GET_FLAG(&mbmUART, UART_FLAG_RXNE)) {
			pxMBMasterFrameCBByteReceived();
		}
	} else {
		__HAL_UART_CLEAR_PEFLAG(&mbmUART);
	}
}
#endif
