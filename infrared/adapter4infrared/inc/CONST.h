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

//#define	CCOUNT_PER_1MS	50	/** ÿ1ms���жϴ���8k��Ӧ8, 20k��Ӧ20, 50k��Ӧ50 **/
#define	CCOUNT_PER_1MS	25	/** ÿ1ms���жϴ���25k��Ӧ25 **/
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

#define CTIMER_INFRARED_RECORDTIMEOUT	(CTIMER_10SEC)	/** 10��¼�볬ʱ **/
#define CTIMER_INFRARED_TIMEOUT	((CTIMER_10SEC) + CTIMER_100MS)	/** 8kHz��ʱ, 10��������� **/
#define CTIMER_INFRARED_SAMPLING_MAX	(CTIMER_20MS)	/** �������� **/


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

#define	CENTRYFLAG_BUSY	0xa5	/** ��ʾָ�������ѱ�ʹ�� **/
#define	CENTRYFLAG_IDLE	0xff	/** ��ʾָ������δ��ʹ�� **/

#define	MAGIC_SIZE 4
#define	MAGIC_SIZE_OFFSET	1020	//MAGIC code���ڵ�ַ��ҳ��ƫ��(1020,1021,1022,1023)

/*******************************************************************************/
#define	CSAMPLING_BIT_LEN	120			//һ�������ź�����ռ�õĿռ�(Ҳ�ǳ��������ڻ������е���ʼƫ��)
#define	CSAMPLING_BIT_VALID	121			//��ǰ�����ź�������Ч(��CENTRYFLAG_BUSY���бȽ�)
#define	CHEADER_OF_ENTRY	122			//��ͷλ��ʱ�������ڻ������е���ʼƫ�Ƶ�ַ
#define	CBYTES_OF_ENTRY		128			//һ�������ź�����ռ�õĿռ�
/*******************************************************************************/

#endif
