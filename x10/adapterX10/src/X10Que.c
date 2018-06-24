#include "stm32f10x_lib.h"
#include "string.h"
#include "../inc/CONST.h"
#include "../inc/ctype.h" 
#include "../inc/global.h"
#include "../main.h"
#include "crc8.h" 
#include "src/canTxQue.h"
#include "X10Que.h"

/** house code **/
/** 序号 ==> 编码 **/
const unsigned char houseCode_tab[][2] = 
{
	{'A', 0x6},	/** 0b0110, **/
	{'B', 0xE},	/** 0b1110, **/
	{'C', 0x2},	/** 0b0010, **/
	{'D', 0xA},	/** 0b1010, **/
	
	{'E', 0x1},	/** 0b0001, **/
	{'F', 0x9},	/** 0b1001, **/
	{'G', 0x5},	/** 0b0101, **/
	{'H', 0xD},	/** 0b1101, **/
	
	{'I', 0x7},	/** 0b0111, **/
	{'J', 0xF},	/** 0b1111, **/
	{'K', 0x3},	/** 0b0011, **/
	{'L', 0xB},	/** 0b1011, **/
	
	{'M', 0x0},	/** 0b0000, **/
	{'N', 0x8},	/** 0b1000, **/
	{'O', 0x4},	/** 0b0100, **/
	{'P', 0xC},	/** 0b1100, **/
};

/** 编码 ==> 序号 **/
const unsigned char reverseHouseCode_tab[][2] = 
{
	{'M', 12},
	{'E', 4},
	{'C', 2},
	{'K', 10},
	
	{'O', 14},
	{'G', 6},
	{'A', 0},
	{'I', 8},
	
	{'N', 13},
	{'F', 5},
	{'D', 3},
	{'L', 11},
	
	{'P', 15},
	{'H', 7},
	{'B', 1},
	{'J', 9},
};

/** code code **/
/** 序号 ==> 编码.  如：数字x其编码为keyCode[x - 1][1] **/
const unsigned char keyCode_tab[][2] = {
	/** (p-NO., Encode) **/
	
	{1, 0x0C /** 0b01100 **/},
	{2, 0x1C /** 0b11100 **/},
	{3, 0x04 /** 0b00100 **/},
	{4, 0x14 /** 0b10100 **/},
	  
	{5, 0x02 /** 0b00010 **/},
	{6, 0x12 /** 0b10010 **/},
	{7, 0x0A /** 0b01010 **/},
	{8, 0x1A /** 0b11010 **/},
	  
	{9, 0x0E /** 0b01110 **/},
	{10, 0x1E /** 0b11110 **/},
	{11, 0x06 /** 0b00110 **/},
	{12, 0x16 /** 0b10110 **/},
	         
	{13, 0x00 /** 0b00000 **/},
	{14, 0x10 /** 0b10000 **/},
	{15, 0x08 /** 0b01000 **/},
	{16, 0x18 /** 0b11000 **/},
};

/** 编码 ==> 序号, 如：编码为x的数据其编号为reverseKeyCode[x][1] **/
const unsigned char reverseKeyCode_tab[][2] = {
	{13, 12},
	{5, 4},
	{3, 2},
	{11, 10},
	
	{15, 14},
	{7, 6},
	{1, 0},
	{9, 8},
	
	{14, 13},
	{6, 5},
	{4, 3},
	{12, 11},
	
	{16, 15},
	{8, 7},
	{2, 1},
	{10, 9},
};

/** function code **/
/** 序号 ==> 编码  y = 2*x + 1  **/
const unsigned char funcCode_tab[][2] = {
	{1, 0x01 /** 0b00001 **/},	/** all unit off **/
	{2, 0x03 /** 0b00011 **/},	/** all light on **/
	{3, 0x05 /** 0b00101 **/},	/** on **/
	{4, 0x07 /** 0b00111 **/},	/** off **/
	
	{5, 0x09 /** 0b01001 **/},	/** dim **/
	{6, 0x0B /** 0b01011 **/},	/** bright **/
	{7, 0x0D /** 0b01101 **/},	/** all light off **/
	{8, 0x0F /** 0b01111 **/},	/** extern code **/
	
	{9, 0x11 /** 0b10001 **/},	/** request call **/
	{10, 0x13 /** 0b10011 **/},	/** Confirmation call **/
	{11, 0x15 /** 0b10101 **/},	/** Preset dim 0 **/
	{12, 0x17 /** 0b10111 **/},	/** Preset dim 1 **/
	
	{13, 0x19 /** 0b11001 **/},	/** extern data **/
	{14, 0x1B /** 0b11011 **/},	/** status = on **/
	{15, 0x1D /** 0b11101 **/},	/** status = off **/
	{16, 0x1F /** 0b11111 **/},	/** status request **/
};


