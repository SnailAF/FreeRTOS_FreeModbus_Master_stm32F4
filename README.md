​
FreeModbus是开源的modbus库，可以很方便的移植到单片机上，但是遗憾的是FreeModbus没有主机库，好消息是armink大神写了仿照freemodbus从机写了一套主机，
[armink](https://github.com/armink/FreeModbus_Slave-Master-RTT-STM32)在FreeModbus的基础上写了自己的主机程序，用起来还是有些不方便，主要在于，从机地址必须连续，寄存器地址必须连续，读会的数据要先放到一个二维数组中。
在此基础上，修改为不在限制从机地址和从机寄存器，同时将读取到的寄存器数据直接放置到用户的缓存中。
这个套主机不是特别好用，主要问题是将从机寄存器在本地做了缓存，带来的问题就是，从机地址必须连续，所有从机的寄存器地址必须连续，否则就会带来大量的内存浪费。

核心思想是，读取函数的参数中增加一个指针，将接收的数据返回到指针指向的数组中。

要修改三个地方:

一、接口函数

二、新增指针传递函数

三、修改数据接收处理函数。

一、修改接口函数
以读保持寄存器为例，mbfuncholding_m.c中具体代码修改如下：

原函数
'''
eMBMasterReqErrCode
eMBMasterReqReadHoldingRegister( UCHAR ucSndAddr, USHORT usRegAddr, USHORT usNRegs, LONG lTimeOut )
'''
修改后的函数，USHORT * pusDataBuf；这样读取的到的数据由预先定义的数组，改为用户自定数组。
'''
eMBMasterReqErrCode
eMBMasterReqReadHolding( UCHAR ucSndAddr, USHORT usRegAddr, USHORT usNRegs, USHORT * pusDataBuf,LONG lTimeOut )
'''

主要修改3个地方，

1. 增加数据返回指针，

2. 不在对从机地址进行判断

3. 将用户传递的指针想办传递出去。这里我用了一组函数实现。

修改前后如下：
'''
/* 原函数 */
/**
 * This function will request read holding register.
 *
 * @param ucSndAddr salve address
 * @param usRegAddr register start address
 * @param usNRegs register total number
 * @param lTimeOut timeout (-1 will waiting forever)
 *
 * @return error code
 */
eMBMasterReqErrCode
eMBMasterReqReadHoldingRegister( UCHAR ucSndAddr, USHORT usRegAddr, USHORT usNRegs, LONG lTimeOut )
{
    UCHAR                 *ucMBFrame;
    eMBMasterReqErrCode    eErrStatus = MB_MRE_NO_ERR;

    if ( ucSndAddr > MB_MASTER_TOTAL_SLAVE_NUM ) eErrStatus = MB_MRE_ILL_ARG;
    else if ( xMBMasterRunResTake( lTimeOut ) == FALSE ) eErrStatus = MB_MRE_MASTER_BUSY;
    else
    {
		vMBMasterGetPDUSndBuf(&ucMBFrame);
		vMBMasterSetDestAddress(ucSndAddr);
		ucMBFrame[MB_PDU_FUNC_OFF]                = MB_FUNC_READ_HOLDING_REGISTER;
		ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF]       = usRegAddr >> 8;
		ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF + 1]   = usRegAddr;
		ucMBFrame[MB_PDU_REQ_READ_REGCNT_OFF]     = usNRegs >> 8;
		ucMBFrame[MB_PDU_REQ_READ_REGCNT_OFF + 1] = usNRegs;
		vMBMasterSetPDUSndLength( MB_PDU_SIZE_MIN + MB_PDU_REQ_READ_SIZE );
		( void ) xMBMasterPortEventPost( EV_MASTER_FRAME_SENT );
		eErrStatus = eMBMasterWaitRequestFinish( );
    }
    return eErrStatus;
}
/* 修改后函数 */
/**
 * 将读到的数据放到指定的地址中
 *
 * @param ucSndAddr salve address
 * @param pusDataBuf 读到数据缓存地址。
 * @param usRegAddr register start address
 * @param usNRegs register total number
 * @param lTimeOut timeout (-1 will waiting forever)
 * 
 * @return error code
 */
/* 1# 参数增加*pusDataBuf 接收数据存储的指针 */
eMBMasterReqErrCode
eMBMasterReqReadHoldingRegister( UCHAR ucSndAddr, USHORT usRegAddr, USHORT usNRegs, USHORT * pusDataBuf,LONG lTimeOut )
{
    UCHAR                 *ucMBFrame;
    eMBMasterReqErrCode    eErrStatus = MB_MRE_NO_ERR;

    /* 2# 去掉地址判断 */
	if ( xMBMasterRunResTake( lTimeOut ) == FALSE ) eErrStatus = MB_MRE_MASTER_BUSY;
    else
    {
		vMBMasterGetPDUSndBuf(&ucMBFrame);
		vMBMasterSetDestAddress(ucSndAddr);
		ucMBFrame[MB_PDU_FUNC_OFF]                = MB_FUNC_READ_HOLDING_REGISTER;
		ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF]       = usRegAddr >> 8;
		ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF + 1]   = usRegAddr;
		ucMBFrame[MB_PDU_REQ_READ_REGCNT_OFF]     = usNRegs >> 8;
		ucMBFrame[MB_PDU_REQ_READ_REGCNT_OFF + 1] = usNRegs;
		vMBMasterSetPDUSndLength( MB_PDU_SIZE_MIN + MB_PDU_REQ_READ_SIZE );
        /* 3# 将接收指针传递出去 */
        eMBMasterSetDestDataBufferPoint(pusDataBuf);
		( void ) xMBMasterPortEventPost( EV_MASTER_FRAME_SENT );
		eErrStatus = eMBMasterWaitRequestFinish( );
    }
    return eErrStatus;
}

'''
二、在mbrtu.c中增加指针传递函数
'''
/*
 * 接受到数据要放入的地址指针，如果没有用到一定要置NULL因为在eMBMasterRegHoldingCB
 * 中会根据这个值判断将数据存放到usMRegHoldBuf还是用户定义地址
*/
USHORT * DestDataBuffer = NULL;
/*
 * 将接收数据地址暂存起来，等待eMBMasterRegHoldingCB中被调用。
*/
void eMBMasterSetDestDataBufferPoint(USHORT * data)
{
    DestDataBuffer = data;
}
/*
 * eMBMasterRegHoldingCB中被调用。将接收的数据放到指针中。
*/
void eMBMasterGetDestDataBufferPoint(USHORT ** data)
{
    *data = DestDataBuffer;
}

'''
三、修改接收回调函数
'''
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

'''
到此基本完成了读从机保持寄存器函数的修改，对于其他函数修改只需要一和三，因为modbus是总线，需要等待一个函数功能实现完成之后才能调用另一函数，所以传递指针使用同一组函数就可以。
