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


/** 此数据结构体用于向tasklet传递注册命令 **/
typedef struct rgstCmd_s {
    recvCmd_t Cmd;
    wait_queue_head_t  rgstq;      /** 注册信息同步 **/
    //unsigned char ucAdapterNo;
    unsigned char ucErr;           /** 失败次数 **/
    atomic_t atRgst;     /** 有注册数据包等待处理，同步之用 **/
    //unsigned char ucRgst;     /** 有注册数据包等待处理，同步之用 **/
}rgstCmd_t;

/** 用于临时整理数据之用 **/
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
    unsigned short usAttr[4];       /** 对应各属性, 序号为属性编号 **/
    //unsigned short usConf[4];       /** 对应各配置, 序号为配置编号 **/

    unsigned char ucIdValid;        /** 设备是否有效 **/
    unsigned char ucStatValid;      /** 动态信息是否有效 **/
    wait_queue_head_t  rStatq;      /** 读动态信息时同步之用 **/
    unsigned long ulJiffies;        /** 最后一次收到信息的时间(以此内核心跳作时间)  **/
    unsigned char ucAdapterNo;      /** 设备所在的适配器编号 **/
    
    unsigned char ucPairAddr[4];   /** 通信对端的物理地址 **/
    unsigned char ucPairAdapterNo; /** 通信对端设备所在的适配器编号 **/
    unsigned char ucCmdOpcode;     /** Rmt命令操作码 **/
    unsigned char ucCmdAttr;       /** Rmt命令 - 属性编号 **/
    unsigned short usCmdValue;     /** Rmt命令 - 属性值 **/
    unsigned char ucCmdCnt;       /** Rmt命令 - 计数 **/
//    unsigned char ucCmdAck;       /** Rmt命令 - reserved **/
    unsigned char ucCmdWaitAck;   /** Rmt命令 - 等待应答 **/

    struct list_head stNoNode;      /** 需要建立应用层设备文件的链表结点 **/
    struct list_head stDevList;     /** 同种类型的设备链表结点(实现优先级低)  **/
    struct list_head stRmtCmd;      /** RMT响应 链表连接件 **/
    struct rb_node node_rb;         /** 红黑树结点  **/
    struct cdev dev;
    //struct class_device *pClassDev;
    struct device *pDevice;
    dev_t  no;

} canbusDevice_t;

typedef struct deviceType_s {
    dev_t devNO;                    /** It is stored the first device NO. **/
    unsigned int uiMaxMinorNo;      /** 当前主备号的已用到的最大次设备号 **/
    struct file_operations* pstFops;    /** 操作接口的结构体。与支持的设备的种类数有关 **/
    char* pDesc;
    struct list_head stDevList;     /** the list of same device type dataarea **/
} deviceType_t;

typedef struct RcvCanFrm_s {
    struct CANBus_Message   stRcvCanFrm;
    struct rb_node node_rb;         /** 红黑树结点, 连接到相应适配器的rbtree中 **/
}RcvCanFrm_t;

typedef struct AdapterNO_s {
    unsigned int uiEID;
    unsigned short usSID;

    //unsigned short usSize;        /** 从此适配器上收到的包的个数 **/
    atomic_t atCompleteNo;          /** 已经从该适配器上收完整的数据包, 并等待处理 **/
    //unsigned long ulJiffies;      /** 最后一次从此适配器上收到包的时刻 **/
    struct rb_root rcvDataRoot_rb;  /** 从此适配器上收到数据后挂入此rbtree **/
    int iReceivedData;              /** 是否收到该转换器上的数据 **/
    int iPollInterval;              /** 初始轮询时间间隔 **/
}AdapterNO_t;

typedef struct DevNum_s {
    unsigned short usVendorNo;
    unsigned short usProductNo;
}DevNum_t;

typedef struct DevNoNodeList_s {
    spinlock_t spinlock;            /** CAN的操作同步(实际上是对spi的同步, 凡是spi操作的地方都要用到) **/
    struct list_head stNoNode;      /** 链表结点 **/
}DevNoNodeList_t;

