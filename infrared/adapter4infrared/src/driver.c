
#include "stm32f10x_lib.h"
#include "string.h"
#include "../inc/CONST.h"
#include "../inc/ctype.h"
#include "../inc/global.h"
#include "../inc/macro.h"	
#include "../inc/debug.h"

#include "driver.h"
#include "src/charQue.h"
#include "src/canTxQue.h"
#include "ascii_tab.h"
#include "../main.h"
#include "arch.h"
#include "crc8.h" 

/*******************************************************************************
 * prototype:   int send2can(canFrame_queue_t *dest, charBuf_queue_t *src)
 *
 * descriptor: 将收到的X10数据转换成等待发送的canbus数据
 *
 * input: src
 * output: dest
 * return: none
 *
 * Note: the data come from USART2, the format: len + [data ..] + 0xff
 *******************************************************************************/
u8 g_ucCanseq = 0;
   
int send2can(canFrame_queue_t *dest, const charBuf_queue_t *src)
{
#if	0
    int i;
    CanTxMsg TxMessage;
	unsigned char tmpbuf[16];
	unsigned char ucAddr = 0;
	unsigned char ucFunc = 0;
	unsigned int tmpVal;

    /* transmit */
    TxMessage.StdId = 0x0;            /** destination **/
    TxMessage.ExtId = (0x80 | (g_channel & 0x7f)) | (TxMessage.StdId << 18);       /** source **/
    TxMessage.RTR = CAN_RTR_DATA;
    //TxMessage.IDE = CAN_ID_STD;
    TxMessage.IDE = CAN_ID_EXT;
    TxMessage.DLC = 8;

    TxMessage.Data[0] = 0x80 | (g_channel & 0x7f);           /** destination ***/
    TxMessage.Data[1] = 16;      /** get length first **/
 
    if((g_ucCanseq + 4) >= 255){g_ucCanseq = 0;} else {g_ucCanseq += 2;}
    
    /** clear the buff **/
	for(i = 0; i < 16; i++)tmpbuf[i] = 0;

	/** address **/
	if((src->flags & 0x01) == 1)	/** the address is valid **/
	{	
		tmpVal = src->addrCode;
		for(i = 0; i < 8; i++)
		{
			ucAddr <<= 1;
			ucAddr |= ((tmpVal & 0x30000) == 0x20000);
			tmpVal <<= 2;
		}
		/** keyCode **/
		tmpbuf[0] = (ucAddr & 0x0f);
		tmpbuf[0] = (key2num(tmpbuf[0]) + 1);
		/** houseCode **/
		tmpbuf[1] = ((ucAddr >> 4) & 0x0f);
		tmpbuf[1] = house2num(tmpbuf[1]);
	}
	else
	{
		//ucAddr = g_destAddr;
		tmpbuf[0] = (g_destAddr & 0x0f);
		tmpbuf[1] = ((g_destAddr >> 4) & 0x0f);
	}
		
	/** function **/
	tmpVal = src->funcCode;
	for(i = 0; i < 5; i++)
	{
		ucFunc <<= 1;
		ucFunc |= ((tmpVal & 0x300) == 0x200);
		tmpVal <<= 2;
	}

    /** tmpbuf[10]: cmd, it depends on the funcionCode **/
    if((ucFunc == 0x1b) || (ucFunc == 0x1d))	/** 收到状态回复 **/
    {
    	tmpbuf[10] = 0x43;
    	/** 由于是响应回复, 所以数据帧中的地址应该是源设备地址 **/
    	tmpbuf[4] = tmpbuf[0];
    	tmpbuf[5] = tmpbuf[1];
    	
    	tmpbuf[0] = 0;
    	tmpbuf[1] = 0;
    	
    	tmpbuf[6] = CDEV_TYPE;
    }
    else if(ucFunc == 0x1f)				/** 收到状态查询(可能以后用得上) **/
    {
    	tmpbuf[10] = 0x41;
    }
    else								/** 收到状态设定(可能以后用得上) **/
    {
    	tmpbuf[10] = 0x42;
    }
    
    tmpbuf[11] = 0x01;			/** protocal **/
    tmpbuf[12] = (ucFunc & 0x1f);
    tmpbuf[15] = makeCrc8(0, tmpbuf, 15);
    
    for(i = 0; i <= 15; i++)
    {
        TxMessage.Data[4 + (i & 0x3)] = tmpbuf[i];

        if((((i + 1) & 0x03) == 0) /**  || ((i + 1) == length) **/ )   /** 4, 8, 12, ... , 4x, ...  or the last one **/
        {
        	TxMessage.DLC = 5 + (i & 0x03);
        	
            TxMessage.Data[2] = g_ucCanseq++;
            TxMessage.Data[3] = (i >> 2);
            CanTXqueueIn(dest, &TxMessage);
        }
    }
    /****/
#endif

    return	0;
}