unsigned char num2house(unsigned char __num)
{
	if(__num <= 15)
	{
		return houseCode_tab[__num][1];
	}
	else
	{
		return 0;
	}
}

unsigned char house2num(unsigned char __house)
{
	if(__house <= 15)
	{
		return reverseHouseCode_tab[__house][1];
	}
	else
	{
		return 0;
	}
}

unsigned char num2key(unsigned char __num)
{	
	if(__num <= 15)
	{
		return keyCode_tab[__num][1];
	}
	else
	{
		return 0;
	}
}

unsigned char key2num(unsigned char __key)
{
	if(__key <= 15)
	{
		return reverseKeyCode_tab[__key][1];
	}
	else
	{
		return 0;
	}
}

unsigned char num2func(unsigned char __num)
{
	return funcCode_tab[__num][1];
}


/** 
 * 逻辑地址位图，用于表达地址是否可用 
 * 每一位表示一个地址的使用状态: ０表示此地址尚末使用，可以分配；１表示地址已占用，不可再分配给其它设备
 * 
 * 当前定下的最多1024个可用地址。需要考虑保留地址。
 **/
 
#define	LOGICADDRBITMAP_SIZE	(CTOTALADDRESS / (8 * sizeof(unsigned int)))
unsigned int g_uiLogicAddrBitMap[LOGICADDRBITMAP_SIZE];

/**
 * 地址映射表，用于表达逻辑地址与物理地址之间的关系
 * 每一项表示一个动态地址，可用每一项的索引号表示逻辑地址，对应的内容为物理地址
 *
 * 电力载波的地址只有８位，用一个字节可以表达
 **/
unsigned char g_addrTable[CTOTALADDRESS];

/*******************************************************************************
 * Note: buf 以递增方式处理，不回转
 *		 queue 环形队列处理
 *******************************************************************************/
static int isX10QueEmpty(X10Buf_queue_t *q)
{
	if(q->tail == q->head)
	{
		return TRUE;
	}
	return	FALSE;
}

static int isX10QueFull(X10Buf_queue_t *q)
{
	if((q->tail + 1 == q->head) || (((q->tail + 1) % X10QUEUESIZE) == q->head))
	{
		return TRUE;
	}
	return	FALSE;
}

void	X10QueueInit(X10Buf_queue_t *q)
{
	q->tail = 0;
	q->head = 0;
	q->len = 0;
}

int X10QueueIn(X10Buf_queue_t *q, X10Data_t *X10data)
{
    if(isX10QueFull(q))
    {
        return	FALSE;
    }

    //memcpy(&(q->buf[q->tail]), X10data, sizeof(X10data));
    
    q->buf[q->tail].houseCode = X10data->houseCode; 
    q->buf[q->tail].keyCode = X10data->keyCode; 
    q->buf[q->tail].funcCode = X10data->funcCode;
	q->tail = (q->tail + 1) % X10QUEUESIZE;
    
    return TRUE;
}

int X10QueueIn_irq(X10Buf_queue_t *q, X10Data_t *X10data)
{
	int iRet;

    IRQ_disable();
    iRet = X10QueueIn(q, X10data);
	IRQ_enable();
	    
	return	iRet;
}

int X10QueueOut(X10Buf_queue_t *q, X10Data_t *X10data)
{
    if(isX10QueEmpty(q))
    {
        return	FALSE;
    }
    
    //memcpy(chardata, &(q->buf[q->tail]), sizeof(CanTxMsg));
    X10data->houseCode = q->buf[q->head].houseCode;
    X10data->keyCode = q->buf[q->head].keyCode;
    X10data->funcCode = q->buf[q->head].funcCode;
	q->head = (q->head + 1) % X10QUEUESIZE;
       
    return TRUE;
}


int X10QueueOut_irq(X10Buf_queue_t *q, X10Data_t *X10data)
{
    int iRet;

    IRQ_disable();
    iRet = X10QueueOut(q, X10data);
	IRQ_enable();
	    
	return	iRet;
}
/////////////////////////////////////////////////////

/*******************************************************************************
 * name: logicAddrBitMap_find
 * prototype: 
 * description: find a bit that value is 0, then return the (offset + 1)
 * input: logic address bitmap
 * output: no
 * return: > 0 ok, it is the logic address
 *         = 0 reserved
 *         < 0 error, found no usable address
 *
 * example:
 * offset	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,...,  
 * return	1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,...,
 * Note: 
 *******************************************************************************/
