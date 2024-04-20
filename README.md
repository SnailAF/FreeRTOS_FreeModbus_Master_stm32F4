freeModbus 只有从机开源没有主机开源
armink在FreeModbus的基础上写了自己的主机程序，用起来还是有些不方便，主要在于，从机地址必须连续，寄存器地址必须连续，读会的数据要先放到一个二维数组中。
https://github.com/armink/FreeModbus_Slave-Master-RTT-STM32
在此基础上，修改为不在限制从机地址和从机寄存器，同时将读取到的寄存器数据直接放置到用户的缓存中。
'''
/*
 * FreeModbus 测试程序，将读到的数据写出去。
 * */
void ModbusAppTask( void *argument ){
	uint16_t data[10];
	osDelay(100);
	while(1){
		eMBMasterReqReadHolding(1, 40000, 5, data, 100);
		eMBMasterReqWriteMultipleHolding(1, 40005, 5, data, 100);
		osDelay(100);
	}
}
'''