#if	0
/*******************************************************************************
 * name:    send2com
 * descriptor: 将收到的canbus数据转换成等待发送的com串口数据
 *
 * input: src
 * output: dest
 * return: none
 *
 *******************************************************************************/
int send2com(charBuf_queue_t *dest, charBuf_queue_t *src)
{
	unsigned char i;
    charData_t charData;
    charBuf_queue_t comTmpBuf;

    /** move data to buf which start address is comTmpBuf.buf **/
    charQueueInit(&comTmpBuf);
    for(i = 0; i < src->len; i++)
    {
        charData.ucVal = src->buf[i];
		charQueueIn(&comTmpBuf, &charData);
    }
    
    /** the secondary **/
    data2ascii(comTmpBuf.buf, src->len);
    comTmpBuf.len = (src->len << 1) + 2;
    
    /** the secondary data store into queue **/
    for(i = 0; i < comTmpBuf.len; i++)
    {
        charData.ucVal = comTmpBuf.buf[i];
		charQueueIn(dest, &charData);
    }

	return	0;
}
#endif


/*******************************************************************************
 * name:    send2x10
 * descriptor: 将收到的canbus数据转换成等待发送的x10接口数据帧
 *
 * input: src
 * output: dest
 * return: none
 *
 *******************************************************************************/
#if	0
u8 g_destAddr;
int send2x10(X10Buf_queue_t *dest, const charBuf_queue_t *src)
{
//	static unsigned char tmp;
	X10Data_t x10data;
/** b0   b1   b2   b3   b4   b5   b6   b7   b8    b9    b10  b11  b12  b13 b14  b15 **/
/** 0x11+0x22+0x33+0x44+0x55+0x66+0x77+0x88+Resv1+Resv2+0x44+0x01+Vl + Vh +Resv+CRC **/	
	if(src->buf[10] == 0x41)
	{
		g_destAddr = (((src->buf[1] << 4) & 0xF0) | (src->buf[0] & 0x0f));	/** 只对查询动作保留其地址 **/
		
		x10data.keyCode = num2key(src->buf[0]);
		x10data.houseCode = num2house(src->buf[1]);
		X10QueueIn_irq(dest, &x10data);
		
		x10data.houseCode = num2house(src->buf[1]);
		x10data.keyCode = 0x1f;		//src->buf[12]
		X10QueueIn_irq(dest, &x10data);
		
		if(src->buf[12] != 0x1f)
		{
			/** warning!!! **/
		}
		
		return	0;
	}
	else if(src->buf[10] == 0x42)
	{
		if(((src->buf[12] & 0x1f) != 1) &&
			((src->buf[12] & 0x1f) != 3) && 
			((src->buf[12] & 0x1f) != 13)) /** if it't the all on/off commmand, sending address is no need **/
		{
			x10data.keyCode = num2key(src->buf[0]);
			x10data.houseCode = num2house(src->buf[1]);
			X10QueueIn_irq(dest, &x10data);
		}
	
		x10data.houseCode = num2house(src->buf[1]);
		x10data.keyCode = (src->buf[12] & 0x1f);
		X10QueueIn_irq(dest, &x10data);
	}
	else if(src->buf[10] == 0x43)
	{
		#if	0	/** 不存在此场景 **/
		x10data.keyCode = num2key(src->buf[0]);
		x10data.houseCode = num2house(src->buf[1]);
		X10QueueIn_irq(dest, &x10data);
		
		x10data.houseCode = num2house(src->buf[1]);
		x10data.keyCode = (src->buf[12] & 0x1f);
		X10QueueIn_irq(dest, &x10data);
		#endif
	}
	else if((src->buf[10] == 0x11))	/** 收到自举广播 **/
	{
		if((src->buf[0] == 0xff) && ((src->buf[1] == 0xff)) && ((src->buf[2] == 0xff)) && ((src->buf[3] == 0xff)))
		{
    		inq_irq(&g_msgq, CMSG_RGST);
		}
	}
	else
	{
	}
	
	return 1;
}
#endif


