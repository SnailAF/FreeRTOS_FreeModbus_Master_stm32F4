/* 
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2006-2018 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* ----------------------- System includes ----------------------------------*/
#include <port.h>
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "mb.h"
#include "mbframe.h"
#include "mbproto.h"
#include "mbconfig.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_PDU_FUNC_READ_ADDR_OFF        ( MB_PDU_DATA_OFF )
#define MB_PDU_FUNC_READ_DEVID_OFF       ( MB_PDU_DATA_OFF + 2 )
#define MB_PDU_FUNC_READ_SIZE            ( 3 )
#define MB_PDU_FUNC_READ_DEVID_MAX       ( 0x007D )

#define MB_PDU_FUNC_READ_RSP_BYTECNT_OFF    ( MB_PDU_DATA_OFF )

/* ----------------------- Static functions ---------------------------------*/
eMBException    prveMBError2Exception( eMBErrorCode eErrorCode );

/* ----------------------- Start implementation -----------------------------*/

#if MB_FUNC_READ_DEVICE_ID_ENABLED > 0
eMBException
eMBFuncReadDeviceIdentification( UCHAR * pucFrame, USHORT * usLen )
{
    //USHORT          usRegCount;
    UCHAR          *pucFrameCur;
    UCHAR			ReadDevId;
    UCHAR			ObjectId;
//    UCHAR			ConformityLevel;
//    UCHAR			MoreFollows;
//    UCHAR			NextObjectId;
//    UCHAR           NumberOfObjects;
    eMBException    eStatus = MB_EX_NONE;
    eMBErrorCode    eRegStatus;

    if( *usLen == ( MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN )  && pucFrame[MB_PDU_FUNC_READ_ADDR_OFF] == 0x0E)
    {
		ReadDevId = pucFrame[MB_PDU_FUNC_READ_ADDR_OFF + 1];
		ObjectId  = pucFrame[MB_PDU_FUNC_READ_ADDR_OFF + 2];

		/* Check if the number of registers to read is valid. If not
		 * return Modbus illegal data value exception.
		 */
		if( ReadDevId < 0x5 )
		{
			/* Set the current PDU data pointer to the beginning. */
			pucFrameCur = &pucFrame[MB_PDU_FUNC_OFF];
			*usLen = MB_PDU_FUNC_OFF;

			/* First byte contains the function code. */
			*pucFrameCur++ = MB_FUNC_READ_DEVICE_ID;
			*usLen += 1;

			/* Second byte MEI Type* fixed 0x0e  */
			*pucFrameCur++ = 0X0E;
			*usLen += 1;

			/* Third byte Read Device ID */
			*pucFrameCur++ = ReadDevId;
			*usLen += 1;

			/* Fourth byte Conformity Level equal ReadDevID */
			*pucFrameCur++ = ReadDevId;
			*usLen += 1;

			eRegStatus =
				eMBReadDeviceIdCB( pucFrameCur, ReadDevId,ObjectId, usLen );

			/* If an error occured convert it into a Modbus exception. */
			if( eRegStatus != MB_ENOERR )
			{
				eStatus = prveMBError2Exception( eRegStatus );
			}
		}
		else
		{
			eStatus = MB_EX_ILLEGAL_DATA_VALUE;
		}
    }
    else
    {
        /* Can't be a valid read input register request because the length
         * is incorrect. */
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    return eStatus;
}

#endif
