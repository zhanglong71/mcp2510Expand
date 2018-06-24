#ifndef __CONST_H__
#define __CONST_H__


/*******************************************************************************
 *
 * All constant define here
 *
 *******************************************************************************/
#define	STACKSIZE	8
#define	QUEUESIZE	8

#define	OK	0
#define	ERROR	-1

#define	TRUE	1
#define	FALSE	0

#define CSEND_BUF_NUM	8	//16	 //缓冲区个数
#define CSEND_BUF_SIZE	128	//64	 //缓冲区大小

#define TIMER_NUM	1       /** 定时器个数 **/

//#define	CCOUNT_PER_1MS	50	/** 每1ms的中断次数8k对应8, 20k对应20, 50k对应50 **/
#define	CCOUNT_PER_1MS	25	/** 每1ms的中断次数25k对应25 **/
#define CTIMER_1MS 	(CCOUNT_PER_1MS)

#define CTIMER_10MS 	((CTIMER_1MS) * 10)
#define CTIMER_20MS 	((CTIMER_1MS) * 20)	
#define CTIMER_30MS 	((CTIMER_1MS) * 30)
#define CTIMER_100MS	((CTIMER_1MS) * 100)
#define CTIMER_200MS	((CTIMER_100MS) * 2)	
#define CTIMER_400MS	((CTIMER_100MS) * 4)
#define CTIMER_500MS	((CTIMER_100MS) * 5)	
#define CTIMER_800MS	((CTIMER_100MS) * 8)
#define CTIMER_1SEC		((CTIMER_100MS) * 10)

#define CTIMER_2SEC	((CTIMER_1SEC) * 2)
#define CTIMER_10SEC	((CTIMER_1SEC) * 10)

#define CTIMER_INFRARED_RECORDTIMEOUT	(CTIMER_10SEC)	/** 10秒录入超时 **/
#define CTIMER_INFRARED_TIMEOUT	((CTIMER_10SEC) + CTIMER_100MS)	/** 8kHz定时, 10秒计数次数 **/
#define CTIMER_INFRARED_SAMPLING_MAX	(CTIMER_20MS)	/** 采样结束 **/


#define CDEV_TYPE	3

#define CHARQUEUESIZE   128
#define X10QUEUESIZE   16

#define CANQUEUESIZE    (128 >> 2)

/*******************************************************************************/
#define FILTER_SID  (0x01)
#define FILTER_EID  (0x0)
#define FILTER_ID    ((FILTER_SID << 18) | (FILTER_EID))
//#define FILTER_ID   ((u32)((FILTER_SID << 18) | (FILTER_EID)) << 3) | CAN_ID_EXT | CAN_RTR_DATA)

#define MASK_SID    (0x7f)
#define MASK_EID    (0x0)
#define FILTER_MASK    ((MASK_SID << 18) | (MASK_EID))

/*******************************************************************************/
#define ASCII_STX   0x02
#define ASCII_ETX   0x03

/*******************************************************************************/
#define	CTOTALADDRESS	1024

/*** 
 * 此参数非常敏感。 在与TDEX6438+对接的最早的"开发板"中，用12可正常控制，
 * 但是在新开发板中，12不能工作。换成6，刚上电时也可以工作，但过不了几分钟就又不能控制.
 * 中断实现代码决定了，换成5是不能工作的. 2013/1/25 15:44 现已解决此问题
 *
 **/
//#define	CX10_SYNC_OFFSET	12
//#define	CX10_SYNC_OFFSET	9
//#define	CX10_SYNC_OFFSET	6
//#define	CX10_SYNC_OFFSET	5
#define	CX10_SYNC_OFFSET	4
//#define	CX10_SYNC_OFFSET	3
/*******************************************************************************/
#define	CFLASH_PAGE_SIZE	1024

#define	CENTRYFLAG_BUSY	0xa5	/** 表示指定的项已被使用 **/
#define	CENTRYFLAG_IDLE	0xff	/** 表示指定的项未被使用 **/

#define	MAGIC_SIZE 4
#define	MAGIC_SIZE_OFFSET	1020	//MAGIC code所在地址的页内偏移(1020,1021,1022,1023)

/*******************************************************************************/
#define	CSAMPLING_BIT_LEN	120			//一条红外信号码项占用的空间(也是长度数据在缓冲区中的起始偏移)
#define	CSAMPLING_BIT_VALID	121			//当前红外信号码项有效(与CENTRYFLAG_BUSY进行比较)
#define	CHEADER_OF_ENTRY	122			//起头位置时长数据在缓冲区中的起始偏移地址
#define	CBYTES_OF_ENTRY		128			//一条红外信号码项占用的空间
/*******************************************************************************/

#endif