/*******************************************************************************
 * name:    send2infrared
 * descriptor: 将收到的canbus数据转换成等待发送的wireless接口数据
 *
 * input: src
 * output: dest
 * return: none
 *
 *******************************************************************************/
int send2infrared(charBuf_queue_t *dest, charBuf_queue_t *src)
{
	unsigned char i;
    charData_t charData;
//    charBuf_queue_t comTmpBuf;
/** b0   b1   b2   b3   b4   b5   b6   b7   b8    b9    b10  b11  b12  b13 b14  b15 **/
/** 0x11+0x22+0x33+0x44+0x55+0x66+0x77+0x88+Resv1+Resv2+0x41+0x01+Vl + Vh +Resv+CRC **/
/** 对遥控器而言，上面的0x11是遥控器号，V1是当前遥控器的按键号 **/
    /** check CRC8 **/
    if((src->buf[src->len - 1] == makeCrc8(0, src->buf, src->len - 1)) || 1) {  /** CRC **/
    	
    	/** response 0x42 only, store into queue **/
    	if(src->buf[10] == 0x42)
    	{
    		for(i = 0; i < 4; i++) {
    			g_Phase_sending.addr[i] = src->buf[i];
    			g_Phase_sending.cmd[i] = src->buf[i + 10];
    		}
    		
			readRemoteCodeTable(g_Phase_sending.addr[0], g_Phase_sending.cmd[2], g_Phase_sending.buf);
			memcpy((unsigned char *)g_Phase_sending.head, &(g_Phase_sending.buf[CHEADER_OF_ENTRY]), sizeof(g_Phase_sending.head));
		
    		dest->len = src->len;
    		for(i = 0; i < src->len; i++) {
    		    charData.ucVal = src->buf[i];
				charQueueIn_irq(dest, &charData);
    		}
    	} else {/** nothing **/}
	}
	
	return	0;
}

/*******************************************************************************
 *
 * 发送信道号给zigbee模块
 *
 *******************************************************************************/
int sendChannel(charBuf_queue_t *dest, unsigned char __channel)
{
	unsigned char i;
    charData_t charData;
    unsigned char buf[34];

	for(i = 0; i < 16; i++)
	{
		buf[i] = 0;
	}
    buf[10] = 0xa1;
    buf[11] = 0x01;
    buf[12] = __channel;
    buf[15] = makeCrc8(0, buf, 15);           /** CRC **/
    
    data2ascii(buf, 16);
    
    /** the secondary data store into queue **/
    for(i = 0; i < 34; i++)
    {
        charData.ucVal = buf[i];
		charQueueIn(dest, &charData);
    }

	return	0;
}

/*******************************************************************************
 *
 * sysProcess
 * input: data - msg
 * output: none
 * return: TRUE - process ok
 *         FALSE - Can't process
 * descript: 
 *
 *******************************************************************************/
