
# FreeModbus主机移植优化与数据直接缓存技巧

[FreeModbus](https://github.com/armink/FreeModbus_Slave-Master-RTT-STM32) 是一款开源的 Modbus 库，便于移植到单片机上。可惜官方没有提供主机库，值得庆幸的是，armink 大神基于 FreeModbus 仿写了一套主机程序。不过，这个主机仍有一些局限：**要求从机地址、寄存器地址连续，读取的数据需先存进二维数组**，实际使用并不便利。

**本文改进特点**：

- 不再强制从机地址/寄存器连续
- 读取到的寄存器数据，可直接写入用户自定义缓存
- 节省内存，适用不连续地址场景

核心思路：  
> 让读取类函数的参数中增加一个指针，将接收的数据直接返回到指针所指向的用户自定义数组中。

---

## 总体修改思路

修改涉及三处：

1. **接口函数**：参数中增加数据返回指针
2. **新增指针传递函数**：缓存用户数据指针
3. **数据接收处理函数**：将数据存入用户缓存

---

### 1. 修改接口函数

以“读保持寄存器”为例（`mbfuncholding_m.c`）：

#### 原函数

```c
eMBMasterReqErrCode
eMBMasterReqReadHoldingRegister(UCHAR ucSndAddr, USHORT usRegAddr, USHORT usNRegs, LONG lTimeOut)
```

#### 修改后函数

增加 `USHORT *pusDataBuf` 参数，读取数据直接存于用户自定义的数组中。

```c
eMBMasterReqErrCode
eMBMasterReqReadHoldingRegister(UCHAR ucSndAddr, USHORT usRegAddr, USHORT usNRegs, USHORT *pusDataBuf, LONG lTimeOut)
```

#### 主要变动

1. 增加数据返回指针参数
2. 移除对从机地址的判断
3. 增加函数将用户数据指针缓存下来

#### 代码对比

```c
/* 原始实现 */
eMBMasterReqErrCode
eMBMasterReqReadHoldingRegister(UCHAR ucSndAddr, USHORT usRegAddr, USHORT usNRegs, LONG lTimeOut)
{
    UCHAR *ucMBFrame;
    eMBMasterReqErrCode eErrStatus = MB_MRE_NO_ERR;

    if (ucSndAddr > MB_MASTER_TOTAL_SLAVE_NUM)
        eErrStatus = MB_MRE_ILL_ARG;
    else if (!xMBMasterRunResTake(lTimeOut))
        eErrStatus = MB_MRE_MASTER_BUSY;
    else
    {
        vMBMasterGetPDUSndBuf(&ucMBFrame);
        vMBMasterSetDestAddress(ucSndAddr);
        ucMBFrame[MB_PDU_FUNC_OFF] = MB_FUNC_READ_HOLDING_REGISTER;
        ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF] = usRegAddr >> 8;
        ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF + 1] = usRegAddr;
        ucMBFrame[MB_PDU_REQ_READ_REGCNT_OFF] = usNRegs >> 8;
        ucMBFrame[MB_PDU_REQ_READ_REGCNT_OFF + 1] = usNRegs;
        vMBMasterSetPDUSndLength(MB_PDU_SIZE_MIN + MB_PDU_REQ_READ_SIZE);
        (void) xMBMasterPortEventPost(EV_MASTER_FRAME_SENT);
        eErrStatus = eMBMasterWaitRequestFinish();
    }
    return eErrStatus;
}
```

```c
/* 改进实现：数据写入用户地址 */
eMBMasterReqErrCode
eMBMasterReqReadHoldingRegister(UCHAR ucSndAddr, USHORT usRegAddr, USHORT usNRegs, USHORT *pusDataBuf, LONG lTimeOut)
{
    UCHAR *ucMBFrame;
    eMBMasterReqErrCode eErrStatus = MB_MRE_NO_ERR;

    if (!xMBMasterRunResTake(lTimeOut))
        eErrStatus = MB_MRE_MASTER_BUSY;
    else
    {
        vMBMasterGetPDUSndBuf(&ucMBFrame);
        vMBMasterSetDestAddress(ucSndAddr);
        ucMBFrame[MB_PDU_FUNC_OFF] = MB_FUNC_READ_HOLDING_REGISTER;
        ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF] = usRegAddr >> 8;
        ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF + 1] = usRegAddr;
        ucMBFrame[MB_PDU_REQ_READ_REGCNT_OFF] = usNRegs >> 8;
        ucMBFrame[MB_PDU_REQ_READ_REGCNT_OFF + 1] = usNRegs;
        vMBMasterSetPDUSndLength(MB_PDU_SIZE_MIN + MB_PDU_REQ_READ_SIZE);
        // 关键：设置数据指针
        eMBMasterSetDestDataBufferPoint(pusDataBuf);
        (void) xMBMasterPortEventPost(EV_MASTER_FRAME_SENT);
        eErrStatus = eMBMasterWaitRequestFinish();
    }
    return eErrStatus;
}
```

---

### 2. 新增指针传递函数（于 mbrtu.c）

用于在“回调”中取到用户的数组指针：

```c
// 指针全局变量，未用务必置 NULL
USHORT *DestDataBuffer = NULL;

// 申请通信时设置
void eMBMasterSetDestDataBufferPoint(USHORT *data) {
    DestDataBuffer = data;
}

// 回调中取出
void eMBMasterGetDestDataBufferPoint(USHORT **data) {
    *data = DestDataBuffer;
}
```

---

### 3. 修改接收回调函数

```c
/**
 * Modbus master holding register callback function.
 */
eMBErrorCode eMBMasterRegHoldingCB(UCHAR *pucRegBuffer, USHORT usAddress,
                                   USHORT usNRegs, eMBRegisterMode eMode)
{
    eMBErrorCode eStatus = MB_ENOERR;
    USHORT *pusSRegHoldBuf;

    if (eMode == MB_REG_WRITE)
        return eStatus;

    eMBMasterGetDestDataBufferPoint(&pusSRegHoldBuf);

    if (pusSRegHoldBuf != NULL) // 若传递了用户指针即写入用户缓冲
    {
        usAddress--; // modbus协议函数已+1
        while (usNRegs > 0)
        {
            *pusSRegHoldBuf = *pucRegBuffer++ << 8;
            *pusSRegHoldBuf |= *pucRegBuffer++;
            pusSRegHoldBuf++;
            usNRegs--;
        }
        eMBMasterSetDestDataBufferPoint((USHORT *)NULL); // 一次用完清空
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }

    return eStatus;
}
```

---

## 总结

这样就实现了**直接把主机收到的数据写入用户的缓存**，针对其他读取功能改造时也只需按“第一步和第三步”处理即可。

> **注意：** Modbus 总线在一次传输完成后，建议及时清理指针变量，确保下次不会误写串发！

