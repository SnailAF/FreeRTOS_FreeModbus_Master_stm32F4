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
 * File: $Id: portserial.c,v 1.60 2013/08/13 15:07:05 Armink $
 */

#include "port.h"
#include "usart.h"
/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- Static variables ---------------------------------*/
UCHAR ucGIEWasEnabled = FALSE;
UCHAR ucCriticalNesting = 0x00;

/* ----------------------- Defines ------------------------------------------*/
#define mbUART huart2

/* ----------------------- static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBPortSerialInit(UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits,
		eMBParity eParity) {
	/**
	 * set 485 mode receive and transmit control IO
	 * @note MODBUS_SLAVE_RT_CONTROL_PIN_INDEX need be defined by user
	 */
	BOOL bInitialized = TRUE;

	mbUART.Instance = USART2;
	mbUART.Init.BaudRate = ulBaudRate;

	switch (eParity) {
	case MB_PAR_NONE:
		mbUART.Init.Parity = UART_PARITY_NONE;
		break;
	case MB_PAR_ODD:
		mbUART.Init.Parity = UART_PARITY_ODD;
		break;
	case MB_PAR_EVEN:
		mbUART.Init.Parity = UART_PARITY_EVEN;
		break;
	}
	switch (ucDataBits) {
	case 8:
		if (eParity == MB_PAR_NONE)
			mbUART.Init.WordLength = UART_WORDLENGTH_8B;
		else
			mbUART.Init.WordLength = UART_WORDLENGTH_9B;
		break;
	case 7:
		break;
	default:
		bInitialized = FALSE;
	}
	if (bInitialized) {
		ENTER_CRITICAL_SECTION();
		mbUART.Init.StopBits = UART_STOPBITS_1;
		mbUART.Init.Mode = UART_MODE_TX_RX;
		mbUART.Init.HwFlowCtl = UART_HWCONTROL_NONE;
		mbUART.Init.OverSampling = UART_OVERSAMPLING_16;
		EXIT_CRITICAL_SECTION();
	}
	if (HAL_UART_Init(&mbUART) != HAL_OK) {
		Error_Handler();
	}
	__HAL_UART_ENABLE_IT(&mbUART, UART_IT_RXNE);
	return bInitialized;
}


void vMBPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable) {
	ENTER_CRITICAL_SECTION();
	if (xRxEnable) {
		__HAL_UART_ENABLE_IT(&mbUART, UART_IT_RXNE);
	} else {
		__HAL_UART_DISABLE_IT(&mbUART, UART_IT_RXNE);
	}
	if (xTxEnable) {
		__HAL_UART_ENABLE_IT(&mbUART, UART_IT_TC);
	} else {
		__HAL_UART_DISABLE_IT(&mbUART, UART_IT_TC);
	}
	EXIT_CRITICAL_SECTION();
}

void vMBPortClose(void) {
	HAL_UART_MspDeInit(&mbUART);
}

BOOL xMBPortSerialPutByte(CHAR ucByte) {
	mbUART.Instance->DR = (uint8_t) ucByte;
	return TRUE;
}

BOOL xMBPortSerialGetByte(CHAR *pucByte) {
	*pucByte = (uint8_t) (mbUART.Instance->DR & (uint8_t) 0x00FF);
	return TRUE;
}

/*
 * Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call
 * xMBPortSerialPutByte( ) to send the character.
 */
void prvvUARTTxReadyISR(void) {
	pxMBFrameCBTransmitterEmpty();
}

/*
 * Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
void prvvUARTRxISR(void) {
	pxMBFrameCBByteReceived();
}

void USART2_IRQHandler(void)
{
	if (__HAL_UART_GET_IT_SOURCE(&mbUART, UART_IT_TC)) {
		if (__HAL_UART_GET_FLAG(&mbUART, UART_FLAG_TC)) {
			pxMBFrameCBTransmitterEmpty();
		}
	}
	if (__HAL_UART_GET_IT_SOURCE(&mbUART, UART_IT_RXNE)) {
		if (__HAL_UART_GET_FLAG(&mbUART, UART_FLAG_RXNE)) {
			pxMBFrameCBByteReceived();
		}
	}
}