typedef struct RmtCmdNode_s {
    struct rb_node node_rb;         /** 红黑树结点, 连接到相应遥控命令的rbtree中 **/
    struct canbusDevice_s *devNode; /** 遥控命令要处理的节点 **/
} RmtCmdNode_t;

typedef struct sndPktnode_s {
    struct list_head sndPktListNode;     /** node FIFO: wait for send via canbus **/
    struct CANBus_Message   sndMsg;     /** 要发送的数据 **/
} sndPktnode_t;

typedef struct RmtCmdList_s {
    spinlock_t spinlock;            /** for sync **/
    struct rb_root RcvDirectCmdRoot_rb; /** 收到直接操作设备的数据后挂入此rbtree **/
    struct rb_root RcvSmplexRmtCmdRoot_rb;   /** 收到单向遥控数据后挂入此rbtree **/
    struct rb_root RcvDuplexRmtCmdRoot_rb;   /** 收到双工遥控操作数据后挂入此rbtree **/
    struct rb_root SndDuplexRmtAckRoot_rb;   /** 处理双工遥控操作中挂入此rbtree **/
    atomic_t atTimerExpire;  /** 空闲状态下定时等待处理 **/
    struct list_head sndPktfifoHead;    /** node FIFO: wait for send via canbus **/

    struct kfifo *pRcvBuff;             /** CAN接收数据缓冲区 **/
    struct kfifo *pSndBuff;             /** CAN发送数据缓冲区 **/
    spinlock_t sndfifoBaseLock;         /** 用于CAN发送数据缓冲区的自旋锁. 寄生在kfifo之中 **/
    spinlock_t rcvfifoBaseLock;         /** 用于CAN接收数据缓冲区的自旋锁. 寄生在kfifo之中 **/
    spinlock_t sndfifoLock;             /** 用于CAN发送数据缓冲区的自旋锁 **/

    int iErrCount;      /** canbus连续出错的次数。 canbus一旦发送成功就清零 **/

    struct rb_root DoingRmtCmdRoot_rb;  /** 正在处理的数据挂入此rbtree **/
    wait_queue_head_t  rmtCmdq;         /** 等待遥控命令时同步之用 **/
    wait_queue_head_t  rawDataq;        /** 收到raw数据时放入kfifo同步之用 **/
    struct task_struct *task;
}RmtCmdList_t;

/**
 * white name list 
 *
 * I think not only Addr, but also Vendor, 
 **/
typedef struct whiteListNode_s {    /** white name list**/
    unsigned char ucPhysAddr[4];    /** 物理地址 **/
    //unsigned char ucExtendAddr[4];  /** 物理地址 **/
    unsigned char ucDispName[32];   /** Display name **/
    unsigned char ucValid;            /** the entry valid or not **/
    struct rb_node node_rb;         /** rbtree node **/
} whiteListNode_t;
typedef struct whiteListHead_s {    /** white name list Head **/
    int iCount;                     /** total number of white list entry **/
    struct rb_root whiteListRoot_rb;  /** 从此适配器上收到数据后挂入此rbtree **/
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
    int iCount;                         /** 节点个数 **/
    struct rb_root DevInfoRoot_rb;    /** 数据区节点rbtree树的根 **/
} devInfoHead_t;

typedef struct host_s {
    int iEnable;   
    int ifirstPoll;   
    struct class *HuaRain_class;
    struct rgstCmd_s rgstCmd;
    struct devInfoHead_s DevInfoHead; /** 数据区节点rbtree树的根 **/

    spinlock_t canLock;              //CAN的操作同步(实际上是对spi的同步, 凡是spi操作的地方都要用到)
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
s3c2440与mcp2510之间的连接
SPIMISO0  GPE11
SPIMOSI0  GPE12
SPICLK0   GPE13

SPI CS0   GPG14
CAN INT   GPG13
CAN RST   GPG2
**********************************************************/
 
