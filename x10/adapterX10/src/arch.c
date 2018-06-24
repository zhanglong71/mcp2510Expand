
#include "stm32f10x_lib.h"
#include "../inc/CONST.h"
#include "../inc/ctype.h"
#include "../inc/global.h"	
#include "../inc/debug.h"
#include "src/charQue.h"
#include "src/canTxQue.h"
#include "ascii_tab.h"
#include "../main.h"
#include "arch.h"
#include "crc8.h" 
#include "X10Que.h"

/*******************************************************************************
 *
 * function stack operation
 *
 ********************************************************************************/
void init_fstack(fstack_t *s)
{
	s->top = 0;
}

int	stackpop(fstack_t *s)
{
	if((s->top <= 0) || (s->top >= STACKSIZE))    /** make sure ... top [1..STACKSIZE - 1]  **/
	{
		return	FALSE;
	}
	
	s->top--;
	return	TRUE;
}

int	stackpush(fstack_t *s, func_t *func)   /** make sure ... top [0..STACKSIZE-2]  **/
{
	if((s->top < 0) || (s->top >= STACKSIZE - 1))
	{
		return	FALSE;
	}
	s->top++;
	s->func[s->top].func = func->func;
	s->func[s->top].arg = func->arg;
	
	return	TRUE;
}
/**
 * get the data at the top of the stack
 *
 **/
int	stacktop(const fstack_t *s, func_t *f)   /** make sure ... top [1..STACKSIZE - 1]  **/
{
	if((s->top <= 0) || (s->top >= STACKSIZE))
	{
		return	FALSE;
	}
	f->func = s->func[s->top].func;
	f->arg = s->func[s->top].arg;

	return	TRUE;
}
/*******************************************************************************
 *
 * msg queue operation
 *
 *******************************************************************************/
void init_queue(msgq_t *q)
{
	q->tail = q->head = 0;
}

static int isempty(msgq_t *q)
{
	if(q->tail == q->head)
	{
		return TRUE;
	}
	return	FALSE;
}

static int isfull(msgq_t *q)
{
	if((q->tail + 1 == q->head) || (((q->tail + 1) % QUEUESIZE) == (q->head)))
	{
		return TRUE;
	}
	return	FALSE;
}

int	inq(msgq_t *q, msgType_t val)		    //Note: check queue full is necessary before invoke this routine
{
    if(isfull(q))
    {
        return  FALSE;
    }
    
	q->buf[q->tail] = val;
	q->tail = (q->tail + 1) % QUEUESIZE;

	return TRUE;
}

int	inq_irq(msgq_t *q, msgType_t val)		//Note: check queue full is necessary before invoke this routine
{
    int iRet;
    
    IRQ_disable();
    iRet = inq(q, val);
    IRQ_enable();

	return iRet;
}

int	outq(msgq_t *q, msgType_t *val)		//Note: check queue empty is necessary before invoke this routine
{
    if(isempty(q))
    {
        return  FALSE;
    }
    
	*val = q->buf[q->head];
	q->head = (q->head + 1) % QUEUESIZE;
	    
	return	TRUE;
}
int	outq_irq(msgq_t *q, msgType_t *val)		//Note: check queue empty is necessary before invoke this routine
{
    int iRet;
    
    IRQ_disable();
    iRet = outq(q, val);
	IRQ_enable();
	    
	return	iRet;
}

/*******************************************************************************
 * Timer operation
 *******************************************************************************/
void SetTimer(Timer_t * timer, int tick)
{
//    timer->count = 0;
    timer->tick = tick;
    timer->tick_bak = tick;
}

void SetTimer_irq(Timer_t * timer, int tick)
{
    IRQ_disable();
    
    SetTimer(timer, tick);
    
    IRQ_enable();
}

void ClrTimer(Timer_t * timer)
{
    timer->tick = 0;
    timer->tick_bak = 0;
//    timer->count = 0;
}
void ClrTimer_irq(Timer_t * timer)
{
    IRQ_disable();
    
    ClrTimer(timer);
    
    IRQ_enable();
}
/*******************************************************************************
 * delay() operation
 * 
 * wait until timer out
 *******************************************************************************/
int g_tmr_delay;

void delay(int TickCount)
{
    g_tmr_delay = TickCount;
    
    while(g_tmr_delay);
}

void setDelayTimer(u8 TimerNum, int TickCount)
{
    IRQ_disable();
    if(TimerNum == 0)
    {
        g_tmr_delay = TickCount;
    }
    IRQ_enable();
}

int getDelayTimer(u8 TimerNum)
{
    if(TimerNum == 0) 
    {
        return g_tmr_delay;
    }
    else 
    {
        return	0;
    }
}

#if	1
/*******************************************************************************
 * prototype:   int send2can(canFrame_queue_t *dest, X10RevData_t *src)
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
   
int send2can(canFrame_queue_t *dest, const X10RevData_t *src)
{
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

    TxMessage.Data[0] = 0x80 | (g_channel & 0x7f);           /** destination **/
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

    return	0;
}
#endif

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

#if 0
/*******************************************************************************
 *
 * comTxStart/comTxStop
 * input: none
 * output: none
 * return: none
 * descript: 
 *
 *******************************************************************************/
void comTxStart(void)
{
    GPIO_ResetBits(GPIOC, GPIO_Pin_1);
}
void comTxStop(void)
{
    GPIO_SetBits(GPIOC, GPIO_Pin_1);
}

/*******************************************************************************
 *
 * comRxCheck
 * input: none
 * output: none
 * return: status of pin/port
 * descript: 
 *
 *******************************************************************************/
int comRxCheck(void)
{
    return GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0);
}
#endif

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
    int iRet = FALSE;
    
    switch(*val)
	{
	case CMSG_COMRX:        /** 收到一组完整的USART数据, 整理成多个canbus帧，并存入队列 **/
		
		break;

	case CMSG_CANRX:        /** 收到一组完整的canbus数据, 整理成带控制字符的数据，并存入队列 **/
	    //send2com(&g_com1TxQue, &g_canRevBuf);     /** output to USART1 for debug **/
	    //send2com(&g_com2TxQue, &g_canRevBuf); 
	    send2x10(&g_x10TxQue, &g_canRevBuf);
	    iRet = TRUE;
		break;
	case CMSG_QUECH:        /** 查询信道channel **/
	    //sendChannel(&g_comTxQue, g_channel);
	    sendChannel(&g_com2TxQue, g_channel);
	    iRet = TRUE;
		break;
	case CMSG_COMTIMEOUT:   /** 接收串口数据超时 **/
	    
	    break;
	case CMSG_X10RX:
		send2can(&g_canTxQue, &g_x10Revframe);
		iRet = TRUE;
		
		
		break;
	default:
		/** Can't process the msg here **/
		
		break;
	}
	
	return  iRet;
}

#if 0
/*******************************************************************************
 * DAEMON_USART2_Send()
 *******************************************************************************/
void DAEMON_USART2_Send(void)
{
    charData_t   comData;
    
    if(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == SET)
    {
        if(charQueueOut_irq(&g_comTxQue, &comData) == TRUE)
        {
            USART_SendData(USART2, comData.ucVal);
            //USART_SendData(USART1, comData.ucVal);  /** for debug only **/
        }
    }
}
#endif

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
/////////////////////////////////////////////////////
