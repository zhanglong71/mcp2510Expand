#ifndef _MEMDEV_H_
#define _MEMDEV_H_

#include "huaRainType-base.h"

#include <linux/cdev.h>
#include <linux/interrupt.h>

//#define PDEBUG
#undef PDEBUG
#ifdef PDEBUG
#define PLOG(fmt,args...) printk(KERN_WARNING "DEBUG:" fmt,##args)
#else
#define PLOG(frm,args...)
#endif

/***********************************************/

typedef int (*cmp_t)(void *arg1, void *arg2);
typedef void (*print_t)(const char* string1, int lineNo, struct CANBus_Message* pCanBusMsg);


/** �����ݽṹ��������tasklet����ע������ **/
typedef struct rgstCmd_s {
    recvCmd_t Cmd;
    wait_queue_head_t  rgstq;      /** ע����Ϣͬ�� **/
    //unsigned char ucAdapterNo;
    unsigned char ucErr;           /** ʧ�ܴ��� **/
    atomic_t atRgst;     /** ��ע�����ݰ��ȴ�����ͬ��֮�� **/
    //unsigned char ucRgst;     /** ��ע�����ݰ��ȴ�����ͬ��֮�� **/
}rgstCmd_t;

/** ������ʱ��������֮�� **/
typedef struct CANdev_Message_s {
    union DevCmd_s stDevCmd;
    unsigned short usLength;
    unsigned char ucAdapterNo;
}CANdev_Message_t;

/** The DataArea of device **/
typedef struct canbusDevice_s {
    struct IdentifyMsg_s    stIdMsg;
    /** current work state **/
    unsigned char ucDispName[32];   /** Display name **/
    unsigned short usAttr[4];       /** ��Ӧ������, ���Ϊ���Ա�� **/
    //unsigned short usConf[4];       /** ��Ӧ������, ���Ϊ���ñ�� **/

    unsigned char ucIdValid;        /** �豸�Ƿ���Ч **/
    unsigned char ucStatValid;      /** ��̬��Ϣ�Ƿ���Ч **/
    wait_queue_head_t  rStatq;      /** ����̬��Ϣʱͬ��֮�� **/
    unsigned long ulJiffies;        /** ���һ���յ���Ϣ��ʱ��(�Դ��ں�������ʱ��)  **/
    unsigned char ucAdapterNo;      /** �豸���ڵ���������� **/
    
    unsigned char ucPairAddr[4];   /** ͨ�ŶԶ˵������ַ **/
    unsigned char ucPairAdapterNo; /** ͨ�ŶԶ��豸���ڵ���������� **/
    unsigned char ucCmdOpcode;     /** Rmt��������� **/
    unsigned char ucCmdAttr;       /** Rmt���� - ���Ա�� **/
    unsigned short usCmdValue;     /** Rmt���� - ����ֵ **/
    unsigned char ucCmdCnt;       /** Rmt���� - ���� **/
//    unsigned char ucCmdAck;       /** Rmt���� - reserved **/
    unsigned char ucCmdWaitAck;   /** Rmt���� - �ȴ�Ӧ�� **/

    struct list_head stNoNode;      /** ��Ҫ����Ӧ�ò��豸�ļ��������� **/
    struct list_head stDevList;     /** ͬ�����͵��豸������(ʵ�����ȼ���)  **/
    struct list_head stRmtCmd;      /** RMT��Ӧ �������Ӽ� **/
    struct rb_node node_rb;         /** ��������  **/
    struct cdev dev;
    //struct class_device *pClassDev;
    struct device *pDevice;
    dev_t  no;

} canbusDevice_t;

typedef struct deviceType_s {
    dev_t devNO;                    /** It is stored the first device NO. **/
    unsigned int uiMaxMinorNo;      /** ��ǰ�����ŵ����õ��������豸�� **/
    struct file_operations* pstFops;    /** �����ӿڵĽṹ�塣��֧�ֵ��豸���������й� **/
    char* pDesc;
    struct list_head stDevList;     /** the list of same device type dataarea **/
} deviceType_t;

typedef struct RcvCanFrm_s {
    struct CANBus_Message   stRcvCanFrm;
    struct rb_node node_rb;         /** ��������, ���ӵ���Ӧ��������rbtree�� **/
}RcvCanFrm_t;

typedef struct AdapterNO_s {
    unsigned int uiEID;
    unsigned short usSID;

    //unsigned short usSize;        /** �Ӵ����������յ��İ��ĸ��� **/
    atomic_t atCompleteNo;          /** �Ѿ��Ӹ��������������������ݰ�, ���ȴ����� **/
    //unsigned long ulJiffies;      /** ���һ�δӴ����������յ�����ʱ�� **/
    struct rb_root rcvDataRoot_rb;  /** �Ӵ����������յ����ݺ�����rbtree **/
    int iReceivedData;              /** �Ƿ��յ���ת�����ϵ����� **/
    int iPollInterval;              /** ��ʼ��ѯʱ���� **/
}AdapterNO_t;

typedef struct DevNum_s {
    unsigned short usVendorNo;
    unsigned short usProductNo;
}DevNum_t;

typedef struct DevNoNodeList_s {
    spinlock_t spinlock;            /** CAN�Ĳ���ͬ��(ʵ�����Ƕ�spi��ͬ��, ����spi�����ĵط���Ҫ�õ�) **/
    struct list_head stNoNode;      /** ������ **/
}DevNoNodeList_t;

