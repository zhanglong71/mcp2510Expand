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
#define TIMER_1SEC  8000	/** 8kHz定时, 每一秒计数次数 **/
#define TIMER_10SEC 80000	/** 8kHz定时, 10秒计数次数 **/
#define TIMER_CALIB 800000	/** 8kHz定时, 100秒超时退出 **/
#define TIMER_RGST  800		/** 注册时间间隔 **/
#define CTIMER_CALIB (16000)	/** 进入校正状态需要持续按下按键的时间, 斩定为2s **/


#define CDEV_TYPE	2

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
#define	MAGIC_SIZE 4
/*******************************************************************************/
#endif
