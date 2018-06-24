#ifndef __PTYPE_H__
#define __PTYPE_H__

/*******************************************************************************
 *
 * All data type define here
 *
 *******************************************************************************/

typedef enum {
    FAILED = 0, 
    PASSED = !FAILED
} RetStatus;

typedef	int	(*pfunc_t)(unsigned *arg);	

typedef struct func_s {
    pfunc_t		func;       /** function **/
    unsigned int  *arg;    /** argument **/
} func_t;

typedef struct func_stack{
	int		top;
	func_t		func[STACKSIZE]; 
}fstack_t;

typedef enum {
    CMSG_NONE = 0, 
    CMSG_INIT, 
    CMSG_TMR, 
    CMSG_CANTX,
    CMSG_CANRX,
    CMSG_COMTX,
    CMSG_COMRX,
    
	//CMSG_WLSTX,
    //CMSG_WLSRX,
    
    CMSG_QUECH,
    CMSG_RGST,		/** 设备报到注册 **/   
    CMSG_SAVE,		/** 保存新的值 **/



    CINFR_RCV,	/** 收到完整的红外信号 **/

    CUART1_RCV,
    CUART2_RCV,
    CCAN_RCV,
    CMSG_COMTIMEOUT,
} msgType_t;

typedef struct msg_queue{
	int	head;
	int	tail;
	msgType_t	buf[QUEUESIZE];
} msgq_t;

typedef struct Timer_s {
    unsigned int tick_bak;
    unsigned int tick;
    //unsigned int count;                      /** Is it necessary ? **/
}Timer_t; 

typedef struct bitmap_s {
    unsigned int bitmap;		    /** 32 bits **/
} bitmap_t;


typedef struct canFrame_queue_s{
	int	head;
	int	tail;
	/** 
	 * flag: the queue attribute 
	 *
	 * bit0: the can device disable or not
	 **/
	int flag;
	CanTxMsg	buf[CANQUEUESIZE];
} canFrame_queue_t;

typedef enum {              /** 用于控制com数据的发送 **/
    CTRL_START = 0,         /** 0 - start transmission **/
    CTRL_CONTI = 1,         /** 1 - continue transmission  **/
                            /** 2 reserved. for other control **/
    CTRL_TMR = 5,           /** 3..200 for delay **/
    CTRL_STOP = 255,        /** 255 - stop transmission**/
} ctrlFlag_t;

typedef struct charData_s{
	unsigned char ucVal;
	ctrlFlag_t ctrl;         /** 控制字段，用于表达当前的data是有效数据还是延迟或其它功能 **/
} charData_t;

typedef struct charBuf_queue_s {
    int	head;
		int	tail;
    int len;
    unsigned char buf[CHARQUEUESIZE];     /** 128 bytes **/
} charBuf_queue_t;

/*******************************************************************************/
typedef union flashPage_u {
	unsigned char arrChar[CFLASH_PAGE_SIZE];
	u32  arrInt[CFLASH_PAGE_SIZE/4];
} flashPage_t;
/*******************************************************************************/
typedef struct Phase_sampling_s {
	//int head[2];
	short head[2];
	unsigned char  buf[CBYTES_OF_ENTRY];
	unsigned char  addr[4];		//采样用到。记录命令的前4byte，da0da1da2da3
	unsigned char  cmd[4];		//采样用到。记录命令如：0x42+0x01+0x01+0x00
} Phase_sampling_t;


#endif /** ifndef end **/