typedef struct RmtCmdNode_s {
    struct rb_node node_rb;         /** ��������, ���ӵ���Ӧң�������rbtree�� **/
    struct canbusDevice_s *devNode; /** ң������Ҫ����Ľڵ� **/
} RmtCmdNode_t;

typedef struct sndPktnode_s {
    struct list_head sndPktListNode;     /** node FIFO: wait for send via canbus **/
    struct CANBus_Message   sndMsg;     /** Ҫ���͵����� **/
} sndPktnode_t;

typedef struct RmtCmdList_s {
    spinlock_t spinlock;            /** for sync **/
    struct rb_root RcvDirectCmdRoot_rb; /** �յ�ֱ�Ӳ����豸�����ݺ�����rbtree **/
    struct rb_root RcvSmplexRmtCmdRoot_rb;   /** �յ�����ң�����ݺ�����rbtree **/
    struct rb_root RcvDuplexRmtCmdRoot_rb;   /** �յ�˫��ң�ز������ݺ�����rbtree **/
    struct rb_root SndDuplexRmtAckRoot_rb;   /** ����˫��ң�ز����й����rbtree **/
    atomic_t atTimerExpire;  /** ����״̬�¶�ʱ�ȴ����� **/
    struct list_head sndPktfifoHead;    /** node FIFO: wait for send via canbus **/

    struct kfifo *pRcvBuff;             /** CAN�������ݻ����� **/
    struct kfifo *pSndBuff;             /** CAN�������ݻ����� **/
    spinlock_t sndfifoBaseLock;         /** ����CAN�������ݻ�������������. ������kfifo֮�� **/
    spinlock_t rcvfifoBaseLock;         /** ����CAN�������ݻ�������������. ������kfifo֮�� **/
    spinlock_t sndfifoLock;             /** ����CAN�������ݻ������������� **/

    int iErrCount;      /** canbus��������Ĵ����� canbusһ�����ͳɹ������� **/

    struct rb_root DoingRmtCmdRoot_rb;  /** ���ڴ�������ݹ����rbtree **/
    wait_queue_head_t  rmtCmdq;         /** �ȴ�ң������ʱͬ��֮�� **/
    wait_queue_head_t  rawDataq;        /** �յ�raw����ʱ����kfifoͬ��֮�� **/
    struct task_struct *task;
}RmtCmdList_t;

/**
 * white name list 
 *
 * I think not only Addr, but also Vendor, 
 **/
typedef struct whiteListNode_s {    /** white name list**/
    unsigned char ucPhysAddr[4];    /** �����ַ **/
    //unsigned char ucExtendAddr[4];  /** �����ַ **/
    unsigned char ucDispName[32];   /** Display name **/
    unsigned char ucValid;            /** the entry valid or not **/
    struct rb_node node_rb;         /** rbtree node **/
} whiteListNode_t;
typedef struct whiteListHead_s {    /** white name list Head **/
    int iCount;                     /** total number of white list entry **/
    struct rb_root whiteListRoot_rb;  /** �Ӵ����������յ����ݺ�����rbtree **/
    int iEnable;         /** Enable white list or not **/
} whiteListHead_t;
/** white name list **/

/** proc file **/
typedef struct proc_intf_s {        /** export the device messages **/
    struct proc_dir_entry *proc_dir;
    struct proc_dir_entry *proc_entry[9];
} proc_intf_t;
/** proc file **/

typedef struct devInfoHead_s {
    int iCount;                         /** �ڵ���� **/
    struct rb_root DevInfoRoot_rb;    /** �������ڵ�rbtree���ĸ� **/
} devInfoHead_t;

typedef struct host_s {
    int iEnable;   
    int ifirstPoll;   
    struct class *HuaRain_class;
    struct rgstCmd_s rgstCmd;
    struct devInfoHead_s DevInfoHead; /** �������ڵ�rbtree���ĸ� **/

    spinlock_t canLock;              //CAN�Ĳ���ͬ��(ʵ�����Ƕ�spi��ͬ��, ����spi�����ĵط���Ҫ�õ�)
    struct work_struct can_work;      /** used creat character device node **/
    struct tasklet_struct can_tasklet;
    struct timer_list can_timer;      /** broadcast timer **/
    int iTimerEnable;       /** Enable timer or not **/ 
    int iTimerPollPeriod;   /** poll timer period **/ 
    int iTimerPollDelay;    /** next poll timer delay **/ 
    int devNodeFromWhite;   /** create devNode when initial white list **/ 

    struct RmtCmdList_s stRmtCmdList; /** remote cmd list that wait for response **/

} host_t;
/***********************************************/
void register_canDev(dev_t __devno, int __num, struct file_operations* fops);
void unregister_canDev(int num);

//ssize_t mcp2510Pkt_write(unsigned char cmd, const unsigned short *__buf, size_t size, void* DevInfo);
ssize_t mcp2510Pkt_write(unsigned char cmd, const unsigned char *__buf, size_t size, void* DevInfo);

ssize_t mcp2510Pkt_writeCmdX(const struct sendCmd_s *data, const int interval);
struct AdapterNO_s * getAdapterTabAddr(void);

unsigned char makeCrc8(unsigned char crc8, unsigned char *ptr, unsigned char len);

/***********************************************/

#endif

/**********************************************************
s3c2440��mcp2510֮�������
SPIMISO0  GPE11
SPIMOSI0  GPE12
SPICLK0   GPE13

SPI CS0   GPG14
CAN INT   GPG13
CAN RST   GPG2
**********************************************************/
 