int sysProcess(msgType_t *val)
{
 	charData_t   charData;
    int iRet = FALSE;	  
    int i = 0;
    
    switch(*val)
	{
	case CMSG_COMRX:        /** 收到一组完整的USART数据, 整理成多个canbus帧，并存入队列 **/
		break;

	case CMSG_CANRX:        /** 收到一组完整的canbus数据, 整理后存入队列 **/
	    send2infrared(&g_infraredTxQue, &g_canRevBuf);
	    
	    iRet = TRUE;
	    
    	#if	0
		for(i = 0; i < 4; i++) {
			charData.ucVal = g_Phase_sending.addr[i];  
			charQueueIn_irq(&g_com1TxQue, &charData);
		}
    	for(i = 0; i < 4; i++) {
			charData.ucVal = g_Phase_sending.cmd[i];  
			charQueueIn_irq(&g_com1TxQue, &charData);
		}
		#endif
		
		#if	1
		charData.ucVal = g_Phase_sending.addr[0];
		charQueueIn_irq(&g_com1TxQue, &charData);
		charData.ucVal = g_Phase_sending.cmd[2];
		charQueueIn_irq(&g_com1TxQue, &charData);
		
		charData.ucVal = 0xff & (g_Phase_sending.head[0] >> 8);
		charQueueIn_irq(&g_com1TxQue, &charData);
		charData.ucVal = 0xff & g_Phase_sending.head[0];
		charQueueIn_irq(&g_com1TxQue, &charData);
		charData.ucVal = 0xff & (g_Phase_sending.head[1] >> 8);
		charQueueIn_irq(&g_com1TxQue, &charData);
		charData.ucVal = 0xff & g_Phase_sending.head[1];
		charQueueIn_irq(&g_com1TxQue, &charData);
    	for(i = 0; i < 100; i++) {
			charData.ucVal = g_Phase_sending.buf[i];
			charQueueIn_irq(&g_com1TxQue, &charData);
		}
    	for(i = 120; i < CBYTES_OF_ENTRY; i++) {
			charData.ucVal = g_Phase_sending.buf[i];
			charQueueIn_irq(&g_com1TxQue, &charData);
		}
		#endif
		
		break;

	case CMSG_QUECH:        /** 查询信道channel **/
	    sendChannel(&g_com2TxQue, g_channel);
	    iRet = TRUE;
		break;
	case CMSG_COMTIMEOUT:   /** 接收串口数据超时 **/
	    break;
	    
	    #if	0
	case CMSG_WLSRX:
		send2can(&g_canTxQue, &g_wlsRevBuf);	
		iRet = TRUE;
		break;
		#endif
		
	case CINFR_RCV:			/** 收到完整的红外信号，保存之 **/
		memcpy(&(g_Phase_sending.buf[CHEADER_OF_ENTRY]), (unsigned char *)g_Phase_sending.head, 4);
		writeRemoteCodeTable(g_Phase_sending.addr[0], g_Phase_sending.cmd[2], g_Phase_sending.buf);
		iRet = TRUE;
		
       	MDEBUG_PA7_INVERSE();	//??????????????????????????????????????????????????????
		break;
	default:
		/** Can't process the msg here **/
		
		break;
	}
	
	return  iRet;
}

/*******************************************************************************
 * DAEMON_USARTx_Send()
 *******************************************************************************/
void DAEMON_USARTx_Send(USART_TypeDef* USARTx, charBuf_queue_t* comTxQue)
{
    charData_t   comData;
    
    if(USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == SET)
    {
        if(charQueueOut_irq(comTxQue, &comData) == TRUE)
        {
            USART_SendData(USARTx, comData.ucVal);
        }
    }
}
/*******************************************************************************
 * DAEMON_CAN_Send()
 * canbus报文发送策略:
 * 1、在禁止标志无效、并且有数据的情景下发报文
 * 2、发送一个报文后:
 *      a、通过CanDeviceSetDisableFlag()设置禁止标志，
 *      b、通过getDelayTimer()设置超时时间
 * 3、查看禁止标志:
 *      a、为0不理会(也就是随时可以发数据)
 *      b、为1, 若发送完成了、或超时了就通过CanDeviceClrDisableFlag()清除禁止标志
 *
 * Note: 考虑竞争情景，超时时间应该比发送一个canbus报文的时间长很多。当前在示波器
 *       上测得发送一个报文的时间约1.12ms。所以在清除禁示标记的情景中，正常情况下
 *       都在(CAN_TransmitStatus(TransmitMailbox) == CANTXOK)条件下通过。在有故障
 *       或掉线的情况下才会超时。
 *******************************************************************************/
