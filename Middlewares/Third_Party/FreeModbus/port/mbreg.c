
/* ----------------------- Platform includes --------------------------------*/
#include <port.h>
#include "mbreg.h"
/* ----------------------- Modbus includes ----------------------------------*/
#include <string.h>
/* content of the holding regs */
uint16_t   usSHoldBuf[REG_HOLDING_NREGS]	= {0};		//modbus 缓存40000
uint16_t   usSInBuf[REG_INPUT_NREGS] 		= {0}; 	    //输入缓存
uint8_t    usSCoilBuf[REG_COILS_NREGS] 		= {0};	    //输出IO缓存
uint8_t    usSDiscInBuf[REG_DISCRETE_NREGS] = {0};		//离散输入缓存

char VendorName[] = VENDOR_NAME;
char ProductCode[] = PRODUCT_CODE;
char MajorMinorRevision[] = MAJOR_MINOR_REVISION;


uint8_t Byte_To_Bit(uint8_t *p);
void Bit_To_Byte(uint8_t n,uint8_t *p);
eMBErrorCode
eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    /* it already plus one in modbus function method. */
    usAddress--;

    if( ( usAddress >= REG_INPUT_START )
            && ( usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - REG_INPUT_START );
        while( usNRegs > 0 )
        {
            *pucRegBuffer++ = ( unsigned char )( usSInBuf[iRegIndex] >> 8 );
            *pucRegBuffer++ = ( unsigned char )( usSInBuf[iRegIndex] & 0xFF );
            iRegIndex++;
            usNRegs--;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

eMBErrorCode
eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    /* it already plus one in modbus function method. */
    usAddress--;

    if( ( usAddress >= REG_HOLDING_START ) &&
            ( usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - REG_HOLDING_START );
        switch ( eMode )
        {
        /* Pass current register values to the protocol stack. */
        case MB_REG_READ:
            while( usNRegs > 0 )
            {
                *pucRegBuffer++ = ( unsigned char )( usSHoldBuf[iRegIndex] >> 8 );
                *pucRegBuffer++ = ( unsigned char )( usSHoldBuf[iRegIndex] & 0xFF );
                iRegIndex++;
                usNRegs--;
            }
            break;

        /* Update current register values with new values from the
         * protocol stack. */
        case MB_REG_WRITE:
            while( usNRegs > 0 )
            {
                usSHoldBuf[iRegIndex] = *pucRegBuffer++ << 8;
                usSHoldBuf[iRegIndex] |= *pucRegBuffer++;
                iRegIndex++;
                usNRegs--;
            }
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}


eMBErrorCode
eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT             iRegIndex, iNReg;

    iNReg =  usNCoils / 8 + 1;		//8个coils占用一个字节。余量加1

    /* it already plus one in modbus function method. */
    usAddress--;

    if( ( usAddress >= REG_COILS_START ) &&
            ( usAddress + usNCoils <= REG_COILS_START + REG_COILS_NREGS ) )
    {
        iRegIndex = ( USHORT )( usAddress - REG_COILS_START );
        switch ( eMode )
        {
        /* Pass current register values to the protocol stack. */
        case MB_REG_READ:
            while( iNReg > 0 )
            {
                *pucRegBuffer++ = Byte_To_Bit(&usSCoilBuf[iRegIndex]); //线圈寄存器用1bit代表1个coils。
                iNReg--;
                iRegIndex += 8;
            }
            pucRegBuffer--;
            /* last coils */
            usNCoils = usNCoils % 8;
            /* filling zero to high bit */
            *pucRegBuffer = *pucRegBuffer << (8 - usNCoils);
            *pucRegBuffer = *pucRegBuffer >> (8 - usNCoils);
            break;

        /* Update current register values with new values from the
         * protocol stack. */
        case MB_REG_WRITE:
            while (iNReg > 1)
            {
				Bit_To_Byte(*pucRegBuffer++,&usSCoilBuf[iRegIndex]);
                iNReg--;
				iRegIndex += 8;
            }
            /* last coils */
            usNCoils = usNCoils % 8;
			if(usNCoils > 0)
			{
				for(UCHAR i=0;i<usNCoils;i++)
					usSCoilBuf[iRegIndex++] = (*pucRegBuffer & (0x01 << i)) >> i;
			}
            break;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}

/**
 * Modbus slave discrete callback function.
 *
 * @param pucRegBuffer discrete buffer
 * @param usAddress discrete address
 * @param usNDiscrete discrete number
 *
 * @return result
 */
eMBErrorCode
eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          iRegIndex, iNReg;

    iNReg =  usNDiscrete / 8 + 1;

    /* it already plus one in modbus function method. */
    usAddress--;

    if( ( usAddress >= REG_DISCRETE_START )
            && ( usAddress + usNDiscrete <= REG_DISCRETE_START + REG_DISCRETE_NREGS ) )
    {

        iRegIndex = (USHORT) (usAddress - REG_DISCRETE_START);

        while (iNReg > 0)
        {
            *pucRegBuffer++ = Byte_To_Bit(&usSDiscInBuf[iRegIndex]);
            iNReg--;
            iRegIndex += 8;
        }
        pucRegBuffer--;
        /* last discrete */
        usNDiscrete = usNDiscrete % 8;
        /* filling zero to high bit */
        *pucRegBuffer = *pucRegBuffer << (8 - usNDiscrete);
        *pucRegBuffer = *pucRegBuffer >> (8 - usNDiscrete);

    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}


eMBErrorCode
eMBReadDeviceIdCB( UCHAR * pucFrameCur, UCHAR ReadDevId,UCHAR ObjectId, USHORT *usLen )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    UCHAR strlen;
    UCHAR str[20];
    switch(ReadDevId)
    {
    	/* request to get the basic device identification (stream access) */
		case 1:
			/* More Follows */
			*pucFrameCur++ = 0x00;
			*usLen += 1;
			/* Next Object Id */
			*pucFrameCur++ = 0x00;
			*usLen += 1;
			/* Number of objects */
			*pucFrameCur++ = 0x03;
			*usLen += 1;

			/* 1st Object ID  && Object length && Object Value  VendorName */
			*pucFrameCur++ = 0x01;
			*usLen += 1;
			strlen = sizeof(VendorName);
			*pucFrameCur++ =strlen;
			*usLen += 1;
			memcpy(pucFrameCur,VendorName,strlen);
			*usLen += strlen;
			pucFrameCur += strlen;

			/* 2st Object ID  && Object length && Object Value  ProductCode */
			*pucFrameCur++ = 0x02;
			*usLen += 1;
			strlen = sizeof(ProductCode);
			strlen = sizeof(str);
			*pucFrameCur++ =strlen;
			*usLen += 1;
			memcpy(pucFrameCur,ProductCode,strlen);
			*usLen += strlen;
			pucFrameCur += strlen;


			/* 3st Object ID  && Object length && Object Value  MajorMinorRevision */
			*pucFrameCur++ = 0x03;
			*usLen += 1;
			strlen = sizeof(MajorMinorRevision);
			*pucFrameCur++ =strlen;
			*usLen += 1;
			memcpy(pucFrameCur,MajorMinorRevision,strlen);
			*usLen += strlen;
			pucFrameCur += strlen;

			/**/
			break;
		/* request to get the regular device identification (stream access) */
		case 2:
			//break;
		/* request to get the extended device identification (stream access) */
		case 3:
			//break;
		/* request to get one specific identification object (individual access) */
		case 4:
			//break;
		default:
			eStatus = MB_ENOREG;
			break;
    }

    return eStatus;
}
/**
  *****************************************************************************
  * @brief  数组 to 字节 转换
  * @note	将数组中连续的8个，转换到一个字节里。a【8】 = {a,b,c,d,e,f,g,h}
  *			转换后b = 0b abcd efgh,
  * @param 要转换数组的首地址
  * @retval 转换完成的字节
  *****************************************************************************
  */
uint8_t Byte_To_Bit(uint8_t *p)
{
    uint8_t i = 0;
    uint8_t bit = 0;
    for(i = 0; i < 8; i++)
    {
        bit = bit <<1;
        bit |= (*(p+7-i) & 0x01);
    }
    return(bit);
}

/**
  *****************************************************************************
  * @brief  字节 to 数组 转换
  * @note	将一个字节里的8bit，输出到数组中连续的8个，
  *			转换前b = 0b hgfedcba 转换后a【8】 = {a,b,c,d,e,f,g,h}。
  * @param 要转换的字节n，输出数组的首地址p，
  * @retval None
  *****************************************************************************
  */
void Bit_To_Byte(uint8_t n,uint8_t *p)
{

    uint8_t i = 0;
    for(i = 0; i < 8; i++)
    {
        *(p+7-i) = (n & (0x01 << i)) >> i;
    }
}