int logicAddrBitMap_find(unsigned int * __addrBitMap)
{
	int i, j;
	int iRet;
	unsigned int tmp;
	for(i = 0; i < LOGICADDRBITMAP_SIZE; i++)
	{
		if(__addrBitMap[i] ^ 0xffffffff)
		{
			iRet = (i << 5);
			tmp = __addrBitMap[i];
			
			for(j = 0; j < (sizeof(unsigned int) << 3); i++)
			{
				if(tmp & 0x01 == 0)
				{
					iRet += j + 1;
					return	iRet;
				}
				else
				{
					tmp >>= 1;
				}
			}
		}
	}
	return ERROR;
}
/*******************************************************************************
 * name: logicAddrBitMap_test_set
 * prototype: 
 * description: test and set the specified bit of address bit map
 * input: logic address bitmap, the bit offset
 * output: no
 * return: the old value of specified bit
 *
 *******************************************************************************/
int logicAddrBitMap_test_set(unsigned int * __addrBitMap, int offset)
{
	unsigned int mask;
	unsigned int tmp;
	unsigned int oldval;
	
	tmp = __addrBitMap[offset >> 5];		/** line NO.  offset/32 **/
	mask = (1 << (offset & ((1 << 5) - 1)));
	
	oldval = tmp & mask;
	tmp |= mask;

	return oldval;
}

/*******************************************************************************
 * name: logicAddrAlloc
 * prototype: 
 * description: allocate a logic address according to physical address
 *
 * input: physical address
 * output: no
 * return: logic address
 * 
 *******************************************************************************/
int logicAddrAlloc(unsigned char physAddr)
{
	int logicAddr; 
	
	logicAddr = logicAddrBitMap_find(g_uiLogicAddrBitMap);
	if(logicAddr >= 1)
	{
		logicAddrBitMap_test_set(g_uiLogicAddrBitMap, logicAddr - 1);
		
		g_addrTable[logicAddr - 1] = physAddr;
	}
	
	return	logicAddr;
}

/*******************************************************************************
 * name: logic2phys
 * description: find physical address with specified logic address
 * input: logic address
 * output: no
 * return: physical address
 *
 *******************************************************************************/
int logic2phys(unsigned int __logicAddr)
{
	if(__logicAddr < CTOTALADDRESS)
	{
		return	g_addrTable[__logicAddr];
	}
	
	return	ERROR;
}

/*******************************************************************************
 * name: phys2logic
 * description: find logic address with specified physical address
 * input: physical address
 * output: no
 * return: logic address
 *
 *******************************************************************************/
int phys2logic(unsigned char __physAddr)
{
	int i;
	
	for(i = 0; i < CTOTALADDRESS; i++)
	{
		if((g_addrTable[i] ^ __physAddr) == 0)
		{
			return	i;
		}
	}
	
	return	ERROR;
}

/** 设备自举, 根据需要调整 **/
void dev_oneRegister(canFrame_queue_t *canFrame, unsigned int __NUM)
{
	int i;
	unsigned char buf[16];
	CanTxMsg TxMessage;

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

	memset(buf, 0, 16);

	//buf[0] = buf[1] = buf[2] = buf[3] = 0;
	buf[4] = (__NUM & 0xff);		//__NUM
	buf[5] = ((__NUM >> 8) & 0xff);	//house: P
	buf[6] = CDEV_TYPE;

	buf[10] = 0x12;
	buf[11] = 0x01;
	buf[15] = makeCrc8(0, buf, 15);

	for(i = 0; i <= 15; i++)
	{
		TxMessage.Data[4 + (i & 0x3)] = buf[i];

		if((((i + 1) & 0x03) == 0) /**  || ((i + 1) == length) **/ )   /** 4, 8, 12, ... , 4x, ...  or the last one **/
		{
			TxMessage.DLC = 5 + (i & 0x03);

			TxMessage.Data[2] = g_ucCanseq++;
			TxMessage.Data[3] = (i >> 2);
			CanTXqueueIn(canFrame, &TxMessage);
		}
	}
}

/** 设备自举, 根据需要调整 **/
void dev_allRegister(canFrame_queue_t *canFrame)
//void bootstrapping(canFrame_queue_t *canFrame)
{
	int i, j;
	unsigned char buf[16];
	CanTxMsg TxMessage;

	memset(buf, 0, 16);

	for(i = 0; i < 9; i++)
	{
		buf[0] = buf[1] = buf[2] = buf[3] = 0;
		buf[4] = i;		//key: i
		buf[5] = 0x0f;	//house: P

		buf[10] = 0x12;
		buf[11] = 0x01;
		buf[15] = makeCrc8(0, buf, 15);

		for(j = 0; j <= 15; j++)
		{
			TxMessage.Data[4 + (j & 0x3)] = buf[j];

			if((((j + 1) & 0x03) == 0) /**  || ((i + 1) == length) **/ )   /** 4, 8, 12, ... , 4x, ...  or the last one **/
			{
				TxMessage.DLC = 5 + (j & 0x03);

				TxMessage.Data[2] = g_ucCanseq++;
				TxMessage.Data[3] = (j >> 2);
				CanTXqueueIn(canFrame, &TxMessage);
			}
		}


	}
}