void DAEMON_CAN_Send(void)
{
    static u8 TransmitMailbox;
    CanTxMsg TxMessage;
    
#if 1
    if(CanDeviceGetDisableFlag(&g_canTxQue) != 0)
    {
        if((CAN_TransmitStatus(TransmitMailbox) == CANTXOK) || (getDelayTimer(0) == 0))
        {
            CanDeviceClrDisableFlag(&g_canTxQue);
        }
    }

    if(CanDeviceGetDisableFlag(&g_canTxQue) == 0)
    {
        if(CanTXqueueOut(&g_canTxQue, &TxMessage) == TRUE)
        {
            CanDeviceSetDisableFlag(&g_canTxQue);

            TransmitMailbox = CAN_Transmit(&TxMessage);
            setDelayTimer(0, 10);
        }
    }
#endif
}

/*******************************************************************************
 * function: flashWrite
 * Description: write 1KB data into a specified page
 * input: 	arr - the data array pointer
 * output: no
 * return: average
 * 
 * Note: flash Page size FLASH_PAGE_SIZE=1KBytes
********************************************************************************/
//#define	CFLASHSTARTADDR (0x0801FC00)		/** the 127th Flash Page **/

#if	0
unsigned int pageAddrTab[17] = {
	(0x0801FC00 - (0x400 * 0)),	/** the 127th Flash Page **/
	(0x0801FC00 - (0x400 * 1)), /** the 126th Flash Page **/
	(0x0801FC00 - (0x400 * 2)),
	(0x0801FC00 - (0x400 * 3)),
	(0x0801FC00 - (0x400 * 4)),
	(0x0801FC00 - (0x400 * 5)),
	(0x0801FC00 - (0x400 * 6)),
	(0x0801FC00 - (0x400 * 7)),
	(0x0801FC00 - (0x400 * 8)),
	(0x0801FC00 - (0x400 * 9)),
	(0x0801FC00 - (0x400 * 10)),
	(0x0801FC00 - (0x400 * 11)),
	(0x0801FC00 - (0x400 * 12)),
	(0x0801FC00 - (0x400 * 13)),
	(0x0801FC00 - (0x400 * 14)),
	(0x0801FC00 - (0x400 * 15)),
	(0x0801FC00 - (0x400 * 16)),
	(0x0801FC00 - (0x400 * 17)),
}
#define MpageAddr(x)	(pageAddrTab[x])
#endif

int flashWrite(u32 arr[]/*input*/, int pageNO/*input*/)
{
	u32 FlashAddress;
	FLASH_Status FLASHStatus = FLASH_COMPLETE;
	
	/* Unlock the Flash Program Erase controller */
    FLASH_Unlock();   
    /* Clear All pending flags */
    FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);	//清标志位
    
    /* Erases the specified Flash Page */
    FlashAddress = Mpage2Addr(pageNO);
    FLASHStatus = FLASH_ErasePage(FlashAddress); 	//擦除这个扇区
	if(FLASHStatus != FLASH_COMPLETE)
	{
		return -1;
	}
    
    /** Writes the Data at the Address **/
    FlashAddress = Mpage2Addr(pageNO);
    while(FlashAddress < Mpage2Addr(pageNO) + CFLASH_PAGE_SIZE)
    {
    	FLASH_ProgramWord(FlashAddress, arr[(FlashAddress - Mpage2Addr(pageNO)) >> 2]);
    	
    	FlashAddress += 4;
	}
	
	FLASH_Lock();
	
	return	0;
}

void flashCache_init(void)
{
	int i;
	for(i = 0; i < CFLASH_PAGE_SIZE; i++)g_flash.arrChar[i] = CENTRYFLAG_IDLE;
}

