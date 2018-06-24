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

#define CSEND_BUF_NUM	8	//16	 //����������
#define CSEND_BUF_SIZE	128	//64	 //��������С

#define TIMER_NUM	1       /** ��ʱ������ **/
#define TIMER_1SEC  8000	/** 8kHz��ʱ, ÿһ��������� **/
#define TIMER_10SEC 80000	/** 8kHz��ʱ, 10��������� **/
#define TIMER_CALIB 800000	/** 8kHz��ʱ, 100�볬ʱ�˳� **/
#define TIMER_RGST  800		/** ע��ʱ���� **/
#define CTIMER_CALIB (16000)	/** ����У��״̬��Ҫ�������°�����ʱ��, ն��Ϊ2s **/


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
 * �˲����ǳ����С� ����TDEX6438+�Խӵ������"������"�У���12���������ƣ�
 * �������¿������У�12���ܹ���������6�����ϵ�ʱҲ���Թ������������˼����Ӿ��ֲ��ܿ���.
 * �ж�ʵ�ִ�������ˣ�����5�ǲ��ܹ�����. 2013/1/25 15:44 ���ѽ��������
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
