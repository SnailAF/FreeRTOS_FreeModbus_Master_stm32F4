/*
 * FreeModbus Libary: user callback functions and buffer define in master mode
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
 * File: $Id: user_mb_app_m.c,v 1.60 2013/11/23 11:49:05 Armink $
 * 2022/02/01	snailAF
 * v2.0 读从节点返回数据由固有的本地寄存器缓存，改为保持至用户定义的地址
 * v2.1 修复写寄存器包错误问题。原因是如果是写则不需要数据指针，
 */
#include "mbreg_m.h"
#include "stddef.h"
#include "mb_m.h"
/*-----------------------Master mode use these variables----------------------*/

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0

/**
 * Modbus master input register callback function.
 *
 * @param pucRegBuffer input register buffer
 * @param usAddress input register address
 * @param usNRegs input register number
 *
 * @return result
 */
eMBErrorCode eMBMasterRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT *        pusSRegInBuf;

    eMBMasterGetDestDataBufferPoint(&pusSRegInBuf);
    if(pusSRegInBuf != NULL) //增加判断是存入用户指定地址，还是默认地址
    {
        /* it already plus one in modbus function method. */
        usAddress--;

        while (usNRegs > 0)
        {
            *pusSRegInBuf = *pucRegBuffer++ << 8;
            *pusSRegInBuf |= *pucRegBuffer++;
            pusSRegInBuf++;
            usNRegs--;
        }
        eMBMasterSetDestDataBufferPoint((USHORT *)NULL);
    }else
    {
    	eStatus = MB_EILLSTATE;
    }

    return eStatus;
}

/**
 * Modbus master holding register callback function.
 *
 * @param pucRegBuffer holding register buffer
 * @param usAddress holding register address
 * @param usNRegs holding register number
 * @param eMode read or write
 *
 * @return result
 */
eMBErrorCode eMBMasterRegHoldingCB(UCHAR * pucRegBuffer, USHORT usAddress,
        USHORT usNRegs, eMBRegisterMode eMode)
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT *        pusSRegHoldBuf;

    if(eMode == MB_REG_WRITE){
    	return eStatus;
    }
    eMBMasterGetDestDataBufferPoint(&pusSRegHoldBuf);
    if(pusSRegHoldBuf != NULL) //增加判断是存入用户指定地址，还是默认地址
    {				
        /* it already plus one in modbus function method. */
        usAddress--;

        while (usNRegs > 0)
        {
            *pusSRegHoldBuf = *pucRegBuffer++ << 8;
            *pusSRegHoldBuf |= *pucRegBuffer++;
            pusSRegHoldBuf++;
            usNRegs--;
        }
        eMBMasterSetDestDataBufferPoint((USHORT *)NULL);
    }else
    {
    	eStatus = MB_EILLSTATE;
    }

    return eStatus;
}

/**
 * Modbus master coils callback function.
 *
 * @param pucRegBuffer coils buffer
 * @param usAddress coils address
 * @param usNCoils coils number
 * @param eMode read or write
 *
 * @return result
 */
eMBErrorCode eMBMasterRegCoilsCB(UCHAR * pucRegBuffer, USHORT usAddress,
        USHORT usNCoils, eMBRegisterMode eMode)
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          iRegIndex , iRegBitIndex;
    UCHAR *         pucCoilBuf;

    if(eMode == MB_REG_WRITE){
    	return eStatus;
    }
    eMBMasterGetDestDataBufferPoint((USHORT **)&pucCoilBuf);
    if(pucCoilBuf != NULL) //增加判断是存入用户指定地址，还是默认地址
    {
		/* it already plus one in modbus function method. */
		usAddress--;
#if 1	//一个bit占一个字节
		uint8_t i = 0;
		for(i = 0; i < usNCoils; i++)
		{
			iRegIndex = i/8;
			iRegBitIndex = i%8;
			*(pucCoilBuf + i) =  (pucRegBuffer[iRegIndex]  & (0x01 << iRegBitIndex)) >> iRegBitIndex;
		}
#else
        iRegIndex = 0;
        iRegBitIndex = 0;
        USHORT iNReg =  usNCoils / 8 + 1;
		while (iNReg > 1)
		{
			xMBUtilSetBits(&pucCoilBuf[iRegIndex++], iRegBitIndex, 8,
					*pucRegBuffer++);
			iNReg--;
		}
		/* last coils */
		usNCoils = usNCoils % 8;
		/* xMBUtilSetBits has bug when ucNBits is zero */
		if (usNCoils != 0)
		{
			xMBUtilSetBits(&pucCoilBuf[iRegIndex++], iRegBitIndex, usNCoils,
					*pucRegBuffer++);
		}
#endif
        eMBMasterSetDestDataBufferPoint((USHORT *)NULL);
    }else
    {
    	eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

/**
 * Modbus master discrete callback function.
 *
 * @param pucRegBuffer discrete buffer
 * @param usAddress discrete address
 * @param usNDiscrete discrete number
 *
 * @return result
 */
eMBErrorCode eMBMasterRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
	eMBErrorCode eStatus = MB_ENOERR;
	USHORT iRegIndex, iRegBitIndex;
	UCHAR *pucDiscreteInputBuf;

	eMBMasterGetDestDataBufferPoint((USHORT **)&pucDiscreteInputBuf);
	if (pucDiscreteInputBuf != NULL) //增加判断是存入用户指定地址，还是默认地址
	{
		/* it already plus one in modbus function method. */
		usAddress--;
#if 1	//一个bit占一个字节
		uint8_t i = 0;
		for (i = 0; i < usNDiscrete; i++) {
			iRegIndex = i / 8;
			iRegBitIndex = i % 8;
			*(pucDiscreteInputBuf + i) = (pucRegBuffer[iRegIndex] & (0x01 << iRegBitIndex))
					>> iRegBitIndex;
		}
#else
		iRegIndex = 0;
		iRegBitIndex = 0;
		USHORT iNReg =  usNDiscrete / 8 + 1;
		/* write current discrete values with new values from the protocol stack. */
		while (iNReg > 1) {
			xMBUtilSetBits(&pucDiscreteInputBuf[iRegIndex++], iRegBitIndex, 8,
					*pucRegBuffer++);
			iNReg--;
		}
		/* last discrete */
		usNDiscrete = usNDiscrete % 8;
		/* xMBUtilSetBits has bug when ucNBits is zero */
		if (usNDiscrete != 0) {
			xMBUtilSetBits(&pucDiscreteInputBuf[iRegIndex++], iRegBitIndex,
					usNDiscrete, *pucRegBuffer++);
		}
#endif
		eMBMasterSetDestDataBufferPoint((USHORT *)NULL);
	} else {
		eStatus = MB_EILLSTATE;
	}
	return eStatus;
}
#endif