/*******************************************************************************
 * Description: 将遥控器号r，遥控器按键编号b,转换成页号p,和页内偏移f, 并读出相应的数据
 *******************************************************************************/
int readRemoteCodeTable(int _remoteNO/* intput */, int _buttonNO/* intput */, unsigned char *_address/** output **/)
{
	int pageNO;
	int offset;
	
	/** 确定页号 **/
	if(_remoteNO == 0) {
		pageNO = (_buttonNO >> 3);
	} else {
		/** p = r*4-3+b/16 **/
		pageNO = (_remoteNO << 3) - 6 + (_buttonNO >> 3);
	}
	/** 确定页内偏移号f = (b % 8) * 128 **/
	offset = ((_buttonNO & 0x07) << 7);
	
	memcpy(g_flash.arrChar, (char *)Mpage2Addr(pageNO), CFLASH_PAGE_SIZE);
	
	memcpy(_address, (char *)Mpage2Addr(pageNO) + offset, CBYTES_OF_ENTRY);
	
	return	0;
}

/*******************************************************************************
 * 将收到的数据写入flash
 *******************************************************************************/
int writeRemoteCodeTable(int _remoteNO/* intput */, int _buttonNO/* intput */, unsigned char *_address/** input **/)
{
	int pageNO;
	int offset;
	
	/** 确定页号 **/
	if(_remoteNO == 0) {
		pageNO = (_buttonNO >> 3);
	} else {
		/** p = r*4-3+b/16 **/
		pageNO = (_remoteNO << 3) - 6 + (_buttonNO >> 3);
	}
	/** 确定页内偏移号f = (b % 8) * 128 **/
	offset = ((_buttonNO & 0x07) << 7);
	
	
	memcpy(g_flash.arrChar + offset,  _address, CBYTES_OF_ENTRY);
	flashWrite((u32 *)g_flash.arrChar, pageNO);

	return	0;
}

/*******************************************************************************
 * Description: 将遥控器号r，转换成页号p,和页内偏移f, 并读出相应的数据附加数据
 *******************************************************************************/
 #if	0
int readRemoteCodeAddons(int _remoteNO/* intput */, unsigned char *_address/** output **/)
{
	int pageNO;
	int offset;
	
	/** 确定页号 p = 0, 4, 8, 12, 16  **/
	pageNO = (_remoteNO << 2);
	/** 确定页内偏移号f = 768, 768, 768, 768 768......the last 256 byte **/
	offset = 768;
	
	if(memcmp((char *)(Mpage2Addr(pageNO) + MAGIC_SIZE_OFFSET), g_magic, MAGIC_SIZE) != 0)	//没有写入过数据
	{
		memset(g_flash.arrChar, 0, CFLASH_PAGE_SIZE);
		memcpy((g_flash.arrChar + MAGIC_SIZE_OFFSET), g_magic, MAGIC_SIZE);
	} else {
		memcpy(g_flash.arrChar, (char *)Mpage2Addr(pageNO), CFLASH_PAGE_SIZE);
	}
	
	memcpy(_address, (char *)Mpage2Addr(pageNO) + offset, 4);
	
	return	0;
}

/*******************************************************************************
 * 将附加数据数据写入flash
 *   将_address指向的数据写入到_remoteNO遥控器号指向的存储地址
 *******************************************************************************/
int writeRemoteCodeAddons(int _remoteNO/* intput */, unsigned char *_address/** input **/)
{
	int pageNO;
	int offset;
	
	/** 确定页号 p = 0, 4, 8, 12, 16  **/
	pageNO = (_remoteNO << 2);
	/** 确定页内偏移号f = 768, 768, 768, 768 768......the last 256 byte **/
	offset = 768;
	
	//memcpy((char *)(Mpage2Addr(pageNO) + offset, _address, 64);
	memcpy(g_flash.arrChar + offset, _address, 4);
	flashWrite((u32 *)g_flash.arrChar, pageNO);

	return	0;
}
#endif

/////////////////////////////////////////////////////

