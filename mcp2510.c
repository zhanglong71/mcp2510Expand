#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/kfifo.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/wait.h>  
#include <linux/sched.h>  
#include <linux/string.h>  
#include <linux/vmalloc.h>  
#include <mach/irqs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zl");
MODULE_VERSION("V1.0");
MODULE_DESCRIPTION("MCP2510 CANbusModule");

#include "mcp2510.h"
#include "crc8.c"
EXPORT_SYMBOL(makeCrc8);
#include "spi.c"
#include "lib/rbtreeDataArea.h"
#include "lib/rbtreeRcvData.h"
#include "lib/rbtreeRmtCmd.h"
#include "lib/rbtreeWhiteList.h"
#include "protocol.h"
//#include "CanDev.h"

static char version[16] = "1.0.0";
static char date[32] = __DATE__" "__TIME__;

static struct canbusDevice_s *ptmpDevNode; //设备节点(有必要设为静态全局变量 ? )

static struct CANdev_Message_s stTmp;   //整理数据用的临时缓冲区

struct CANBus_Message *pIntMsg;     //make sure only inturrupt use it
struct CANBus_Message *pBhMsg;      //make sure only bottom half use it
recvCmd_t   bh_RcvCmd;              //make sure only bottom half use it

struct DevNoNodeList_s stNoNodeList;    //the list of dataarea created just, then need to create character device node

/** 
 * 设备类型注册表
 * 注册一个设备类型的驱动(填写相应的表格)后
 *
 * 设计之初，考虑到各种不同的设备有不同的操作，故而有了此设备类型表格  
 * 到目前为止(2012-12-20 14:16:38)不觉得此功能分类的必要，或者认为终端设备类型是一个更微观的对象，而是觉得对驱动类型分类的必要
 * 偿试将其作为驱动类型的分类，其数据类型有必要做相应修改.如,可将设备类型相关的描述以数组的形式列在驱动的各个元素中, 这样就形成
 * 驱动类型到设备类型的细化.
 *
 **/
struct deviceType_s devTypeTab[MMaxTypeNum] = {
    [0] = {
        .devNO = 0,          /** The first element is reserved **/
        .uiMaxMinorNo = 0,
        .pDesc = "NONE",
    },
    [1] = {
        .pDesc = "canbus-zigbee",
    },
    [2] = {
        .pDesc = "canbus-x10",
    },
    [3] = {
        .pDesc = "canbus-LockBy315M",
    },
    [4] = {
        .pDesc = "canbus-infrared",
    },
    [5] = {
        .pDesc = "canbus-",
    },


};
/** 适配器编号及对应的(SID + EID) **/
struct AdapterNO_s AdapterNoTab[] = {

/**
 * typedef struct AdapterNO_s {
 *  unsigned int uiEID;     //adapter NO.
 *  unsigned short usSID;   //host address
 *
 *  atomic_t atCompleteNo;
 *  struct rb_root rcvDataRoot_rb;
 * }AdapterNO_t;
 **/

    /** reserved **/
   [0].uiEID = 0x0,     
   [0].usSID = 0x0,
   [0].atCompleteNo = ATOMIC_INIT(0),
   [0].rcvDataRoot_rb = RB_ROOT,
   [0].iReceivedData = 0,
   [0].iPollInterval = 0,

   [1] =  {0x01, 0x00, ATOMIC_INIT(0), RB_ROOT, 0, 1},
   [2] =  {0x02, 0x00, ATOMIC_INIT(0), RB_ROOT, 0, 1},
   [3] =  {0x03, 0x00, ATOMIC_INIT(0), RB_ROOT, 0, 1},

#if 1
   [4] =  {0x04, 0x00, ATOMIC_INIT(0), RB_ROOT, 0, 1},
   [5] =  {0x05, 0x00, ATOMIC_INIT(0), RB_ROOT, 0, 1},
   [6] =  {0x06, 0x00, ATOMIC_INIT(0), RB_ROOT, 0, 1},
   [7] =  {0x07, 0x00, ATOMIC_INIT(0), RB_ROOT, 0, 1},

   [8] =  {0x08, 0x00, ATOMIC_INIT(0), RB_ROOT, 0, 1},
   [9] =  {0x09, 0x00, ATOMIC_INIT(0), RB_ROOT, 0, 1},
   [10] = {0x0a, 0x00, ATOMIC_INIT(0), RB_ROOT, 0, 1},
   [11] = {0x0b, 0x00, ATOMIC_INIT(0), RB_ROOT, 0, 1},

   [12] = {0x0c, 0x00, ATOMIC_INIT(0), RB_ROOT, 0, 1},
   [13] = {0x0d, 0x00, ATOMIC_INIT(0), RB_ROOT, 0, 1},
   [14] = {0x0e, 0x00, ATOMIC_INIT(0), RB_ROOT, 0, 1},
   [15] = {0x0f, 0x00, ATOMIC_INIT(0), RB_ROOT, 0, 1},
   [16] = {0x010, 0x0, ATOMIC_INIT(0), RB_ROOT, 0, 1},
#endif
};

/** 由（厂商编号+产品编号）定位到产品的信息 **/
struct DevNum_s DevNumTab[/** MMaxVendorNum **/][MMaxTypeNum] = {
   [0][0] =  {0x0, 0x0},
   [1][1] =  {0x00, 0x01},
   [2][1] =  {0x00, 0x02},

   [5][5] =  {0x12, 0x34},
};

/** 
 * 0 - 初始化后开始轮询的延时
 * 1 - 轮询(或正常数据发送)后到下一次轮询的时间间隔
 * 2 - idle状态下的轮询间隔(不能小于1s)
 *
 * final - 
 **/
int tmr_interval[] = {
    3, 10, 3, 1, 
    10, 3
    //60, 20, 10, 3
};

struct whiteListHead_s whiteListHead = {
    0, RB_ROOT, 0
};
struct proc_intf_s proc_intf;
/** 整个系统的功能开关, 默认开启，但在调整参数时，关闭 **/
struct host_s host;

//static char buffer[PAGE_SIZE];
/**********************************************************************************************************
 * 
 * Data flow figure and spinlock
 *
 *              +------------+    rcvInt     +-------------------+     readFunc     +-------------+
 *              | canLock    |  ---------->  | Use rcvfifoLock   |  ------------->  |             |
 *              |   MCP2510  |    SndInt     +-------------------+     writeFunc    | userSpace   |
 *              |   (SPI)    |  <----------  | Use sndfifoLock   |  <-------------  |             |
 *              +------------+               +-------------------+                  +-------------+
 *
 *  read:       rcvfifo --> userSpace : use rcvfifoLock and disable local inturrput
 *  write:      user    --> sndfifo   : use sndfifoLock and disable local inturrput
 *  interrput:  mcp2510 --> rcvfifo(rcvInt) : use canLock and rcvfifoLock
 *              sndfifo --> mcp2510(sndInt) : use canLock and sndfifoLock
 *
 **********************************************************************************************************/
/** host controlor driver initial **/
static int hostCD_init(struct host_s *pHost)
{
    int iRet = 0;

    pHost->iEnable = 0;
    pHost->ifirstPoll = 0;
    pHost->iTimerPollPeriod = 3;
    pHost->iTimerPollDelay = 10;
    pHost->devNodeFromWhite= 0;
    pHost->HuaRain_class = NULL;

    pHost->DevInfoHead.iCount = 0;
    pHost->DevInfoHead.DevInfoRoot_rb = RB_ROOT;

    pHost->stRmtCmdList.RcvDirectCmdRoot_rb = RB_ROOT;
    pHost->stRmtCmdList.RcvSmplexRmtCmdRoot_rb = RB_ROOT;
    pHost->stRmtCmdList.RcvDuplexRmtCmdRoot_rb = RB_ROOT;
    pHost->stRmtCmdList.SndDuplexRmtAckRoot_rb = RB_ROOT;
    pHost->stRmtCmdList.iErrCount = 0;

    atomic_set(&pHost->rgstCmd.atRgst, 0);
    //init_waitqueue_head(&(ptmpDevNode->rStatq));
    init_waitqueue_head(&(pHost->rgstCmd.rgstq));        /** initial wait queue for regester **/
    memset(&pHost->rgstCmd.Cmd, 0, sizeof(pHost->rgstCmd.Cmd));

    atomic_set(&pHost->stRmtCmdList.atTimerExpire, 0);
    INIT_LIST_HEAD(&(pHost->stRmtCmdList.sndPktfifoHead));
    spin_lock_init(&(pHost->stRmtCmdList.spinlock));

    spin_lock_init(&pHost->stRmtCmdList.sndfifoBaseLock);   //Initial send fifo spinlock 
    spin_lock_init(&pHost->stRmtCmdList.rcvfifoBaseLock);   //Initial recv fifo spinlock 
    spin_lock_init(&pHost->stRmtCmdList.sndfifoLock);       //Initial send fifo spinlock 
    spin_lock_init(&pHost->canLock);       //Initial mcp2510 hardware sync spinlock 

    pHost->stRmtCmdList.pSndBuff = kfifo_alloc((sizeof(struct CANBus_Message) << 6), GFP_KERNEL, &pHost->stRmtCmdList.sndfifoBaseLock);  //环形fifo缓存(send)
    if(pHost->stRmtCmdList.pSndBuff == NULL) 
    {
        printk("[%s-%d]: kfifo_alloc failed !\n", __func__, __LINE__);
        iRet = -ENOMEM;
    }

    pHost->stRmtCmdList.pRcvBuff = kfifo_alloc((sizeof(struct CANBus_Message) << 6), GFP_KERNEL, &pHost->stRmtCmdList.rcvfifoBaseLock);  //环形fifo缓存(receive)
    if(pHost->stRmtCmdList.pRcvBuff == NULL) 
    {
        kfifo_free(pHost->stRmtCmdList.pSndBuff);    /** 释放已申请的空间 **/
        printk("[%s-%d]: kfifo_alloc failed !\n", __func__, __LINE__);
        iRet = -ENOMEM;
    }

    return  iRet;
}

void printCanBusMsg(const char* string1, int lineNo, struct CANBus_Message* pCanBusMsg)
{
    printk("[%s-%d]: Message Length is: 0x%x, \tSID is: 0x%x, EID is: 0x%x, RTR is: 0x%x\n revMessage is: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", 
            string1, lineNo, 
            pCanBusMsg->Length, 
            pCanBusMsg->SID, 
            pCanBusMsg->EID, 
            pCanBusMsg->RTR, 
            pCanBusMsg->Messages[0], 
            pCanBusMsg->Messages[1], 
            pCanBusMsg->Messages[2], 
            pCanBusMsg->Messages[3], 
            pCanBusMsg->Messages[4], 
            pCanBusMsg->Messages[5], 
            pCanBusMsg->Messages[6], 
            pCanBusMsg->Messages[7]);
}

void printCmdA(const char* string, int lineNo, struct DevCmdA_s* pCmdA)
{
#if 0
    printk("[%s-%d]CmdA: PhysAddr: 0x%x,0x%x,0x%x,0x%x Opcode: 0x%x Attr: 0x%x Value: 0x%x CRC: 0x%x \n",
            string, lineNo,
            pCmdA->ucPhysAddr[0],
            pCmdA->ucPhysAddr[1],
            pCmdA->ucPhysAddr[2],
            pCmdA->ucPhysAddr[3],

            pCmdA->ucOpcode,
            pCmdA->ucAttr,
            pCmdA->usValue,
            pCmdA->usCRC
            );
#endif
}

void printIdMsg(const char *string, int lineNo, struct IdentifyMsg_s* pIdMsg)
{
    printk("[%s-%d]DevId: PhysAddr: [0x%x,0x%x,0x%x,0x%x] [VNo: 0x%x PNo 0x%x Type: 0x%x] [YMD: %u-%u-%u] [0x%x.%x.%x.%x.%x.%x.%x.%x]\n",
            string, lineNo,
            pIdMsg->ucPhysAddr[0],
            pIdMsg->ucPhysAddr[1],
            pIdMsg->ucPhysAddr[2],
            pIdMsg->ucPhysAddr[3],

            pIdMsg->usVendorNo,
            pIdMsg->usProductNo,
            pIdMsg->usDevType,

            pIdMsg->usYear,
            pIdMsg->ucMonth,
            pIdMsg->ucDay,

            pIdMsg->other[0],
            pIdMsg->other[1],
            pIdMsg->other[2],
            pIdMsg->other[3],
            pIdMsg->other[4],
            pIdMsg->other[5],
            pIdMsg->other[6],
            pIdMsg->other[7]

            );
}
void printCmdB(const char *string, int lineNo, struct DevCmdB_s* pCmdB)
{
#if 0
    printk("[%s-%d]CmdA: PhysAddr: 0x%x,0x%x,0x%x,0x%x Opcode: 0x%x CRC: 0x%x \n",
            string, lineNo,
            pCmdB->ucPhysAddr[0],
            pCmdB->ucPhysAddr[1],
            pCmdB->ucPhysAddr[2],
            pCmdB->ucPhysAddr[3],
            pCmdB->ucOpcode,

            pCmdB->usCRC
            );
#endif
    printIdMsg(string, lineNo, &(pCmdB->stIdmsg));
}

void printCmdC(const char *string, int lineNo, struct DevCmdC_s* pCmdC)
{
#if 0
    printk("[%s-%d]CmdC: PhysAddr: 0x%x,0x%x,0x%x,0x%x Opcode: 0x%x Query 0x%x CRC: 0x%x \n",
            string, lineNo,
            pCmdC->ucPhysAddr[0],
            pCmdC->ucPhysAddr[1],
            pCmdC->ucPhysAddr[2],
            pCmdC->ucPhysAddr[3],

            pCmdC->ucOpcode,
            pCmdC->ucQuery,
            pCmdC->usCRC
            );
#endif
}

void printSendCmd(const char *string, int lineNo, struct sendCmd_s* pSendCmd)
{
#if 1
    int i;

    printk("[%s-%d]Info: len: %u, uiEID: %u, usSID: %u, ucAdapterNo: %u\n",
            string, lineNo,
            pSendCmd->len,
            pSendCmd->uiEID,
            pSendCmd->usSID,
            pSendCmd->ucAdapterNo
            );

    printk("cmd: ");
    for(i = 0; i < 32; i++)
    {
        printk("%3d", pSendCmd->cmd[i]);
    }
#endif
}
/** 需要用到此表格 **/
struct AdapterNO_s * getAdapterTabAddr(void)
{
    return  AdapterNoTab;
}
EXPORT_SYMBOL(getAdapterTabAddr);

int searchAdapterNo(unsigned short SID, unsigned int EID)
{
    int i;

    for(i = 1; i < ARRAY_SIZE(AdapterNoTab); i++)
    {
        //if((SID == AdapterNoTab[i].usSID) && (EID == AdapterNoTab[i].uiEID)) 
        if(EID == AdapterNoTab[i].uiEID)    /** ignore SID **/
        {
            break;
        }
    }

    return  (i < ARRAY_SIZE(AdapterNoTab)) ? i : 0; 

    //return  0;
}

int searchDevNum(unsigned short vendor, unsigned short product)
{
    int i;
    int j;

    for(i = 0; i < ARRAY_SIZE(DevNumTab); i++)
    {
        for(j = 0; j < ARRAY_SIZE(DevNumTab[i]); j++)
        {
            if((vendor == DevNumTab[i][j].usVendorNo) && (product == DevNumTab[i][j].usProductNo)) 
            {
                break;
            }
        }
    }

    if(j < ARRAY_SIZE(DevNumTab[i]))
    {
        return j;
    }

    return  0;
}

/******************************************************************************** 
 * 
 * compare two address
 *
 * input: addr1, addr2
 * output:
 *
 ********************************************************************************/
static int cmpPhysAddr(void* addr1, void* addr2)
{
    //return  memcmp(addr1, addr2, 4);
    /** 
     * 4byte地址长度已经没有必要。
     * 将高2字节作它用。
     * 当前考虑将第3字节作为接口类型编号
     **/
    return  memcmp(addr1, addr2, 2);
}
/******************************************************************************** 
 * 
 * compare two device address
 *
 * input: dev1, dev2
 * output:
 * return: 
 *
 ********************************************************************************/
#if 0
static int cmpDevAddr(struct canbusDevice_s* dev1, struct canbusDevice_s* dev2)
{
    return  memcmp(dev1->stIdMsg.ucPhysAddr, dev2->stIdMsg.ucPhysAddr, 4);
}
#endif
/********************************************************************************
 * compare seq
 ********************************************************************************/
static int cmpCanseq(void* addr1, void* addr2)
{
    return (int)((*(unsigned char *)addr1) - (*(unsigned char *)addr2));
}
/********************************************************************************
 * Type: Function
 * name: DevNoClr
 * author: zhanglong93sohu.com
 * date: 2012-03-09 10:08:53  
 * input: addr - the addr of device No. table
 *        size - the total number of device type
 * output: void
 * description: Clear all the device number in the table
 * if the DevNo is not 0, then it is device number or it is not device number
 *
 * when arrived packet that type is not 0, then we receive it and stored into buffer or ignore it
 *
 ********************************************************************************/
//static int  allocDevNo(void* addr, char* name)
static void DevNoClr(struct deviceType_s* addr, int size)
{
    int i;

    for(i = 0; i < size; i++)
    {   
        //((struct deviceType_s *)addr)[i].devNO = (dev_t)0;  
        //INIT_LIST_HEAD(&(((struct deviceType_s *)addr)[i].stDevList));

        addr[i].devNO = (dev_t)0;  
        INIT_LIST_HEAD(&(addr[i].stDevList));
    } 
}

/*************************************************************************************
 *
 * Function __mcp2510Pkt_read
 *  
 *  input: intrNum
 *  output: pRcvMsg
 *  return:
 *
 *  descriptor:
 *
 *************************************************************************************/
static int __mcp2510Pkt_read(struct CANBus_Message* pRcvMsg, unsigned char intrNum)
{
    unsigned char revAddr;
    unsigned char uMes;
    //int ret;

    revAddr = (intrNum << 3) + 1;               //得到触发中断的接收缓冲器寄存器的地址

    /** read SID/EID/RTR/Length **/
    MCP2510_CS_L;
    while(!SPI0_READY);
    iowrite8(0x03, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(revAddr, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(0xFF, (void *)SPTDAT0);
    while(!SPI0_READY);
    pRcvMsg->SID = ioread8((void *)SPRDAT0) << 3;
    iowrite8(0xFF, (void *)SPTDAT0);
    while(!SPI0_READY);
    uMes = ioread8((void *)SPRDAT0);
    pRcvMsg->SID |= (uMes >> 5);
    pRcvMsg->EID = ((uMes << 16) & 0x3ffff);
    if(!(uMes & 0x08))
    {
        pRcvMsg->RTR = (uMes >> 4) & 0x1;
    }
    iowrite8(0xFF, (void *)SPTDAT0);
    while(!SPI0_READY);
    pRcvMsg->EID |= (ioread8((void *)SPRDAT0) << 8);
    iowrite8(0xFF, (void *)SPTDAT0);
    while(!SPI0_READY);
    pRcvMsg->EID |= ioread8((void *)SPRDAT0);
    iowrite8(0xFF, (void *)SPTDAT0);
    while(!SPI0_READY);
    revAddr =  ioread8((void *)SPRDAT0);
    if(uMes & 0x08)
    {
        pRcvMsg->RTR = (revAddr >> 6) & 0x1;
    }
    pRcvMsg->Length = revAddr & 0x0F;
    if (pRcvMsg->Length > 8)
    {
        PLOG("[%s-%d]: Message Length is:%u \n", __func__, __LINE__,  pRcvMsg->Length);
        pRcvMsg->Length = 8;
    }
    /** read data **/
    for(uMes = 0; uMes < pRcvMsg->Length; uMes++)
    {
        iowrite8(0xFF, (void *)SPTDAT0);
        while(!SPI0_READY);
        pRcvMsg->Messages[uMes] = ioread8((void *)SPRDAT0);
    }
    while(!SPI0_READY);
    MCP2510_CS_H;
    /** Clear interrupt flag **/
    uMes = (intrNum >> 1) - 5;
    MCP2510_BitModi(0x2C, uMes, 0);

    return 1;
}

/**********************************************************************
 * 外部中断处理。由mcp2510触发
 *
 * 当前只处理接收事件.
 * 1.将收到的数据放入到kfifo(用作mcp2510接口), 再激活其上等待读数据的进程
 * 2.将收到的数据放入到合适的rbtree(各个适配器有各自的rbtree)两个地方。
 *       然后判断rbtree,如果收到一组完整的数据,就置标记,调度tasklet任务(处理此数据)
 **********************************************************************/
static irqreturn_t proc_Interrupt(int irq, void *dev_ID)
{
    unsigned char intrNum;
    unsigned char uMes;
    struct RcvCanFrm_s *pRcvMsg;
    struct rb_root *rcvDataRoot_rb;

    struct host_s *pHost = (struct host_s *)dev_ID;

    //spin_lock(&host.canLock);
    spin_lock(pHost->canLock);
    intrNum = MCP2510_Read(0x0E) & 0x0E;            //只有那些被中断使能的中断才会被反映在ICOD位中。得到触发中断的接收缓冲器寄存器
    if(intrNum)
    {
        pRcvMsg = kzalloc(sizeof(struct RcvCanFrm_s), GFP_ATOMIC);
        __mcp2510Pkt_read(&(pRcvMsg->stRcvCanFrm), intrNum);

        if(!(pRcvMsg->stRcvCanFrm.Messages[0] & 0x80))
        {
            printk("[%s-%d]Warn: ... RcvFrm.Messages[0] = 0x%x received self send data!!! \n", __func__, __LINE__, pRcvMsg->stRcvCanFrm.Messages[0]);
            //spin_unlock(&host.canLock);
            spin_unlock(pHost->canLock);
            return IRQ_RETVAL(IRQ_HANDLED);
        }


        /** 
         * rawDate接口
         * 将收到的canbus数据帧放入接收fifo 
         **/
        //kfifo_put(host.stRmtCmdList.pRcvBuff, (unsigned char *)pRcvMsg, sizeof(struct CANBus_Message));  /** 将数据写入缓冲区 **/
        kfifo_put(pHost->stRmtCmdList.pRcvBuff, (unsigned char *)pRcvMsg, sizeof(struct CANBus_Message));  /** 将数据写入缓冲区 **/
        //if(waitqueue_active(&(host.stRmtCmdList.rawDataq)))
        if(waitqueue_active(&(pHost->stRmtCmdList.rawDataq)))
        {
            //wake_up(&(host.stRmtCmdList.rawDataq));        /** 唤醒信息处理线(进程)程 **/
            wake_up(&(pHost->stRmtCmdList.rawDataq));        /** 唤醒信息处理线(进程)程 **/
            //printk("[%s-%d]Info: wake_up for recv.\n", __func__, __LINE__);
        }

        /** 将收到的数据存入到由适配器指定的rbtree中 **/
#if 1
        //printk("[%s-%d]Debug: ... RcvFrm.Messages[0] = 0x%x, RcvFrm.SID = 0x%03x, RcvFrm.EID = 0x%05x.\n",
        PLOG("[%s-%d]Debug: ... RcvFrm.Messages[0] = 0x%x, RcvFrm.SID = 0x%03x, RcvFrm.EID = 0x%05x.\n",
                __func__, __LINE__, 
                pRcvMsg->stRcvCanFrm.Messages[0],
                pRcvMsg->stRcvCanFrm.SID,
                pRcvMsg->stRcvCanFrm.EID);
#endif
        rcvDataRoot_rb = &(AdapterNoTab[(pRcvMsg->stRcvCanFrm.Messages[0] & 0x1f)].rcvDataRoot_rb);
        RcvCanFrm_insert(rcvDataRoot_rb, pRcvMsg, cmpCanseq);
        if(NULL != RcvCanFrm_complete(rcvDataRoot_rb))
        {
            //RcvCanFrm_print(__func__, __LINE__, rcvDataRoot_rb, printCanBusMsg);
            atomic_set(&(AdapterNoTab[(pRcvMsg->stRcvCanFrm.Messages[0] & 0x7f)].atCompleteNo), 1);

            //if(host.iEnable != 0)
            if(pHost->iEnable != 0)
            {
                //tasklet_schedule(&host.can_tasklet);
                tasklet_schedule(&pHost->can_tasklet);
            }
        }

        //spin_unlock(&host.canLock);
        spin_unlock(&pHost->canLock);
    }
    else
    {
        uMes = MCP2510_Read(0x2C) & 0x1C; //清除发送中断
        MCP2510_BitModi(0x2C, uMes, 0);
        //spin_unlock(&host.canLock);
        spin_unlock(&pHost->canLock);
    }

    return IRQ_RETVAL(IRQ_HANDLED);
}

static void __deconfig_mcp2510(void)
{
    disable_irq(IRQ_EINT21);
    free_irq(IRQ_EINT21, NULL);
}

static int __config_mcp2510(void)
{
    int result;
    u8 tmp;

    //注册接收中断服务程序
    //if(request_irq(IRQ_EINT21, proc_Interrupt, IRQF_SAMPLE_RANDOM | IRQF_TRIGGER_LOW, "MCP2510", NULL))
    if(request_irq(IRQ_EINT21, proc_Interrupt, IRQF_SAMPLE_RANDOM | IRQF_TRIGGER_LOW, "MCP2510", &host))
    {
        result = -EFAULT;
        goto err;
    }

    MCP2510_BitModi(0x0F, 0xE0, 0x80); //设置MCP2510为配置模式

    MCP2510_Write(0x2B, 0x03);      //设置(RX1IE, EX0IE)中断使能寄存器, 使能接收缓冲中断
    /** RXFn**/ 
    for(result = 0x0; result <= 0x0B; result++)
    {
        MCP2510_Write(result, 0x00);
    }
    /** RXFn**/ 
    for(result = 0x10; result <= 0x1B; result++)
    {
        MCP2510_Write(result, 0x00);
    }
    /** RXFn**/ 
    MCP2510_Write(0x01, 0x08);
    MCP2510_Write(0x05, 0x08);
    MCP2510_Write(0x09, 0x08);
    MCP2510_Write(0x11, 0x08);
    MCP2510_Write(0x15, 0x08);
    MCP2510_Write(0x19, 0x08);
    /*** 设置RXMn ***/
    for(result = 0x20; result <= 0x27; result++)
    {
        if((result == 0x21) || (result == 0x25))
        {
            MCP2510_Write(result, 0xE0);   /** SID(2..0), EID(17,16) **/ 
        }
        else if((result == 0x20) || (result == 0x24))
        {
            MCP2510_Write(result, 0x0f);    
        }
        else
        {
            MCP2510_Write(result, 0x0);    
        }
    }


    MCP2510_CS_L;                   //设置接收屏蔽寄存器RXMn都为0
    iowrite8(0x02, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(0x20, (void *)SPTDAT0);
    while(!SPI0_READY);
    for(tmp=0; tmp < 8; tmp++)
    {
        iowrite8(0x0, (void *)SPTDAT0);
        while(!SPI0_READY);
    }
    MCP2510_CS_H;

    //设置接收控制寄存器
    MCP2510_BitModi(TXBnCTRL[0], 0x0B, 0x03); //设置TXB0CTRL -发送缓冲器0有最高优先级
    MCP2510_BitModi(TXBnCTRL[1], 0x0B, 0x02); //设置TXB1CTRL -发送缓冲器1有次高优先级
    MCP2510_BitModi(TXBnCTRL[2], 0x0B, 0x01); //设置TXB2CTRL -发送缓冲器2有低优先级

    MCP2510_BitModi(0x60, 0x64, 0x04); //设置RXB0CTRL -接收缓冲器0控制寄存器－
    //接收符合滤波器条件的所有带扩展标识符或标准标识符的有效报文
    //如果RXB0 满， RXB0 接收到的报文将被滚存至RXB1
    MCP2510_BitModi(0x70, 0x60, 0x00); //设置RXB1CTRL -接收缓冲器 1 控制寄存器－
    //接收符合滤波器条件的所有带扩展标识符或标准标识符的有效报文

    //MCP2510_Write(0x2A, 0x02);    //设置CNF1
    MCP2510_Write(0x2A, 0x03);    //设置CNF1
    MCP2510_Write(0x29, 0x9E);    //设置CNF2
    MCP2510_Write(0x28, 0x03);    //设置CNF3，设置波特率为125Kbps/s(1, 7, 4, 4)

    MCP2510_Write(0x2C, 0x0);      //清空中断标志
    MCP2510_BitModi(0x0F, 0xE0, 0x00); //设置MCP2510为正常模式
    result = 0;
err:

    return  result;
}

#if 0
#endif

/**
 * Note: read operation get data from fifo and store into user buf, not read/write mcp2510
 *
 */

#if 0
#endif

/***********************************************************************
 * 
 * input: SendMsg
 * output:
 * return: 
 *
 * descript:
 *      send canbus frame via spi interface
 *      
 ***********************************************************************/
int __mcp2510Pkt_write(struct CANBus_Message* SendMsg)
{
    int i = 0;
    unsigned char reg;

    if(host.iEnable == 0)   /** disable host. don't send data **/
    {
        printk("[%s-%d]Info: host disable !!!\n", __func__, __LINE__); 
        return SendMsg->Length;
    }

#if 0
    //读三个发送缓冲器寄存器，以确定哪个发送缓冲器可用。
    for(i = 0; i < 3; i++)
    {
        if(!(MCP2510_Read(TXBnCTRL[i]) & 0x08))
            break;
    }
    //三个缓冲器都不可用，返回错误
    if(i == 3)
    {
        //printk("[%s-%d]Warning: All 3 TX-buffer are busy \n", __func__, __LINE__);
        return -EBUSY;
    }
#else
    /*** 只使用一个缓冲器，牺牲性能，保证顺序. 测试比较得出：牺牲的性能微不足道 ***/
    i = 0;
    if((MCP2510_Read(TXBnCTRL[i]) & 0x08))
    {
        return -EBUSY;
    }
#endif
    
    PLOG("[%s-%d]: Message Length: 0x%x,\tSID: 0x%x, EID: 0x%x, RTR: 0x%x\n sndMessage: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", 
            __func__, __LINE__, 
            SendMsg->Length, 
            SendMsg->SID, 
            SendMsg->EID, 
            SendMsg->RTR, 
            SendMsg->Messages[0], 
            SendMsg->Messages[1], 
            SendMsg->Messages[2], 
            SendMsg->Messages[3], 
            SendMsg->Messages[4], 
            SendMsg->Messages[5], 
            SendMsg->Messages[6], 
            SendMsg->Messages[7]);
    /** Start send process **/
    MCP2510_CS_L;
    iowrite8(0x02, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(TXBnCTRL[i] + 1, (void *)SPTDAT0);
    while(!SPI0_READY);
    reg = SendMsg->SID >> 3;
    iowrite8(reg, (void *)SPTDAT0);
    while(!SPI0_READY);
    reg = (unsigned char)(SendMsg->SID << 5);
    reg =reg |(SendMsg->EID >> 16);
    //if(SendMsg->EID)
    if(1)
    {
        reg |= (1 << 3);
    }
    iowrite8(reg, (void *)SPTDAT0);
    while(!SPI0_READY);
    reg = (unsigned char)(SendMsg->EID >> 8);
    iowrite8(reg, (void *)SPTDAT0);
    while(!SPI0_READY);
    reg = (unsigned char)SendMsg->EID;
    iowrite8(reg, (void *)SPTDAT0);
    while(!SPI0_READY);
    if(SendMsg->Length > 8)
    {
        printk("[%s-%d]: the length of data is %d grate than 8 !!! \n", __func__, __LINE__, SendMsg->Length);
        //SendMsg->Length = 8;
    }
    iowrite8(SendMsg->Length | (SendMsg->RTR << 6), (void *)SPTDAT0);

    for(reg = 0; reg < SendMsg->Length; reg++)
    {
        while(!SPI0_READY);
        iowrite8(SendMsg->Messages[reg], (void *)SPTDAT0);
    }
    while(!SPI0_READY);
    //printk("[%s-%d]: %d bytes date had been sent ! \n", __func__, __LINE__, SendMsg->Length);
    MCP2510_CS_H;

    MCP2510_BitModi(TXBnCTRL[i], 0x08, 0x08);

    return SendMsg->Length;

}

/** 
 * the only interface for data into kfifo
 *
 *
 * 将数据放入kfifo. 当前在进程上下文使用之. 当前从kfifo中取数据也是在进程上下文，用信号量似乎更合适。
 **/
ssize_t mcp2510Pkt_writeCmdX(const struct sendCmd_s *data, const int interval)
{
    int i, j;

    static unsigned char ucCanseq = 0;      /** 静态变量, canbus上的流水帧号，每发一个帧加1, 每新起一个数据包加3 **/
    struct CANBus_Message *pSndMsg;
    int loop;
    unsigned long flags;

    pSndMsg = kzalloc(sizeof(struct CANBus_Message), GFP_ATOMIC);
    if(pSndMsg == NULL)
    {
        printk("[%s-%d]Warning:  kzalloc() failed !\n", __func__, __LINE__);
        return  0;
    }

    spin_lock_irqsave(&host.stRmtCmdList.sndfifoLock, flags);

    loop = ((data->len + 3) / 4);

    if(ucCanseq + loop >= 255)
    {
        ucCanseq = 0;
    }
    else
    {
        ucCanseq += 2;
    }
    pSndMsg->EID = data->usSID;
    pSndMsg->SID = data->uiEID;

    pSndMsg->RTR = 0;
    //pSndMsg->Length = 8;

    pSndMsg->Messages[0] = data->ucAdapterNo;
    pSndMsg->Messages[1] = data->len;
    pSndMsg->Messages[2] = ucCanseq;
    pSndMsg->Messages[3] = 0;

/** CmdD(physAddr(4) + (opcode)1 + (Attr)1 + (value)2 + pairAddr(4) + (crc)1) length is 13 **/

    for(i = 0; i < loop; i++)
    {
        for(j = 0; j < 4; j++)
        {
            pSndMsg->Messages[4 + j] = data->cmd[j + (i << 2)];
        }

        if(((i + 1) << 2) > data->len)  /** the last packet ? **/
        {
            pSndMsg->Length = (4 + (data->len & 0x03));
        }
        else
        {
            pSndMsg->Length = 8;
        }

        kfifo_put(host.stRmtCmdList.pSndBuff, (unsigned char *)pSndMsg, sizeof(struct CANBus_Message));  /** 将数据写入缓冲区 **/

        ucCanseq += 1;
        pSndMsg->Messages[2] = ucCanseq;    /** 暂不考虑连续发送超过16个包的场景 **/
        pSndMsg->Messages[3] += 1;
    }
    spin_unlock_irqrestore(&host.stRmtCmdList.sndfifoLock, flags);

    if(waitqueue_active(&(host.stRmtCmdList.rmtCmdq)))
    {
        wake_up(&(host.stRmtCmdList.rmtCmdq));        /** 唤醒信息处理线程 **/
        //printk("[%s-%d]Info: wake_up for send cmdX.\n", __func__, __LINE__);
    }

    kfree(pSndMsg);

    host.can_timer.data = interval;
    mod_timer(&host.can_timer, (jiffies + (host.can_timer.data * HZ)));

    return  2;
}

EXPORT_SYMBOL(mcp2510Pkt_writeCmdX);

/**
 * mcp2510Pkt_write() 正常的设备控制数据的传输
 *
 * note: 只有在初始轮询完成后才能正常运作 
 *    并且不应包括功能
 *  1、初始轮询
 *  2、空闲状态时的轮询
 *  3、加入、剔出网络的控制。
 *
 * NOTE : 此函数的功能趋于弱化，最终成为非导出的函数, 或删除
 **/
//ssize_t mcp2510Pkt_write(unsigned char cmd, const unsigned short *__buf, size_t size, void* DevInfo)
ssize_t mcp2510Pkt_write(unsigned char cmd, const unsigned char *__buf, size_t size, void* DevInfo)
{
#if 1
    /** 当前周期归零 **/
    int iRet = 0;
    struct sendCmd_s *pSendCmd;

    if(host.ifirstPoll == 0)    /** 接入网络后的第一次轮询还没完成，不能进行数据发送 **/
    {
        return  2;
    }

    pSendCmd = kzalloc(sizeof(struct sendCmd_s), GFP_KERNEL);
    if(pSendCmd == NULL)
    {
        printk("[%s-%d]Warning:  kzalloc() failed !\n", __func__, __LINE__);
        goto err0;
    }

    if(MIsDUPRMTCmd(cmd))   /** 0x63 **/
    {
        struct canbusDevice_s *PairInfo;

        PairInfo  = DevNode_search(&(host.DevInfoHead.DevInfoRoot_rb), ((struct canbusDevice_s *)DevInfo)->ucPairAddr, cmpPhysAddr); 
        if(PairInfo == NULL)
        {
            printk("[%s-%d]Warning: Can't find the control node[0x%x, 0x%x, 0x%x, 0x%x]!!!\n", 
                    __func__, __LINE__,
                    ((struct canbusDevice_s *)DevInfo)->ucPairAddr[0],
                    ((struct canbusDevice_s *)DevInfo)->ucPairAddr[1],
                    ((struct canbusDevice_s *)DevInfo)->ucPairAddr[2],
                    ((struct canbusDevice_s *)DevInfo)->ucPairAddr[3]
                    );
            goto err1;
        }

        /** 0..3 dest address **/
        for(iRet = 0; iRet < 4; iRet++)
        {
            //pSendCmd->cmd[iRet] = ((struct canbusDevice_s *)DevInfo)->ucPairAddr[iRet];
            pSendCmd->cmd[iRet] = PairInfo->stIdMsg.ucPhysAddr[iRet];
        }
        /** 4..7 src address **/
        for(iRet = 0; iRet < 4; iRet++)
        {
            //pSendCmd->cmd[iRet] = ((struct canbusDevice_s *)DevInfo)->ucPairAddr[iRet];
            pSendCmd->cmd[iRet + 4] = ((struct canbusDevice_s *)DevInfo)->stIdMsg.ucPhysAddr[iRet];
        }
        /** 8 version 0x0 **/
        pSendCmd->cmd[8] = 0x0;
        /** 9 reserved 0x0 **/
        pSendCmd->cmd[9] = 0x0;
        /** 10 opCode 0x63 **/
        pSendCmd->cmd[10] = cmd;
        /** 11 attr **/
        pSendCmd->cmd[11] = ((struct canbusDevice_s *)DevInfo)->ucCmdAttr;
        /** 12 value7..0 **/
        pSendCmd->cmd[12] = ((struct canbusDevice_s *)DevInfo)->usAttr[pSendCmd->cmd[11] & 0x03] & 0xff;
        /** 13 value15..8 **/
        pSendCmd->cmd[13] = (((struct canbusDevice_s *)DevInfo)->usAttr[pSendCmd->cmd[11] & 0x03] >> 8) & 0xff;
        /** 14 reserved **/
        pSendCmd->cmd[14] = 0x0;
        /** 15 CRC **/
        pSendCmd->cmd[15] = makeCrc8(0, pSendCmd->cmd, 15);

        /** 13,14 SID, EID **/
        pSendCmd->ucAdapterNo = PairInfo->ucAdapterNo;
        pSendCmd->usSID = AdapterNoTab[pSendCmd->ucAdapterNo].usSID;
        pSendCmd->uiEID = AdapterNoTab[pSendCmd->ucAdapterNo].uiEID;
        pSendCmd->len = 16;

        iRet = mcp2510Pkt_writeCmdX(pSendCmd, host.iTimerPollDelay); 
        kfree(pSendCmd);

        return iRet;
    }
    if(MBaseCMD(cmd))   /** /0x41/0x42/0x43 **/
    {
        /** 0..3 dest address **/
        for(iRet = 0; iRet < 4; iRet++)
        {
            pSendCmd->cmd[iRet] = ((struct canbusDevice_s *)DevInfo)->stIdMsg.ucPhysAddr[iRet];
        }
        /** 4..7 dest address **/
        for(iRet = 0; iRet < 4; iRet++)
        {
            pSendCmd->cmd[iRet + 4] = 0x0;
        }
        /** 8 version 0x0 **/
        pSendCmd->cmd[8] = 0x0;
        /** 9 reserved 0x0 **/
        pSendCmd->cmd[9] = 0x0;
        /** 10 opCode 0x42 **/
        pSendCmd->cmd[10] = cmd;
        /** 11 attr **/
        pSendCmd->cmd[11] = 0x01;
        /** 12 value7..0 **/
        pSendCmd->cmd[12] = (*__buf) & 0xff;
        /** 13 value15..8 **/
        pSendCmd->cmd[13] = ((*__buf) >> 8) & 0xff;
        /** 14 reserved **/
        pSendCmd->cmd[14] = 0;
        /** 15 CRC8 **/
        pSendCmd->cmd[15] = makeCrc8(0, pSendCmd->cmd, 15);
        /** 13,14 SID, EID **/
        pSendCmd->ucAdapterNo = ((struct canbusDevice_s *)DevInfo)->ucAdapterNo;
        pSendCmd->usSID = AdapterNoTab[pSendCmd->ucAdapterNo].usSID;
        pSendCmd->uiEID = AdapterNoTab[pSendCmd->ucAdapterNo].uiEID;
        pSendCmd->len = 16;

        iRet = mcp2510Pkt_writeCmdX(pSendCmd, host.iTimerPollDelay); 
        kfree(pSendCmd);

        return iRet;
    }
    else if(MIsRawCmd(cmd))   /** 0x44 **/
    {
        pSendCmd->cmd[15] = makeCrc8(0, pSendCmd->cmd, 15);

        pSendCmd->ucAdapterNo = ((struct canbusDevice_s *)DevInfo)->ucAdapterNo;
        pSendCmd->usSID = AdapterNoTab[pSendCmd->ucAdapterNo].usSID;
        pSendCmd->uiEID = AdapterNoTab[pSendCmd->ucAdapterNo].uiEID;
        pSendCmd->len = 16;

        iRet = mcp2510Pkt_writeCmdX(pSendCmd, host.iTimerPollDelay); 
        kfree(pSendCmd);

        return iRet;
    }
    else
    {
        printk("[%s-%d]Warning:  unrecognized command !\n", __func__, __LINE__);
    }

err1:
    kfree(pSendCmd);
err0:
#endif
    /** Can't goto here **/
    return  0;
}
EXPORT_SYMBOL(mcp2510Pkt_write);


static ssize_t mcp2510Pkt_poll(void)
{
#if 1
    int iRet;
    int iLoopCnt;
    static int iCnt = 0;
    struct sendCmd_s *pSendCmd;

    pSendCmd = kzalloc(sizeof(struct sendCmd_s), /** GFP_KERNEL **/ GFP_ATOMIC);
    if(pSendCmd == NULL)
    {
        printk("[%s-%d]Warning:  kzalloc() failed !\n", __func__, __LINE__);
        return -ENOMEM;
    }

    iLoopCnt = 0;   /** 防死循环. 循环次数不超过2倍的数组大小 **/
    if(host.ifirstPoll == 0)    /** 初始轮询没有完成(要求每一个转换器都要查一遍) **/
    {
        do  /** Note: if no adapter detected, then find one with specified SID+EID**/
        {
            iCnt++;
            if(iCnt >= ARRAY_SIZE(AdapterNoTab))
            {
                iCnt = 1;
                iLoopCnt += 1;
                host.ifirstPoll = 1;
            }
        } while(((AdapterNoTab[iCnt].usSID == 0) && (AdapterNoTab[iCnt].uiEID == 0))
                && (iLoopCnt < 2));
    }
    else        /** 初始轮询已经完成，只查那些收到过数据的转换器 **/
    {
        do  /** Note: if no adapter detected, then find one with specified SID+EID**/
        {
            iCnt++;
            if(iCnt >= ARRAY_SIZE(AdapterNoTab))
            {
                iCnt = 1;
                iLoopCnt += 1;
            }
        } while((((AdapterNoTab[iCnt].usSID == 0) && (AdapterNoTab[iCnt].uiEID == 0)) || (AdapterNoTab[iCnt].iReceivedData == 0))
                && (iLoopCnt < 2));
    }
    /** 0..3 dest address **/
    for(iRet = 0; iRet < 4; iRet++)
    {
        pSendCmd->cmd[iRet] = 0xff;
    }
    /** 4..7 dest address **/
    for(iRet = 4; iRet < 8; iRet++)
    {
        pSendCmd->cmd[iRet] = 0x0;
    }
    /** 8 version 0x0 **/
    pSendCmd->cmd[8] = 0x0;
    /** 9 opCode 0x0 **/
    pSendCmd->cmd[9] = 0x0;
    /** 10 opCode 0x11 **/
    pSendCmd->cmd[10] = 0x11;
    /** 11 attr **/
    pSendCmd->cmd[11] = 0x01;
    /** 12,13,14 reserved **/
    for(iRet = 0; iRet < 3; iRet++)
    {
        pSendCmd->cmd[iRet + 12] = 0x0;
    }
    /** 15 CRC **/
    pSendCmd->cmd[15] = makeCrc8(0, pSendCmd->cmd, 15);

    /** 13,14 SID, EID **/
    pSendCmd->ucAdapterNo = iCnt;
    pSendCmd->usSID = AdapterNoTab[pSendCmd->ucAdapterNo].usSID;
    pSendCmd->uiEID = AdapterNoTab[pSendCmd->ucAdapterNo].uiEID;
    pSendCmd->len = 16;

    iRet = (iLoopCnt > 0)? host.iTimerPollDelay : host.iTimerPollPeriod;        /** 轮询过一遍了 **/
    iRet = (host.ifirstPoll == 0)? AdapterNoTab[pSendCmd->ucAdapterNo].iPollInterval : iRet;
    mcp2510Pkt_writeCmdX(pSendCmd, iRet); 

    kfree(pSendCmd);

#endif

    return  0;
}

/**
 * 不在白名单中, 发命令剔出网络
 **/
static ssize_t mcp2510Pkt_outWhite(unsigned char *addr, int adapterNo)
{
#if 1
    int iRet;
    struct sendCmd_s *pSendCmd;

    pSendCmd = kzalloc(sizeof(struct sendCmd_s), /** GFP_KERNEL **/GFP_ATOMIC);
    if(pSendCmd == NULL)
    {
        printk("[%s-%d]Warning:  kzalloc() failed !\n", __func__, __LINE__);
        return -ENOMEM;
    }

    /** 0..3 dest address **/
    for(iRet = 0; iRet < 4; iRet++)
    {
        pSendCmd->cmd[iRet] = addr[iRet];
    }
    /** 4..7 src address **/
    for(iRet = 0; iRet < 4; iRet++)
    {
        pSendCmd->cmd[iRet + 4] = 0x0;
    }
    /** 8 version 0x0 **/
    pSendCmd->cmd[8] = 0x0;
    /** 9 reserved 0x0 **/
    pSendCmd->cmd[9] = 0x0;

    /** 10 opCode 0x13 **/
    pSendCmd->cmd[10] = 0x13;
    /** 11 attr **/
    pSendCmd->cmd[11] = 0x01;
    /** 12,13,14 reserved **/
    for(iRet = 0; iRet < 4; iRet++)
    {
        pSendCmd->cmd[iRet + 12] = 0x0;
    }
    /** 7 CRC **/
    pSendCmd->cmd[15] = makeCrc8(0, pSendCmd->cmd, 15);
    /** 13,14 SID, EID **/
    pSendCmd->ucAdapterNo = adapterNo;
    pSendCmd->usSID = AdapterNoTab[pSendCmd->ucAdapterNo].usSID;
    pSendCmd->uiEID = AdapterNoTab[pSendCmd->ucAdapterNo].uiEID;
    pSendCmd->len = 16;

    iRet = mcp2510Pkt_writeCmdX(pSendCmd, ((host.ifirstPoll == 0)? AdapterNoTab[pSendCmd->ucAdapterNo].iPollInterval : host.iTimerPollPeriod)); 

    kfree(pSendCmd);

    return  0;
#endif
}
/*********************************************************************************************
 *
 * canwork_handle
 * Description: make device node(/dev/node) with the driver 
 *
 * use spin_lock_bh()/spin_unlock_bh() to make sure that tasklet can't operate the stNoNodeList
 *
 * 实现时发现：spin_lock_bh()/spin_unlock_bh()会有严重故障
 *********************************************************************************************/
void canwork_handle(struct work_struct* arg)
{
    struct canbusDevice_s *ptmpDataNode;
    struct canbusDevice_s *ptmpDataNodepos;
    
    if(arg != &host.can_work)
    {
        printk("[%s-%d]Warning:  input arg is not wanted !\n", __func__, __LINE__);
        return;
    }

    //spin_lock_bh(&(stNoNodeList.spinlock));
    spin_lock_irq(&(stNoNodeList.spinlock));
    //spin_lock_irqsave(&(stNoNodeList.spinlock), flags);
    list_for_each_entry_safe(ptmpDataNode, ptmpDataNodepos, &(stNoNodeList.stNoNode), stNoNode)
    {
        if(0 == ptmpDataNode->no)
        {
            printk("[%s-%d]Info: major DevNO is zero !!! \n", __func__, __LINE__);
            continue;
        }
     
        cdev_init(&(ptmpDataNode->dev), devTypeTab[ptmpDataNode->stIdMsg.usDevType].pstFops);
        cdev_add(&(ptmpDataNode->dev), ptmpDataNode->no, 1);
#if 1
        ptmpDataNode->pDevice = device_create(host.HuaRain_class, 
                NULL, 
                ptmpDataNode->no, 
                ptmpDataNode, 
                "%s-%d", 
                ptmpDataNode->ucDispName,                //devTypeTab[ptmpDataNode->stIdMsg.usDevType].pDesc,
                MINOR(ptmpDataNode->no)
                );
#endif
        list_del(&(ptmpDataNode->stNoNode));
    }
    //spin_unlock_bh(&(stNoNodeList.spinlock));
    spin_unlock_irq(&(stNoNodeList.spinlock));
    //spin_unlock_irqrestore(&(stNoNodeList.spinlock), flags);
    //printk("[%s-%d]Info[%d]: work handler DOING SOMETHING(Create device NODE )!\n", __func__, __LINE__, iCnt);
}

/************************************************************************************************************
 * pieceCmdFrame
 *
 * input:   node_rb
 * 
 * output:  revCmd
 *
 * return:  NO. of packet seq
 *
 * descript: stored data at the proper area with specific frame seq
 *
 ************************************************************************************************************/
static int pieceCmdFrame(struct rb_node *node_rb, recvCmd_t * rcvCmd)
{
    int i = 0;
    int iPktSeq = 0;
    struct CANBus_Message *pCanMsg;      //make sure only bottom half use it

    if(NULL != node_rb) /** first packet **/
    {
        pCanMsg = &((rb_entry(node_rb, struct RcvCanFrm_s, node_rb))->stRcvCanFrm);
        rcvCmd->len = pCanMsg->Messages[1];
        rcvCmd->uiEID = pCanMsg->EID;
        rcvCmd->usSID = pCanMsg->SID;
        rcvCmd->ucAdapterNo = (rcvCmd->uiEID & 0x3f);
    }

    if(pCanMsg->Messages[3] != 0)    /** 第一个包的序号不是0, 认为出错 **/
    {
        return  0;
    }

    while((NULL != node_rb) && (iPktSeq << 2 <= MMaxRcvLen))
    {
        pCanMsg = &((rb_entry(node_rb, struct RcvCanFrm_s, node_rb))->stRcvCanFrm);
        for(i = 0; i < 4; i++)
        {
            rcvCmd->cmd[(iPktSeq << 2) + i] = pCanMsg->Messages[4 + i];
        }
        node_rb = rb_next(node_rb);
        iPktSeq++ ;
    }

    return  iPktSeq;
}

/*********************************************************************************************
 *
 * description:
 * condition:   1.local register operation
 *              2.received a packet complete
 * process:
 *  1.if find a local register operation(translate to 0x12 command and store it into ...), then wake up the workqueue
 *  2.else research received data order by AdapterNoTab[i]
 *  
 *
 *********************************************************************************************/
void tasklet_handle(unsigned long arg)
{
    struct rb_root *rcvDataRoot_rb = NULL;
    struct whiteListNode_s *whiteListNode = NULL;
    struct rb_node *node_rb;
    struct host_s *pHost;
    //struct RmtCmdANode_s  *ptmpCmdANode;
    //unsigned char ucCrc8;
    static unsigned short usErrNum = 0;
    //unsigned short usTmp;
    int ret;
    int i;
    
    pHost = (struct host_s *)arg;

    /** raw operation:  **/
    if(unlikely(1 == atomic_read(&(pHost->rgstCmd.atRgst))))    /** 有本地操作注册信息 **/
    {
        atomic_set(&(pHost->rgstCmd.atRgst), 0);                /** 同步之用, 只在tasklet中清零 **/
        memcpy(&bh_RcvCmd, &pHost->rgstCmd, sizeof(bh_RcvCmd));

        if(waitqueue_active(&(pHost->rgstCmd.rgstq)))
        {
            wake_up(&(pHost->rgstCmd.rgstq));        /** 唤醒读信息的等待队列 **/
        }
    }
    else
    {
        /** get proper(received cmd) AdapterNO.**/
        for(i = 1; i < ARRAY_SIZE(AdapterNoTab); i++)
        {
            if(0 != atomic_read(&(AdapterNoTab[i].atCompleteNo)))
            {
                AdapterNoTab[i].iReceivedData |= 1;         /** 此转换器上收到数据 **/
                break;
            }
        }
        if(i >= ARRAY_SIZE(AdapterNoTab))
        {
            printk("[%s-%d]Warning: No Adapter data received !!! \n", __func__, __LINE__);
            return;
        }

        atomic_set(&(AdapterNoTab[i].atCompleteNo), 0);     /** clean complete flag. the specific adapter ...**/

        /** get rbtree root with specified AdapterNO **/
        rcvDataRoot_rb = &(AdapterNoTab[i].rcvDataRoot_rb);
        if(NULL == (node_rb = RcvCanFrm_complete(rcvDataRoot_rb)))
        {
            printk("[%s-%d]Warning: No series data in RCV rbtree!!! \n", __func__, __LINE__);
            return;
        }

        /** 从缓冲区中读出数据(the first frame, byte0..3) **/
        ret = pieceCmdFrame(node_rb, &bh_RcvCmd);
        RcvCanFrm_reset(rcvDataRoot_rb);        /** destory the rbtree **/
        if(ret == 0)
        {
            printk("[%s-%d]Warning: pieceCmdFrame() error !!! \n", __func__, __LINE__);
        }
    }

    if(makeCrc8(0, bh_RcvCmd.cmd, bh_RcvCmd.len) != 0)
    {
        printk("[%s-%d]Warning: RCV data CRC error !!! \n", __func__, __LINE__);
        /**
         *  do something here 
         *  because data error, you can determed giveup the data or not
         **/
    }

    if(host.iEnable == 0)   /** disable host. don't process the data **/
    {
        return;
    }

#define MOPCODE  stTmp.stDevCmd.stDevCmdA.ucOpcode    
    stTmp.stDevCmd.stDevCmdA.ucOpcode = bh_RcvCmd.cmd[10];
    stTmp.ucAdapterNo = bh_RcvCmd.ucAdapterNo;
    stTmp.usLength = bh_RcvCmd.len;

    stTmp.stDevCmd.stDevCmdA.ucDestAddr[0] = bh_RcvCmd.cmd[0];
    stTmp.stDevCmd.stDevCmdA.ucDestAddr[1] = bh_RcvCmd.cmd[1];
    stTmp.stDevCmd.stDevCmdA.ucDestAddr[2] = bh_RcvCmd.cmd[2];
    stTmp.stDevCmd.stDevCmdA.ucDestAddr[3] = bh_RcvCmd.cmd[3];

    stTmp.stDevCmd.stDevCmdA.ucSrcAddr[0] = bh_RcvCmd.cmd[4];
    stTmp.stDevCmd.stDevCmdA.ucSrcAddr[1] = bh_RcvCmd.cmd[5];
    stTmp.stDevCmd.stDevCmdA.ucSrcAddr[2] = bh_RcvCmd.cmd[6];
    stTmp.stDevCmd.stDevCmdA.ucSrcAddr[3] = bh_RcvCmd.cmd[7];

/** Weather it is static identify message **/
    switch(MOPCODE)
    {
        case CIdMESSAGE:    //0x22静态信息注册
            /** Save the PhysAddr **/
            for(i = 0; i < 4; i++)
            {
                stTmp.stDevCmd.stDevCmdB.stIdmsg.ucPhysAddr[i] = stTmp.stDevCmd.stDevCmdB.ucSrcAddr[i];
            }

            /** 以物理地址为依据，在红黑树中查找(创建)相应的设备的数据区地址 **/
            ptmpDevNode = DevNode_search(&(host.DevInfoHead.DevInfoRoot_rb), stTmp.stDevCmd.stDevCmdB.ucSrcAddr, cmpPhysAddr); 
            if(NULL == ptmpDevNode)
            {
                /**************************************************************** 
                 * 没找到, 是第一次收到此设备的数据, 并且是静态信息。立即创建一个 
                 *
                 * 创建并初始化后，激活work queue以建立应用层接口使用的设备节点
                 *
                 * 几个条件：1、附属模块已插入，否则不能分配主设备号
                 *           2、功能使能
                 ****************************************************************/

                ptmpDevNode = kzalloc(sizeof(struct canbusDevice_s), GFP_ATOMIC);

                memcpy(&(ptmpDevNode->stIdMsg), &(stTmp.stDevCmd.stDevCmdB.stIdmsg), sizeof(struct IdentifyMsg_s));
                DevNode_insert(&host, ptmpDevNode, cmpPhysAddr);

                ptmpDevNode->ucIdValid = 1;                 /** 此设备有效 **/
                ptmpDevNode->ucStatValid = 0;               /** 此设备的动态信息无效 **/
                ptmpDevNode->ulJiffies = jiffies;
                init_waitqueue_head(&(ptmpDevNode->rStatq));        /** initial wait queue for read stat **/
                ptmpDevNode->ucAdapterNo = searchAdapterNo(0, (bh_RcvCmd.uiEID & 0x7f)); /** search AdapterNo with specified EID **/
                if(ptmpDevNode->ucAdapterNo == 0)   /**  **/
                {
                    printk("[%s-%d]searchAdapterNo Warning: AdapterNo(SID, EID) = 0(0x%x, 0x%x)\n", __func__, __LINE__, pBhMsg->SID, pBhMsg->EID);
                    return;
                }

                /** 设备类型以设备发过来的为主，如果没有，就用厂商号+产品号查设备类型  **/
                if(0 == ptmpDevNode->stIdMsg.usDevType)
                {
                    /** 通过厂商号+产品号查得设备的类型号 **/
                    ptmpDevNode->stIdMsg.usDevType = searchDevNum(ptmpDevNode->stIdMsg.usVendorNo, ptmpDevNode->stIdMsg.usProductNo);
                }
                if(0 == ptmpDevNode->stIdMsg.usDevType)
                {
                    ptmpDevNode->stIdMsg.usDevType = 1;
                }
                ret = ptmpDevNode->stIdMsg.usDevType; 

                if(0 == devTypeTab[ret].devNO)  /** 以此类型设备的设备号作依据, 判断设备驱动是否已经加载 **/
                {   /** 还没加载驱动 **/
                    devTypeTab[ret].uiMaxMinorNo = 0;
                    ptmpDevNode->no = 0;
                }
                else
                {
                    devTypeTab[ret].uiMaxMinorNo++;
                    ptmpDevNode->no = MKDEV(MAJOR(devTypeTab[ret].devNO), devTypeTab[ret].uiMaxMinorNo);
                }

                list_add(&(ptmpDevNode->stDevList), &(devTypeTab[ret].stDevList));       /** 将数据区挂入相应类型的链表中 **/
                //spin_lock_bh(&(stNoNodeList.spinlock));
                list_add(&(ptmpDevNode->stNoNode), &(stNoNodeList.stNoNode));            /** 将数据区挂入尚未建立应用层设备节点的链表中 **/
                //spin_unlock_bh(&(stNoNodeList.spinlock));
                //schedule_work(&can_work);       /** creat character device node **/
                schedule_work(&host.can_work);       /** creat character device node **/
            }
            else
            {
                //printk("[%s-%d]Info: data is created and found ok!\n", __func__, __LINE__);
                /** check device message has been updated or not **/
                if(memcmp(&(ptmpDevNode->stIdMsg), &(stTmp.stDevCmd.stDevCmdB.stIdmsg), sizeof(struct IdentifyMsg_s)))
                {
                    printk("[%s-%d]Info: ID data is changed!\n", __func__, __LINE__);
                }
            }
            break;
        case CCANSWER:  //0x12
            ptmpDevNode = DevNode_search(&(host.DevInfoHead.DevInfoRoot_rb), stTmp.stDevCmd.stDevCmdC.ucSrcAddr, cmpPhysAddr); 
            if(NULL == ptmpDevNode)
            {
                /**************************************************************** 
                 * 没找到, 但收到此设备的数据, 不是静态信息。立即创建一个数据区 
                 *
                 * 创建并初始化后，激活work queue以建立应用层接口使用的设备节点
                 *
                 * 几个条件：1、附属模块已插入，否则不能分配主设备号
                 *           2、功能使能
                 ****************************************************************/
                if(1 == whiteListHead.iEnable)  /** whiteList enable or not **/
                {
                    whiteListNode = whiteNameNode_search(&(whiteListHead.whiteListRoot_rb), stTmp.stDevCmd.stDevCmdC.ucSrcAddr, cmpPhysAddr);
                    if(NULL == whiteListNode)  /** whiteNameList forbidden **/ 
                    {
                        printk("[%s-%d]Info: whiteNameList forbiden! addr: [0x%02x 0x%02x 0x%02x 0x%02x]\n", 
                                __func__, __LINE__,
                                stTmp.stDevCmd.stDevCmdC.ucSrcAddr[0],
                                stTmp.stDevCmd.stDevCmdC.ucSrcAddr[1],
                                stTmp.stDevCmd.stDevCmdC.ucSrcAddr[2],
                                stTmp.stDevCmd.stDevCmdC.ucSrcAddr[3]
                              );
                        mcp2510Pkt_outWhite(stTmp.stDevCmd.stDevCmdC.ucSrcAddr, stTmp.ucAdapterNo & 0x1f);

                        return;
                    }
                }
                ptmpDevNode = kzalloc(sizeof(struct canbusDevice_s), GFP_ATOMIC);
                if(NULL == ptmpDevNode)
                {
                    printk("[%s-%d]Warning: kzalloc() failed !\n", __func__, __LINE__);
                    return;
                }
                memcpy((ptmpDevNode->stIdMsg.ucPhysAddr), 
                        (stTmp.stDevCmd.stDevCmdC.ucSrcAddr),
                        sizeof(stTmp.stDevCmd.stDevCmdC.ucSrcAddr));
                if(NULL != whiteListNode)
                {
                    memcpy((ptmpDevNode->ucDispName), 
                            (whiteListNode->ucDispName),
                            sizeof(ptmpDevNode->ucDispName));
                    ptmpDevNode->ucDispName[sizeof(ptmpDevNode->ucDispName) - 1] = '\0';    /** no matter string long or short, the last character is '\0' **/
                }

                DevNode_insert(&host, ptmpDevNode, cmpPhysAddr);

                ptmpDevNode->ucIdValid = 1;                 /** 此设备有效 **/
                ptmpDevNode->ucStatValid = 0;               /** 此设备的动态信息无效 **/
                ptmpDevNode->ulJiffies = jiffies;
                init_waitqueue_head(&(ptmpDevNode->rStatq));        /** initial wait queue for read stat **/
                //ptmpDevNode->ucAdapterNo = searchAdapterNo(0, (pBhMsg->EID & 0x3f)); /** search AdapterNo with specified EID **/
                ptmpDevNode->ucAdapterNo = searchAdapterNo(0, (bh_RcvCmd.uiEID & 0x3f)); /** search AdapterNo with specified EID **/
                if(ptmpDevNode->ucAdapterNo == 0)   /**  **/
                {
                    printk("[%s-%d]searchAdapterNo Warning: AdapterNo(SID, EID) = 0(0x%x, 0x%x)\n", __func__, __LINE__, pBhMsg->SID, pBhMsg->EID);
                    return;
                }

            #if 0
                printk("[%s-%d]debug: SrcAddr is: [%d, %d, %d, %d]\n", 
                            __func__, __LINE__, 
                            ptmpDevNode->stIdMsg.usDevType,
                            stTmp.stDevCmd.stDevCmdC.ucSrcAddr[0],
                            stTmp.stDevCmd.stDevCmdC.ucSrcAddr[1],
                            stTmp.stDevCmd.stDevCmdC.ucSrcAddr[2],
                            stTmp.stDevCmd.stDevCmdC.ucSrcAddr[3]
                        );
            #endif
                /** 设备类型以设备发过来的为主，如果没有，就用厂商号+产品号查设备类型  **/
                if(0 == ptmpDevNode->stIdMsg.usDevType)
                {
                    /** 通过设备的地址信息得设备的类型号  **/
                    ptmpDevNode->stIdMsg.usDevType = ptmpDevNode->stIdMsg.ucPhysAddr[2];
                }


                if(0 == ptmpDevNode->stIdMsg.usDevType)
                {
                    /** 通过厂商号+产品号查得设备的类型号 **/
                    ptmpDevNode->stIdMsg.usDevType = searchDevNum(ptmpDevNode->stIdMsg.usVendorNo, ptmpDevNode->stIdMsg.usProductNo);

                }
                if(0 == ptmpDevNode->stIdMsg.usDevType)
                {
                    ptmpDevNode->stIdMsg.usDevType = 1;
                }

                /** ...... **/

                ret = ptmpDevNode->stIdMsg.usDevType; 

                if(0 == devTypeTab[ret].devNO)  /** 以此类型设备的设备号作依据, 判断设备驱动是否已经加载 **/
                {   /** 还没加载驱动 **/
                    devTypeTab[ret].uiMaxMinorNo = 0;
                    ptmpDevNode->no = 0;
                }
                else
                {
                    devTypeTab[ret].uiMaxMinorNo++;
                    ptmpDevNode->no = MKDEV(MAJOR(devTypeTab[ret].devNO), devTypeTab[ret].uiMaxMinorNo);
                }

                list_add(&(ptmpDevNode->stDevList), &(devTypeTab[ret].stDevList));       /** 将数据区挂入相应类型的链表中 **/
                //spin_lock_bh(&(stNoNodeList.spinlock));
                list_add(&(ptmpDevNode->stNoNode), &(stNoNodeList.stNoNode));            /** 将数据区挂入尚未建立应用层设备节点的链表中 **/
                //spin_unlock_bh(&(stNoNodeList.spinlock));
                schedule_work(&host.can_work);       /** creat character device node **/
            }
            ptmpDevNode->ucIdValid = 1;
            ptmpDevNode->ulJiffies = jiffies;
            ptmpDevNode->ucAdapterNo = searchAdapterNo(0, (bh_RcvCmd.uiEID & 0x3f));    /** Maybe change adapter search AdapterNo with specified EID **/

            break;
        case CCANSTATUS:    /** process 0x43. CSETSTATUS(0x42)/CGETSTATUS(0x41) ignored **/
            {
                struct RmtCmdNode_s *pRmtCmdNode;
                ptmpDevNode = DevNode_search(&(host.DevInfoHead.DevInfoRoot_rb), stTmp.stDevCmd.stDevCmdC.ucSrcAddr, cmpPhysAddr); 
                if(NULL == ptmpDevNode)
                {
                    printk("[%s-%d]: Can't find node: Addr[0x%x, 0x%x, 0x%x, 0x%x], but received its command !!!\n",
                            __func__, __LINE__,
                            stTmp.stDevCmd.stDevCmdC.ucSrcAddr[0],
                            stTmp.stDevCmd.stDevCmdC.ucSrcAddr[1],
                            stTmp.stDevCmd.stDevCmdC.ucSrcAddr[2],
                            stTmp.stDevCmd.stDevCmdC.ucSrcAddr[3]
                          );
                    return;
                }

                //stTmp.stDevCmd.stDevCmdA.ucOpcode = pBhMsg->Messages[5];
                stTmp.stDevCmd.stDevCmdA.ucOpcode = bh_RcvCmd.cmd[10];
                //ptmpDevNode->usAttr[(bh_RcvCmd.cmd[11]) & 0x03] = pBhMsg->Messages[6] | (pBhMsg->Messages[7] << 8);
                ptmpDevNode->usAttr[(bh_RcvCmd.cmd[11]) & 0x03] = bh_RcvCmd.cmd[12] | (bh_RcvCmd.cmd[13] << 8);
                ptmpDevNode->ulJiffies = jiffies;
                ptmpDevNode->ucStatValid = 1;           /** 此设备的动态信息有效 **/

                if(waitqueue_active(&(ptmpDevNode->rStatq)))
                {
                    wake_up(&(ptmpDevNode->rStatq));        /** 唤醒读信息的等待队列 **/
                    //printk("[%s-%d]Info: wake_up for local application(0x43 cmd).\n", __func__, __LINE__);
                }

                if(ptmpDevNode->ucCmdWaitAck == 1)      /** 如果正等待双向遥控的应答 **/
                {
                    pRmtCmdNode = rmtCmdNode_search(&host.stRmtCmdList.SndDuplexRmtAckRoot_rb, ptmpDevNode->stIdMsg.ucPhysAddr, cmpPhysAddr);
                    if(pRmtCmdNode == NULL)
                    {
                        pRmtCmdNode = kzalloc(sizeof(struct RmtCmdNode_s), GFP_ATOMIC);   /** 从tasklet(中断bottom half)上下文中分配内存. 从不睡眠 **/
                        if(pRmtCmdNode == NULL)
                        {
                            printk("[%s-%d]Warning: kzalloc failed \n", __func__, __LINE__);
                            return;
                        }
                        pRmtCmdNode->devNode = ptmpDevNode;
                        RmtCmdNode_insert(&host.stRmtCmdList.SndDuplexRmtAckRoot_rb, pRmtCmdNode, cmpPhysAddr);
                    }

                    if(waitqueue_active(&(host.stRmtCmdList.rmtCmdq)))
                    {
                        wake_up(&(host.stRmtCmdList.rmtCmdq));        /** 唤醒信息处理线程 **/
                        //printk("[%s-%d]Info: wake_up for send ack(0x43 cmd).\n", __func__, __LINE__);
                    }
                }
            }

            break;
        case CGETSTATUS:    /** CGETSTATUS(0x41) ignored **/
            //if(!waitqueue_active(&(pHost->stRmtCmdList.rawDataq)))  /** 查看是否有Raw数据读取进程,有的话就没必要打印这些告警信息 **/
            //{
                printk("[%s-%d]Info: Ignore get-status(0x41) command from [0x%02x, 0x%02x, 0x%02x, 0x%02x] to [0x%02x, 0x%02x, 0x%02x, 0x%02x]!!!\n", 
                            __func__, __LINE__,
                            stTmp.stDevCmd.stDevCmdA.ucSrcAddr[0],
                            stTmp.stDevCmd.stDevCmdA.ucSrcAddr[1],
                            stTmp.stDevCmd.stDevCmdA.ucSrcAddr[2],
                            stTmp.stDevCmd.stDevCmdA.ucSrcAddr[3],

                            stTmp.stDevCmd.stDevCmdA.ucDestAddr[0],
                            stTmp.stDevCmd.stDevCmdA.ucDestAddr[1],
                            stTmp.stDevCmd.stDevCmdA.ucDestAddr[2],
                            stTmp.stDevCmd.stDevCmdA.ucDestAddr[3]
                          );
            //}
            break;
        case CSETSTATUS:    /** CSETSTATUS(0x42) ignored **/
            //if(!waitqueue_active(&(pHost->stRmtCmdList.rawDataq)))  /** 查看是否有Raw数据读取进程,有的话就没必要打印这些告警信息 **/
            //{
                printk("[%s-%d]Info: Ignore set-status(0x42) command from [0x%02x, 0x%02x, 0x%02x, 0x%02x] to [0x%02x, 0x%02x, 0x%02x, 0x%02x]!!!\n", 
                            __func__, __LINE__,
                            stTmp.stDevCmd.stDevCmdA.ucSrcAddr[0],
                            stTmp.stDevCmd.stDevCmdA.ucSrcAddr[1],
                            stTmp.stDevCmd.stDevCmdA.ucSrcAddr[2],
                            stTmp.stDevCmd.stDevCmdA.ucSrcAddr[3],

                            stTmp.stDevCmd.stDevCmdA.ucDestAddr[0],
                            stTmp.stDevCmd.stDevCmdA.ucDestAddr[1],
                            stTmp.stDevCmd.stDevCmdA.ucDestAddr[2],
                            stTmp.stDevCmd.stDevCmdA.ucDestAddr[3]
                          );
            //}
            break;
        case   CRMTSETSTATUS:   /** 0x52. ignored 0x51/0x53命令 **/
            {
                struct RmtCmdNode_s *pRmtCmdNode;

                /** find data area and store data **/
                ptmpDevNode = DevNode_search(&(host.DevInfoHead.DevInfoRoot_rb), stTmp.stDevCmd.stDevCmdC.ucDestAddr, cmpPhysAddr); 
                if(NULL == ptmpDevNode)
                {
                    printk("[%s-%d]Info: Can't find node[0x%02x, 0x%02x, 0x%02x, 0x%02x], but received its command !!!\n", 
                            __func__, __LINE__,
                            stTmp.stDevCmd.stDevCmdA.ucDestAddr[0],
                            stTmp.stDevCmd.stDevCmdA.ucDestAddr[1],
                            stTmp.stDevCmd.stDevCmdA.ucDestAddr[2],
                            stTmp.stDevCmd.stDevCmdA.ucDestAddr[3]
                          );
                    return;
                }
                ptmpDevNode->ucCmdOpcode = MOPCODE;
                ptmpDevNode->ucCmdAttr = (bh_RcvCmd.cmd[11] & 0x03);
                ptmpDevNode->usCmdValue = bh_RcvCmd.cmd[12] | (bh_RcvCmd.cmd[13] << 8);
                ptmpDevNode->ucCmdCnt = 0;

                /**
                 * 由于只和内核进(线)程上下文存在竞争，而interrupt、bottomHalf可抢断进(线)程，故而此处自旋锁保护是多余的。 
                 *
                 *                    ***保留此已注释的代码，备忘 ***
                 **/

                //spin_lock_irqsave(&(host.stRmtCmdList.spinlock), flags);
                pRmtCmdNode = rmtCmdNode_search(&host.stRmtCmdList.RcvSmplexRmtCmdRoot_rb, ptmpDevNode->stIdMsg.ucPhysAddr, cmpPhysAddr);
                if(pRmtCmdNode == NULL)
                {
                    pRmtCmdNode = kzalloc(sizeof(struct RmtCmdNode_s), GFP_ATOMIC);   /** 从中断处理上下文(tasklet)代码中分配内存. 从不睡眠 **/
                    if(pRmtCmdNode == NULL)
                    {
                        printk("[%s-%d]Info: kzalloc failed \n", __func__, __LINE__);
                        return;
                    }
                    pRmtCmdNode->devNode = ptmpDevNode;
                    RmtCmdNode_insert(&host.stRmtCmdList.RcvSmplexRmtCmdRoot_rb, pRmtCmdNode, cmpPhysAddr);
                }
                //spin_unlock_irqrestore(&(host.stRmtCmdList.spinlock), flags);

                if(waitqueue_active(&(host.stRmtCmdList.rmtCmdq)))
                {
                    wake_up(&(host.stRmtCmdList.rmtCmdq));        /** 唤醒信息处理线程 **/
                    printk("[%s-%d]Info: wake_up information(0x5x).\n", __func__, __LINE__);
                }
            }
            break;
        case    CDIRECTPUT: /** 0x73. ignore 0x74 **/
            {
                struct RmtCmdNode_s *pRmtCmdNode;

                /** find data area and store data **/
                ptmpDevNode = DevNode_search(&(host.DevInfoHead.DevInfoRoot_rb), stTmp.stDevCmd.stDevCmdC.ucSrcAddr, cmpPhysAddr); 
                if(NULL == ptmpDevNode)
                {
                    printk("[%s-%d]Info: Can't find node[0x%02x, 0x%02x, 0x%02x, 0x%02x], but received its command !!!\n", 
                            __func__, __LINE__,
                            stTmp.stDevCmd.stDevCmdA.ucSrcAddr[0],
                            stTmp.stDevCmd.stDevCmdA.ucSrcAddr[1],
                            stTmp.stDevCmd.stDevCmdA.ucSrcAddr[2],
                            stTmp.stDevCmd.stDevCmdA.ucSrcAddr[3]
                          );
                    return;
                }
#if 0
                ptmpDevNode->ucCmdOpcode = MOPCODE;
                ptmpDevNode->ucCmdAttr = (pBhMsg->Messages[5] & 0x03);
                ptmpDevNode->usCmdValue = pBhMsg->Messages[6] | (pBhMsg->Messages[7] << 8);
                ptmpDevNode->ucCmdCnt = 0;
#else
                stTmp.stDevCmd.stDevCmdA.ucOpcode = bh_RcvCmd.cmd[10];
                ptmpDevNode->usAttr[(bh_RcvCmd.cmd[11] & 0x03)] = bh_RcvCmd.cmd[12] | (bh_RcvCmd.cmd[13] << 8);
                ptmpDevNode->ulJiffies = jiffies;
                ptmpDevNode->ucStatValid = 1;           /** 此设备的动态信息有效 **/
#endif
                /**
                 * 由于只和进(线)程上下文存在竞争，而interrupt、bottomHalf可抢断进(线)程，故而此处自旋锁保护是多余的。 
                 *
                 *                    ***保留此已注释的代码，备忘 ***
                 **/

                //spin_lock_irqsave(&(host.stRmtCmdList.spinlock), flags);
                pRmtCmdNode = rmtCmdNode_search(&host.stRmtCmdList.RcvDirectCmdRoot_rb, ptmpDevNode->stIdMsg.ucPhysAddr, cmpPhysAddr);
                if(pRmtCmdNode == NULL)
                {
                    pRmtCmdNode = kzalloc(sizeof(struct RmtCmdNode_s), GFP_ATOMIC);   /** 从中断处理和进程上下文之外的其他代码中分配内存. 从不睡眠 **/
                    if(pRmtCmdNode == NULL)
                    {
                        printk("[%s-%d]Info: kzalloc failed \n", __func__, __LINE__);
                        return;
                    }
                    pRmtCmdNode->devNode = ptmpDevNode;
                    RmtCmdNode_insert(&host.stRmtCmdList.RcvDirectCmdRoot_rb, pRmtCmdNode, cmpPhysAddr);
                }
                //spin_unlock_irqrestore(&(host.stRmtCmdList.spinlock), flags);

                if(waitqueue_active(&(host.stRmtCmdList.rmtCmdq)))
                {
                    wake_up(&(host.stRmtCmdList.rmtCmdq));        /** 唤醒信息处理线程 **/
                    printk("[%s-%d]Debug: wake_up information(0x7x).\n", __func__, __LINE__);
                }
            }
            break;
        case    CDUPLEXSETSTA: /** 0x62. ignore 0x61/0x63 **/
            {
                struct RmtCmdNode_s *pRmtCmdNode;

                ptmpDevNode = DevNode_search(&(host.DevInfoHead.DevInfoRoot_rb), stTmp.stDevCmd.stDevCmdC.ucDestAddr, cmpPhysAddr); 
                if(NULL == ptmpDevNode)
                {
                    printk("[%s-%d]: Can't find node: Addr[0x%x, 0x%x, 0x%x, 0x%x], but received its command !!!\n",
                            __func__, __LINE__,
                            stTmp.stDevCmd.stDevCmdD.ucDestAddr[0],
                            stTmp.stDevCmd.stDevCmdD.ucDestAddr[1],
                            stTmp.stDevCmd.stDevCmdD.ucDestAddr[2],
                            stTmp.stDevCmd.stDevCmdD.ucDestAddr[3]
                          );
                    RcvCanFrm_reset(rcvDataRoot_rb);        /** destory the rbtree **/
                    return;
                }
#if 0   /** debug  **/
                printk("[%s-%d]: receive command to node: Addr[0x%x, 0x%x, 0x%x, 0x%x] !!!\n",
                            __func__, __LINE__,
                            stTmp.stDevCmd.stDevCmdD.ucDestAddr[0],
                            stTmp.stDevCmd.stDevCmdD.ucDestAddr[1],
                            stTmp.stDevCmd.stDevCmdD.ucDestAddr[2],
                            stTmp.stDevCmd.stDevCmdD.ucDestAddr[3]
                          );
#endif
                ptmpDevNode->ucCmdOpcode = MOPCODE;
                ptmpDevNode->ucCmdAttr = (bh_RcvCmd.cmd[11] & 0x03);
                ptmpDevNode->usCmdValue = (bh_RcvCmd.cmd[12] | (bh_RcvCmd.cmd[13] << 8));

                ptmpDevNode->ucPairAddr[0] = bh_RcvCmd.cmd[4];
                ptmpDevNode->ucPairAddr[1] = bh_RcvCmd.cmd[5];
                ptmpDevNode->ucPairAddr[2] = bh_RcvCmd.cmd[6];
                ptmpDevNode->ucPairAddr[3] = bh_RcvCmd.cmd[7];
                ptmpDevNode->ucPairAdapterNo = (bh_RcvCmd.uiEID & 0x3f);

                //spin_lock_irqsave(&(host.stRmtCmdList.spinlock), flags);
                pRmtCmdNode = rmtCmdNode_search(&host.stRmtCmdList.RcvDuplexRmtCmdRoot_rb, ptmpDevNode->stIdMsg.ucPhysAddr, cmpPhysAddr);
                if(pRmtCmdNode == NULL)
                {
                    pRmtCmdNode = kzalloc(sizeof(struct RmtCmdNode_s), GFP_ATOMIC);   /** 从tasklet(中断bottom half)上下文中分配内存. 从不睡眠 **/
                    if(pRmtCmdNode == NULL)
                    {
                        printk("[%s-%d]Warning: kzalloc failed \n", __func__, __LINE__);
                        RcvCanFrm_reset(rcvDataRoot_rb);        /** destory the rbtree **/
                        return;
                    }
                    pRmtCmdNode->devNode = ptmpDevNode;
                    RmtCmdNode_insert(&host.stRmtCmdList.RcvDuplexRmtCmdRoot_rb, pRmtCmdNode, cmpPhysAddr);
                }
                //spin_unlock_irqrestore(&(host.stRmtCmdList.spinlock), flags);

                if(waitqueue_active(&(host.stRmtCmdList.rmtCmdq)))
                {
                    wake_up(&(host.stRmtCmdList.rmtCmdq));        /** 唤醒信息处理线程 **/
                    //printk("[%s-%d]Info: wake_up for set device(0x62 cmd).\n", __func__, __LINE__);
#if 0
                    printk("[%s-%d]Info: wake_up for set device(0x62 cmd) from addr[0x%x, 0x%x, 0x%x, 0x%x] !!!",
                            __func__, __LINE__ ,
                            stTmp.stDevCmd.stDevCmdD.ucDestAddr[0],
                            stTmp.stDevCmd.stDevCmdD.ucDestAddr[1],
                            stTmp.stDevCmd.stDevCmdD.ucDestAddr[2],
                            stTmp.stDevCmd.stDevCmdD.ucDestAddr[3]
                            );
#endif
                }
            }
            break;

        default:
            {
                printk("[%s-%d]Warning: Unrecognized Opcode: 0x%02x !!! \n", __func__, __LINE__, MOPCODE);
                usErrNum++;
                if(0 == (usErrNum & 0x0f))    /** Error occured some times  **/
                {
                    /** do something here **/
                }
                RcvCanFrm_reset(rcvDataRoot_rb);        /** destory the rbtree **/
            }
            break;
    }
#if 0
    if(MIsDUPRMTCmd(MOPCODE)) /** 0x62/0x63 **/
    {

    }
    else
    {   
        usErrNum++;
        printk("[%s-%d]OPCODE = 0x%x !!!\n", __func__, __LINE__, MOPCODE);
        if(0 == (usErrNum & 0x0f))    /** Error occured some times  **/
        {
            RcvCanFrm_reset(rcvDataRoot_rb);        /** destory the rbtree **/
        }
        return;
    }
#endif

#undef MOPCODE

}

/*********************************************************************************************
 *
 * 定期发查询广播
 *
 * 注意其申请内存的过程只有一次，并且只是用来计数
 *
 *********************************************************************************************/
void timer_handle(unsigned long arg)
{
#if 1
    if((host.iTimerPollPeriod != 0) && (host.iTimerPollDelay != 0))  /** 如果轮询周期不是0 **/
    {
        atomic_inc(&host.stRmtCmdList.atTimerExpire);

        if(waitqueue_active(&(host.stRmtCmdList.rmtCmdq)))
        {
            wake_up(&(host.stRmtCmdList.rmtCmdq));          /** 唤醒信息处理线程 **/
            //printk("[%s-%d]Info: wake_up for timer.\n", __func__, __LINE__);
        }
    }
#endif
}

ssize_t write_version(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
    return   1;
}

int read_version(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len;
    //len = sprintf(page, "\n\tHuNan HuaRain Science & Technology Co.Ltd\n\twww.huarain.com\n\tVersion: %s \n\tcompile date time: %s\n\n", version, date);
    len = sprintf(page, "\n\t"
            "HuNan HuaRain Science & Technology Co.Ltd\n\t"
            "www.huarain.com\n\t"
            "mcp2510-driver Version: %s \n\t"
            "compile date time: %s\n\n", 
            version, 
            date);

    return  len;
}

#if 0
/** device node list. read only **/
ssize_t write_nodeList(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
       return   1;
}

int read_nodeList(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len;

    //len = sprintf(page,"jiffies = %ld \n", jiffies);
    
    //len = DevNode_export(&DevInfoRoot_rb, page, 0);
    //len = DevNode_export(&(host.DevInfoHead.DevInfoRoot_rb), page, 0);
    len = DevNode_export(&(((struct host_s *)data)->DevInfoHead.DevInfoRoot_rb), page, 0);
    
    return len;
}
#endif

/** 
 * white list. read/write 
 *
 * demand whiteNameList entry format below:
 * e.g.
 * addr(unique id)  displayName valid
 * 56789abc         Light       1
 * 0000045a         Light       1
 * 000001fa         Light       0
 *
 * the entry begin with "#", we see it invalid entry
 **/
ssize_t write_whiteList(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
    struct whiteListNode_s *whiteListNode;
    //unsigned long flags;
    char *tmpbuff;
    char addr[4];
    char dispName[32];
    char valid;
    char *src;
    char *ptr;
    //char *sep;
    int iRet;
    int iTmp;
    int i;

    len = (len <= PAGE_SIZE)? len : PAGE_SIZE;
    tmpbuff = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if(NULL == tmpbuff)
    {
        printk("[%s-%d]vmalloc failed !\n", __FILE__, __LINE__);
        iRet =  -ENOMEM;
        goto    err0;
    }
    src = tmpbuff;

    iRet = copy_from_user(src, buff, len);
    if(0 != iRet)
    {
        printk("[%s-%d]copy_from_user() failed. src: [%s]. buff: [%s]. len: [%ld]. iRet: [%d] !\n", __FILE__, __LINE__,
                src, buff, len, iRet
                );
        iRet = -EFAULT;
        goto    err1;
    }
 
    /** Now, parse the entry into three field: address, displayName, valid **/
    if(NULL == (src = whiteNameNode_start(src, " \t")))      /** skip start SPACE, TAB **/
    {
        /** all SPACE TAB **/
        printk("[%s-%d]whiteNameNode_start !\n", __FILE__, __LINE__);
        iRet = -EFAULT;
        goto    err1;
    }
    if('#' == *src)         /** start with '#', it is an annotate **/
    {
        printk("[%s-%d]info:  # annotate!\n", __FILE__, __LINE__);
        iRet = -EFAULT;
        goto    err1;
    }
    if('\n' == *src)         /** only '\n', it is an blank **/
    {
        printk("[%s-%d]info:  blank line !\n", __FILE__, __LINE__);
        iRet = -EFAULT;
        goto    err1;
    }
/** phrase addr **/
    ptr = strsep(&src, " \t");
    iTmp = strlen(ptr);
    for(i = 0; i < iTmp; i++)
    {
        if(0 == isxdigit(ptr[i]))
        {
            printk("[%s-%d]address field illegal: %s\n", __FILE__, __LINE__, ptr);
            iRet = -EFAULT;
            goto    err1;
        }
    }
    if(iTmp > 8)    /** 如果多于4byte, 只保留最后的4byte **/
    {
        for(i = 0; i < 8; i++)
        {
            ptr[i] = ptr[i + (iTmp - 8)];
        }
        ptr[8] = '\0';
        iTmp = 8;
    }

/** phrase addr **/
#if 1
    for(i = 0; i < 4; i++)
    {
        addr[i] = 0;
    }
    for(i = 0; i < iTmp / 2; i++)
    {
        addr[i] = char2x(ptr[iTmp - (i * 2) - 2], ptr[iTmp - (i * 2) - 1]);
    }
    if(0 != (iTmp & 0x1))   /** 奇数个位 **/
    {
        addr[i] = char2x('0', ptr[iTmp - (i * 2) - 1]);
    }
#endif
/** phrase dispName **/
    if(NULL == (src = whiteNameNode_start(src, " \t")))      /** skip start SPACE, TAB **/
    {
        /** all SPACE TAB **/
        printk("[%s-%d]illegal entry, no display valid field!\n", __FILE__, __LINE__);
        iRet = -EFAULT;
        goto    err1;
    }
    ptr = strsep(&src, " \t");
    iTmp = strlen(ptr);
    iTmp = iTmp < (ARRAY_SIZE(dispName) - 1) ? iTmp : (ARRAY_SIZE(dispName) - 1);
    strncpy(dispName, ptr, iTmp);
    dispName[iTmp] = '\0';              /** make sure the array is end with '\0' **/
/** phrase valid filed 0 or 1 **/
    if(NULL == (src = whiteNameNode_start(src, " \t")))     /** skip start SPACE, TAB **/
    {
        /** all SPACE TAB **/
        printk("[%s-%d]illegal entry, no valid field!\n", __FILE__, __LINE__);
        iRet = -EFAULT;
        goto    err1;
    }
    ptr = strsep(&src, " \t\n");
    iTmp = strlen(ptr);
    valid = *ptr;
    if((iTmp != 1) || ((*ptr != '0') && (*ptr != '1')))     /** 长度必须是1, 内容必须是0或1 **/
    {
        printk("[%s-%d]valid field illegal: %s\n", __FILE__, __LINE__, ptr);
        iRet = -EFAULT;
        goto    err1;
    }
   /** 
    * 如果直接依据白名单内的数据创建数据区及节点，纯粹的白名单就没有必要了，那就直接创建设备的数据区及设备节点
    *
    **/
#if 0   /** !!!!!!!!!!!!! **/
    if(host.devNodeFromWhite != 0)
    {
        struct canbusDevice_s *pDevNode = DevNode_search(&(host.DevInfoHead.DevInfoRoot_rb), addr, cmpPhysAddr); 
        int ret;
        if(NULL == pDevNode)
        {
            pDevNode = kzalloc(sizeof(struct canbusDevice_s), GFP_ATOMIC);
            if(NULL == pDevNode)
            {
                printk("[%s-%d]Warning: kzalloc() failed !\n", __func__, __LINE__);
                return  -1;
            }
            memcpy((pDevNode->stIdMsg.ucPhysAddr), 
                    addr,
                    sizeof(addr));
            DevNode_insert(&host, pDevNode, cmpPhysAddr);

            pDevNode->ucIdValid = 1;                 /** 此设备有效 **/
            pDevNode->ucStatValid = 0;               /** 此设备的动态信息无效 **/
            pDevNode->ulJiffies = jiffies;
            init_waitqueue_head(&(pDevNode->rStatq));        /** initial wait queue for read stat **/
            //pDevNode->ucAdapterNo = searchAdapterNo(0, (pBhMsg->EID & 0x3f)); /** search AdapterNo with specified EID **/
            pDevNode->ucAdapterNo = 1;
        #if 0   /** I don't know which adapter does the device attached! **/
            if(pDevNode->ucAdapterNo == 0)   /**  **/
            {
                printk("[%s-%d]searchAdapterNo Warning: AdapterNo(SID, EID) = 0(0x%x, 0x%x)\n", __func__, __LINE__, pBhMsg->SID, pBhMsg->EID);
                return;
            }
        #endif
            /** 设备类型以设备发过来的为主，如果没有，就用厂商号+产品号查设备类型  **/
            if(0 == pDevNode->stIdMsg.usDevType)
            {
                /** 通过厂商号+产品号查得设备的类型号 **/
                pDevNode->stIdMsg.usDevType = searchDevNum(pDevNode->stIdMsg.usVendorNo, pDevNode->stIdMsg.usProductNo);
            }
            if(0 == pDevNode->stIdMsg.usDevType)
            {
                pDevNode->stIdMsg.usDevType = 1;
            }
            ret = pDevNode->stIdMsg.usDevType; 

            if(0 == devTypeTab[ret].devNO)  /** 以此类型设备的设备号作依据, 判断设备驱动是否已经加载 **/
            {   /** 还没加载驱动 **/
                devTypeTab[ret].uiMaxMinorNo = 0;
                pDevNode->no = 0;
            }
            else
            {
                devTypeTab[ret].uiMaxMinorNo++;
                pDevNode->no = MKDEV(MAJOR(devTypeTab[ret].devNO), devTypeTab[ret].uiMaxMinorNo);
            }

            /** 本地(进程上下文)与tasklet(中断上下方)有竞争关系，如何同步? **/
            list_add(&(pDevNode->stDevList), &(devTypeTab[ret].stDevList));       /** 将数据区挂入相应类型的链表中 **/

            /** 本地(进程上下文)与tasklet(中断上下文)有竞争关系, 在此用自旋锁同步 **/
            spin_lock_irqsave(&(stNoNodeList.spinlock), flags);
            list_add(&(pDevNode->stNoNode), &(stNoNodeList.stNoNode));            /** 将数据区挂入尚未建立应用层设备节点的链表中 **/
            spin_unlock_irqrestore(&(stNoNodeList.spinlock), flags);

            schedule_work(&host.can_work);       /** creat character device node **/
        }
        return  0;
    }
#endif
    whiteListNode = whiteNameNode_search(&(whiteListHead.whiteListRoot_rb), addr, cmpPhysAddr);
    if(NULL == whiteListNode)
    {
        whiteListNode = kmalloc(sizeof(struct whiteListNode_s), GFP_KERNEL);
        for(i = 0; i < 4; i++)
        {
            whiteListNode->ucPhysAddr[i] = addr[i];
        }
        whiteNameNode_insert(&(whiteListHead.whiteListRoot_rb), whiteListNode, cmpPhysAddr);
    }
    memcpy(whiteListNode->ucDispName, dispName, sizeof(dispName));
    whiteListNode->ucValid = valid;

    iRet = len;
err1:
    kfree(tmpbuff);
err0:
    return   iRet;
}

int read_whiteList(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len;

    len = whiteListNode_export(&(whiteListHead.whiteListRoot_rb), page, 0);
    
    return len;
}

/** access list read only **/
ssize_t write_accessList(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
       return   1;
}

int read_accessList(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len;

    len = sprintf(page,"jiffies = %lu \n", jiffies);
    return len;
}

/** Enable whiteList status **/
//ssize_t write_whiteListEnable(struct file *filp, const char __user *buff, unsigned long len, void *data)
ssize_t write_Enable(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
    int iRet;
    int src = 12345;    /** not equ to 0 or 1 **/
    int *ptr = data;

    len = ((len > 4)? 4 : len);
    iRet = copy_from_user((char *)&src, buff, len);

    //printk("[%s-%d], data: %d, len: %lu\n", __FILE__, __LINE__, src, len);

    src &= 0xff;
    if((src >= '0') && (src <= '1'))    /** only '0' or '1' accept **/
    {
        *ptr = src & 0x0f;      /** ASCII to digit **/
    }
    return len;
}
//int read_whiteListEnable(char *page, char **start, off_t off, int count, int *eof, void *data)
int read_Enable(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len;

    len = sprintf(page,"%d\n", *(int *)data);

    return len;
}

/** write value int **/
ssize_t write_value(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
    int iRet;
    char src[8] = {'0'};    /** not equ to 0 or 1 **/
    //int *ptr = data;
    int i;
    int itmp = 0;

    len = ((len > 8)? 8 : len);
    iRet = copy_from_user((char *)&src, buff, len);
    //*ptr = 0;

    //printk("[%s-%d], data: %d, len: %lu\n", __FILE__, __LINE__, src, len);

    for(i = 0; i < len; i++)
    {
        if((src[i] >= '0') && (src[i] <= '9'))
        {
            itmp = (itmp * 10) + (src[i] & 0x0f);      /** ASCII to digit **/
        }
        else
        {
            break;
        }
    }

    *(int *)data = itmp;

    return i + 1;
}

int read_value(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len;

    len = sprintf(page,"%d\n", *(int *)data);

    return len;
}

/**
 * 设备类型注册接口
 * input: __devno - majorNO
 *        __num - NO of the device
 *        __fops - operation function pointer
 * output: no
 * return: no
 **/
void register_canDev(dev_t __devno, int __num, struct file_operations* __fops)
{
    struct canbusDevice_s *pCanbusDev;
    //struct canbusDevice_s *ptmp;
    //struct list_head *pos;
    unsigned int majorNO;
    unsigned int minorNO;

    devTypeTab[__num].devNO = __devno;
    devTypeTab[__num].uiMaxMinorNo = 0;
    devTypeTab[__num].pstFops = __fops;


    //printk("[%s-%d]Debug: MAJOR devno: %d.\n", __func__, __LINE__, MAJOR(__devno));
    majorNO = MAJOR(devTypeTab[__num].devNO);
    minorNO = devTypeTab[__num].uiMaxMinorNo;

    if(!list_empty(&(devTypeTab[__num].stDevList)))     /** this type device have been recongized **/
    {

        /** check device TypeTab and initial all cdev in the spec list **/
        list_for_each_entry(pCanbusDev, &(devTypeTab[__num].stDevList), stDevList);
        {
            minorNO += 1;
            pCanbusDev->no = MKDEV(majorNO, minorNO);

            cdev_init(&(pCanbusDev->dev), devTypeTab[__num].pstFops);
            cdev_add(&(pCanbusDev->dev), pCanbusDev->no, 1);
        }

        devTypeTab[__num].uiMaxMinorNo = minorNO;
    }

}
EXPORT_SYMBOL(register_canDev);

/** inline void unregister_canDev(int num) **/
void unregister_canDev(int __num)
{
    struct canbusDevice_s *pCanbusDev;

    devTypeTab[__num].devNO = 0;

    if(!list_empty(&(devTypeTab[__num].stDevList)))
    {
        /** check device TypeTab and clear all cdev in the spec list **/
        list_for_each_entry(pCanbusDev, &(devTypeTab[__num].stDevList), stDevList);
        {
            pCanbusDev->no = (dev_t)0;
            cdev_del(&(pCanbusDev->dev));
        }
    }
}
EXPORT_SYMBOL(unregister_canDev);

/**************************************************************
 *
 *  内核线程处理：
 *  1、单向遥控命令(0x52命令)
 *  2、双向遥控命令(0x61/0x62)
 *  3、直接操作的告知回复()
 *
 **************************************************************/
int kthread_rmtFunc(void *data)
{
    //struct RmtCmdList_s *rmtCmdList = (struct RmtCmdList_s *)data;
    struct RmtCmdList_s *rmtCmdList = &host.stRmtCmdList;
    struct canbusDevice_s *devNode;
    struct RmtCmdNode_s *rmtCmdNode;
    struct rb_root *root;
    struct rb_node *node;
    unsigned long flags;
    int resetFlag;
    int ret;

    while(1)
    {
        //printk("[%s-%d]Debug: wait_event_interruptible sleep.\n", __func__, __LINE__);
        wait_event_interruptible(rmtCmdList->rmtCmdq, 
                ((NULL != rmtCmdList->SndDuplexRmtAckRoot_rb.rb_node)
                 || (NULL != rmtCmdList->RcvDuplexRmtCmdRoot_rb.rb_node)
                 || (NULL != rmtCmdList->RcvDirectCmdRoot_rb.rb_node) 
                 || (NULL != rmtCmdList->RcvSmplexRmtCmdRoot_rb.rb_node)
                 || (0 != atomic_read(&rmtCmdList->atTimerExpire))
                 || ((0 != kfifo_len(rmtCmdList->pSndBuff)) || (rmtCmdList->iErrCount != 0))));
        //printk("[%s-%d]Debug: wait_event_interruptible wake up.\n", __func__, __LINE__);

        /** 
         * Part I 直接控制 
         *
         * 设备是动作的发起者，主机不负责传输的可靠性
         **/
        spin_lock_irqsave(&(rmtCmdList->spinlock), flags);
        rmtCmdNode_move(&rmtCmdList->DoingRmtCmdRoot_rb, &rmtCmdList->RcvDirectCmdRoot_rb);
        spin_unlock_irqrestore(&(rmtCmdList->spinlock), flags);

        root = &rmtCmdList->DoingRmtCmdRoot_rb;
        if(NULL != root->rb_node)    /** Is there any date to be process ? **/
        {
            for(node = rb_first(root); node; node = rb_next(node))
            {
                devNode = rb_entry(node, struct RmtCmdNode_s, node_rb)->devNode;
                devNode->ucStatValid = 0;
                devNode->ucCmdCnt = 0;
            }

            for(node = rb_first(root); node; node = rb_next(node))
            {
                rmtCmdNode = rb_entry(node, struct RmtCmdNode_s, node_rb);
                devNode =  rmtCmdNode->devNode;

                ret = mcp2510Pkt_write(CDIRECTACK, (unsigned char *)&devNode->usCmdValue, sizeof(devNode->usCmdValue) /**ret**/, devNode);
                if(ret < 0)
                {
                    printk("[%s-%d]Warning: mcp2510Pkt_write failed!\n", __func__, __LINE__);

                }
                /** delay at least 10ms **/
                //schedule_timeout(HZ/5 + 1);
                schedule_timeout(HZ);
            }
            RmtCmdNode_reset(root);
        }

        /** Part II 单向遥控 **/
        spin_lock_irqsave(&(rmtCmdList->spinlock), flags);
        rmtCmdNode_move(&rmtCmdList->DoingRmtCmdRoot_rb, &rmtCmdList->RcvSmplexRmtCmdRoot_rb);
        spin_unlock_irqrestore(&(rmtCmdList->spinlock), flags);

        root = &rmtCmdList->DoingRmtCmdRoot_rb;
        if(NULL != root->rb_node)   /** Is there any date to be process ?**/
        {
            for(node = rb_first(root); node; node = rb_next(node))
            {
                devNode = rb_entry(node, struct RmtCmdNode_s, node_rb)->devNode;
                devNode->ucStatValid = 0;
                devNode->ucCmdCnt = 0;
            }

            while(NULL != root->rb_node)
            {
                unsigned long j1 = jiffies + HZ;
                resetFlag = 1;
                for(node = rb_first(root); node; node = rb_next(node))
                {
                    rmtCmdNode = rb_entry(node, struct RmtCmdNode_s, node_rb);
                    devNode =  rmtCmdNode->devNode;

                    if((devNode->ucStatValid == 0) && (devNode->ucCmdCnt <= 0x03))
                    {
                        resetFlag = 0;      /** if only one  **/

                        ret = mcp2510Pkt_write(CSETSTATUS, (unsigned char *)&devNode->usCmdValue, sizeof(devNode->usCmdValue) /**ret**/, devNode);
                        if(ret < 0)
                        {
                            printk("[%s-%d]Warning: mcp2510Pkt_write failed!\n", __func__, __LINE__);
                        }
                        else
                        {
                            devNode->ucCmdCnt  += 1;
                            //printk("[%s-%d]Debug: information.\n", __func__, __LINE__);
                        }
                        /** delay at least 10ms **/
                        //schedule_timeout(HZ);
                        j1 = jiffies + HZ / 5;
                        while(time_before(jiffies, j1))
                        {
                            schedule();
                        }
                    }
                    else /** devNode->ucStatValid == 1 received responsed, or  devNode->ucCmdCnt > 0x03 failed over 3 times  **/
                    {
                        //rb_erase(&(rmtCmdNode->node_rb), root);
                        //kfree(rmtCmdNode);

                        /**
                          printk("[%s-%d]Debug: devNode->ucStatValid = %d, devNode->ucCmdCnt = %d.\n", 
                          __func__, __LINE__,
                          devNode->ucStatValid, 
                          devNode->ucCmdCnt
                          );
                         **/
                    }
                }
                if(resetFlag == 1)
                {
                    RmtCmdNode_reset(root);
                }
                else
                {
                    /**
                     * 实验发现，j2 = jiffies + HZ 时一定会收到回答
                     *           j2 = jiffies + HZ/10 时最好的情况下，第二次发出命令后会收到回答, 但大多数情况下会出现错误数据
                     *           j2 = jiffies + HZ/5  时最好的情况下，一次发送，也有很多，发两次的，
                     *                                                但从收到的回答次数等于计数个数来看，并没有发生数据丢失现象, 只是网络传输延时   
                     **/
                    //unsigned long j2 = jiffies + HZ/10;
                    unsigned long j2 = jiffies + HZ/2;
                    //unsigned long j2 = jiffies + HZ;
                    while(time_before(jiffies, j2))
                    {
                        schedule();
                    }
                }
            }
        }

        /** Part III 双向遥控 **/
        spin_lock_irqsave(&(rmtCmdList->spinlock), flags);
        rmtCmdNode_move(&rmtCmdList->DoingRmtCmdRoot_rb, &rmtCmdList->RcvDuplexRmtCmdRoot_rb);
        spin_unlock_irqrestore(&(rmtCmdList->spinlock), flags);

        root = &rmtCmdList->DoingRmtCmdRoot_rb;
        if(NULL != root->rb_node)   /** Is there any data to be process ?**/
        {
            unsigned long j1 = jiffies + HZ;
            //unsigned long j1 = jiffies + HZ;

            for(node = rb_first(root); node; node = rb_next(node))
            {
                devNode = rb_entry(node, struct RmtCmdNode_s, node_rb)->devNode;
                devNode->ucStatValid = 0;
                devNode->ucCmdCnt = 0;
                //devNode->ucCmdAck = 0;
                devNode->ucCmdWaitAck = 1;
            }
            /** 
             * 处理条件:  
             *          1、有数据节点等待处理
             *          2、处理时限()
             *
             * 由于不是传输的发起者，所以不负责回复确认, 也不负责重传
             *
             **/
            while(NULL != root->rb_node)
            {
                resetFlag = 1;
                for(node = rb_first(root); (node && time_before(jiffies, j1)); node = rb_next(node))
                {
                    rmtCmdNode = rb_entry(node, struct RmtCmdNode_s, node_rb);
                    devNode =  rmtCmdNode->devNode;

                    if((devNode->ucStatValid == 0) && (devNode->ucCmdCnt == 0x0))      /** 没有收到过控制命令的回复而且没有成功发出过数据 **/
                    {
                        ret = mcp2510Pkt_write(CSETSTATUS, (unsigned char *)&devNode->usCmdValue, sizeof(devNode->usCmdValue) /**ret**/, devNode);
                        //printk("[%s-%d]Debug: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!information.\n", __func__, __LINE__);
                        if(ret < 0)
                        {
                            printk("[%s-%d]Warning: mcp2510Pkt_write failed!\n", __func__, __LINE__);
                            resetFlag = 0;                                              /** send failed **/
                        }
                        else
                        {
                            devNode->ucCmdCnt  += 1;
                            //printk("[%s-%d]Debug: information.\n", __func__, __LINE__);
                        }
                        /** delay at least 10ms **/

                        //schedule_timeout(HZ/100 + 1);
                        schedule_timeout(HZ);
                    }
                }
                if(resetFlag == 1)
                {
                    RmtCmdNode_reset(root);
                }
            }
        }

        /** Part IV 双向遥控后续应答 **/
        spin_lock_irqsave(&(rmtCmdList->spinlock), flags);
        rmtCmdNode_move(&rmtCmdList->DoingRmtCmdRoot_rb, &rmtCmdList->SndDuplexRmtAckRoot_rb);
        spin_unlock_irqrestore(&(rmtCmdList->spinlock), flags);

        root = &rmtCmdList->DoingRmtCmdRoot_rb;
        //if(NULL != root->rb_node) 
        while(NULL != root->rb_node)  /** Is there any data to be process ?**/
        {
            unsigned long j1 = jiffies + HZ;
            /** 
             * 处理条件:  
             *          1、有数据节点等待处理
             *          2、处理时限()
             *
             * 由于不是传输的发起者，所以不负责回复确认, 也不负责重传
             *
             **/

            resetFlag = 1;
            for(node = rb_first(root); (node && time_before(jiffies, j1)); node = rb_next(node))
            {
                rmtCmdNode = rb_entry(node, struct RmtCmdNode_s, node_rb);
                devNode =  rmtCmdNode->devNode;

                if(devNode->ucCmdWaitAck == 1)  /** 等待答复 **/
                {
                    devNode->ucCmdWaitAck  = 0;
                    if(devNode->ucStatValid == 1)   /** 已经收到过控制器命令的回复 **/
                    {
                        ret = mcp2510Pkt_write(CDUPLEXPUTACK, (unsigned char *)&devNode->usAttr[1], sizeof(devNode->usAttr[1]) /**ret**/, devNode);
                        if(ret < 0)
                        {
                            resetFlag = 0;              /** if only one  **/
                            printk("[%s-%d]Warning: mcp2510Pkt_write failed!\n", __func__, __LINE__);
                        }
                        else
                        {
                            /** send Ack **/
                            devNode->ucCmdWaitAck = 0;
                        }
                        /** delay at least 10ms **/
                        schedule_timeout(HZ);
                    }
                }
            }
            if(resetFlag == 1)
            {
                RmtCmdNode_reset(root);
            }
          #if 0
            else
            {
                unsigned long j2 = jiffies + HZ/5;
                //unsigned long j2 = jiffies + HZ;
                while(time_before(jiffies, j2))
                {
                    schedule();
                }
            }
          #endif
        }

        /** Part V 定时器后续处理 **/
        if(0 != atomic_read(&rmtCmdList->atTimerExpire))  /** Is there any data to be process ?**/
        {
            atomic_set(&rmtCmdList->atTimerExpire, 0);
            mcp2510Pkt_poll();           
        }

        /** Part VI 发送canbus数据帧 **/
        if((kfifo_len(rmtCmdList->pSndBuff)) || (rmtCmdList->iErrCount != 0))             /** 有数据需要发送 , 或上次发送失败 **/ 
        {
            static struct CANBus_Message   SndMsg;      /** 要发送的数据 **/

            if(rmtCmdList->iErrCount == 0)  /** 上次发送成功, 就取后续数据发送，否则继续发送上次取出的数据 **/
            {
                /** 此fifo内有spinlock同步数据的存入与取出, 无需另外同步 **/
                //spin_lock_irq(&host.stRmtCmdList.sndfifoLock);
                spin_lock_irqsave(&host.stRmtCmdList.sndfifoLock, flags);
                kfifo_get(rmtCmdList->pSndBuff, (unsigned char *)&SndMsg, sizeof(struct CANBus_Message));
                spin_unlock_irqrestore(&host.stRmtCmdList.sndfifoLock, flags);
                //spin_unlock_irq(&host.stRmtCmdList.sndfifoLock);
            }

            /** 与外部中断竞争硬件资源，必须关闭中断 **/
            spin_lock_irq(&host.canLock);
            ret = __mcp2510Pkt_write(&SndMsg);
            spin_unlock_irq(&host.canLock);
            if(-EBUSY != ret)
            {
                rmtCmdList->iErrCount = 0;
            }
            else    /** 设备忙, 有可能是canbus网络没有协调器节点 **/
            {
                rmtCmdList->iErrCount += 1;
                if(rmtCmdList->iErrCount > 1000) /** 连续发生错误  **/
                {
                    rmtCmdList->iErrCount = 0;
                    //kfifo_reset(rmtCmdList->pSndBuff);
                }
            }
        }
    }

    /** Can't goto here **/
    return 0;
}

#if 1
/**  
 * bellow code for seq_file used to export the nodeList data
 **/
static void *my_seq_start(struct seq_file *s, loff_t *pos)
{
    struct canbusDevice_s *data;
    struct rb_node *node = rb_first(&(host.DevInfoHead.DevInfoRoot_rb));

    //sprintf(exportAddr, "---DevNode_export--------------\n");
    /* beginning a new sequence ? */
    if (*pos == 0)
    {
        /* yes => return a non null value to begin the sequence */
        //printk(KERN_INFO"[%s-%d]: pos == 0\n", __func__, __LINE__);
        if(node)
        {
            //data = container_of(node, struct canbusDevice_s, node_rb);
            data = rb_entry(node, struct canbusDevice_s, node_rb);

            return  data;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        /* no => it's the end of the sequence, return end to stop reading */
        *pos = 0;
        //printk(KERN_INFO"[%s-%d]: pos != 0\n", __func__, __LINE__);
        return NULL;
    }
}

static void *my_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    struct canbusDevice_s *data = (struct canbusDevice_s *)v;
    struct rb_node *node = rb_next(&(data->node_rb));

    //printk(KERN_INFO"[%s-%d]: Invoke next\n", __func__, __LINE__);
    (*pos)++;

    if(node)
    {
        data = rb_entry(node, struct canbusDevice_s, node_rb);
    }
    else
    {
        data = NULL;
    }

    return data;
}

static void my_seq_stop(struct seq_file *s, void *v)
{
    //printk(KERN_INFO"[%s-%d]: Invoke stop\n", __func__, __LINE__);
    /** do nothing **/
}

static int my_seq_show(struct seq_file *s, void *v)
{
    struct canbusDevice_s *data = (struct canbusDevice_s *)v;
    char *exportAddr;
    char *tmpAddr;
    int i;

    //printk(KERN_INFO"[%s-%d]: Invoke show\n", __func__, __LINE__);

    tmpAddr = vmalloc(PAGE_SIZE);
    if(NULL == tmpAddr)
    {
        goto err0;
    }
    exportAddr = vmalloc(PAGE_SIZE);
    if(NULL == (exportAddr = vmalloc(PAGE_SIZE)))
    {
        goto err1;
    }

    sprintf(tmpAddr, "\nName: %s", data->ucDispName);
    strcat(exportAddr, tmpAddr);
    strcat(exportAddr, "\n");


    strcat(exportAddr, "Addr: ");
    for(i = 0; i < FIELD_SIZEOF(struct IdentifyMsg_s, ucPhysAddr); i++)
    {
        sprintf(tmpAddr, "0x%02x ", data->stIdMsg.ucPhysAddr[i]);
        strcat(exportAddr, tmpAddr);
    }
    strcat(exportAddr, "\n");

    sprintf(tmpAddr, "Vendor: 0x%04x ", data->stIdMsg.usVendorNo);
    strcat(exportAddr, tmpAddr);

    sprintf(tmpAddr, "Product: 0x%04x ", data->stIdMsg.usProductNo);
    strcat(exportAddr, tmpAddr);

    sprintf(tmpAddr, "DevType: 0x%04x ", data->stIdMsg.usDevType);
    strcat(exportAddr, tmpAddr);

    sprintf(tmpAddr, "Y/M/D: %d-%d-%d ", 
            data->stIdMsg.usYear,
            data->stIdMsg.ucMonth,
            data->stIdMsg.ucDay
           );
    strcat(exportAddr, tmpAddr);

    strcat(exportAddr, "other: ");
    //for(i = 0; i < FIELD_SIZEOF(struct IdentifyMsg_s, other); i++)
    for(i = 0; i < ARRAY_SIZE(data->stIdMsg.other); i++)
    {
        sprintf(tmpAddr, "0x%02x ", data->stIdMsg.other[i]);
        strcat(exportAddr, tmpAddr);
    }

    strcat(exportAddr, "\nusAttr: ");
    //for(i = 0; i < FIELD_SIZEOF(struct canbusDevice_s, usAttr); i++)
    for(i = 0; i < ARRAY_SIZE(data->usAttr); i++)
    {
        sprintf(tmpAddr, "0x%04x ", data->usAttr[i]);
        strcat(exportAddr, tmpAddr);
    }

    sprintf(tmpAddr, "\nucIdValid: 0x%02x ", data->ucIdValid);
    strcat(exportAddr, tmpAddr);

    sprintf(tmpAddr, "ucStatValid: 0x%02x ", data->ucStatValid);
    strcat(exportAddr, tmpAddr);

    sprintf(tmpAddr, "ulJiffies: 0x%08lx ", data->ulJiffies);
    strcat(exportAddr, tmpAddr);

    sprintf(tmpAddr, "ucAdapterNo: 0x%02x ", data->ucAdapterNo);
    strcat(exportAddr, tmpAddr);

    sprintf(tmpAddr, "ucCmdOpcode: 0x%02x ", data->ucCmdOpcode);
    strcat(exportAddr, tmpAddr);

    sprintf(tmpAddr, "ucCmdAttr: 0x%02x ", data->ucCmdAttr);
    strcat(exportAddr, tmpAddr);

    sprintf(tmpAddr, "usCmdValue: 0x%02x ", data->usCmdValue);
    strcat(exportAddr, tmpAddr);

    sprintf(tmpAddr, "ucCmdCnt: 0x%02x ", data->ucCmdCnt);
    strcat(exportAddr, tmpAddr);

    strcat(exportAddr, "\n");

    seq_printf(s, exportAddr);

    vfree(exportAddr);
err1:
    vfree(tmpAddr);
err0:
    return  0;

}

static struct seq_operations nodeList_seq_ops = {
    .start = my_seq_start,
    .next = my_seq_next,
    .stop = my_seq_stop,
    .show = my_seq_show
};

static int huaRain_nodeList_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &nodeList_seq_ops);
};

struct file_operations huaRain_nodeList_file_ops = {
    .owner = THIS_MODULE,
    .open = huaRain_nodeList_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = seq_release
};

#endif

static int open_mcp2510(struct inode *inode, struct file *filp)
{
    //filp->private_data = container_of(inode->i_cdev, struct canbusDevice_s, dev);
    filp->private_data = &host;
    return  0;
}

static int release_mcp2510(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t read_from_mcp2510(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    struct host_s *pHost = (struct host_s *)filp->private_data;
    struct CANBus_Message   *pRcvMsg;      /** 要发送的数据 **/
    int iRet = 0;


    if(buf == NULL)
    {
        iRet = -EFAULT;
        goto err0;
    }

    pRcvMsg = kzalloc(sizeof(struct CANBus_Message), GFP_KERNEL);
    if(pRcvMsg  == NULL)
    {
        printk("[%s-%d]Info:  kzalloc() failed !\n", __func__, __LINE__);
        iRet = -ENOMEM;
        goto err0;
    }
    //wait_event_interruptible(rmtCmdList->rawDataq, (0 != kfifo_len(rmtCmdList->pRcvBuff))); 
    wait_event_interruptible(pHost->stRmtCmdList.rawDataq, (0 != kfifo_len(pHost->stRmtCmdList.pRcvBuff))); 
    if(kfifo_len(pHost->stRmtCmdList.pRcvBuff))
    {
        kfifo_get(pHost->stRmtCmdList.pRcvBuff, (unsigned char *)pRcvMsg, sizeof(struct CANBus_Message));
        copy_to_user(buf, pRcvMsg, sizeof(struct CANBus_Message));

        iRet = sizeof(struct CANBus_Message);
        goto err1;
    }

err1:
    kfree(pRcvMsg);
err0:
    return iRet;
}

static ssize_t write_to_mcp2510(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    int iRet = 0;
    struct sendCmd_s *pSendCmd;

    pSendCmd = kzalloc(sizeof(struct sendCmd_s), GFP_KERNEL);
    if(pSendCmd == NULL)
    {
        printk("[%s-%d]Warning:  kzalloc() failed !\n", __func__, __LINE__);
        return -ENOMEM;
    }
    copy_from_user(pSendCmd, buf, size);

    iRet = mcp2510Pkt_writeCmdX(pSendCmd, 10); 

    printSendCmd(__func__, __LINE__, pSendCmd);
    return size;
}


#include "huaRain-ioctl.h"
static int ioctl_mcp2510(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int i = 0;
    int iRet = 0;
    long lRemain = 0;
    //unsigned long flags;
    static char destAddr[2] = {0};
    static unsigned char ucAdapterNo = 0;
    static unsigned char ucDevType = 0;
    char *pStr = NULL;
    rgstInfo_t  *pRgstInfo;

    struct host_s *pHost = filp->private_data;

    if(pHost->iEnable == 0) 
    {
        iRet = -EFAULT;
        goto err0;
    }

    switch(cmd)
    {
        case READ_STATUS:
        case WRITE_STATUS:
            //iRet = copy_to_user((char *)arg, (char *)pVarMsg, sizeof(devInfo->usAttr));
            printk("[%s-%d]: no implement the command !\n", __func__, __LINE__);
            break;
        case READ_VERSION:
            if((char *)arg == NULL)
            {
                return -EFAULT;
            }

            pStr = kzalloc(VERSION_STRING_SIZE, GFP_KERNEL);
            if(pStr == NULL)
            {
                return -ENOMEM;
            }
            sprintf(pStr, "\n\t"
                            "HuNan HuaRain Science & Technology Co.Ltd \n\t"
                            "www.huarain.com\n\t"
                            "mcp2510-driver Version: %s \n\t"
                            "compile date time: %s \n\n", 
                            version, 
                            date);

            copy_to_user((char *)arg, pStr, VERSION_STRING_SIZE);

            kfree(pStr);

            break;
        case MDevRgst:      
            /** 
             * device register 
             *
             * maybe it is is collid(conflict) with received data process. but 
             **/

            //printk("[%s-%d]info: debug !!!!!!!!!!!!!!!!!!!!!!!\n", __func__, __LINE__);

            pRgstInfo  = kzalloc(sizeof(rgstInfo_t), GFP_KERNEL);
            if(NULL == pRgstInfo)
            {
                iRet = -ENOMEM;
                goto err0;
            }

            if(arg != 0)
            {
                copy_from_user(pRgstInfo, (char *)arg, sizeof(rgstInfo_t));

                memcpy(destAddr, pRgstInfo->addr, sizeof(pRgstInfo->addr));
                ucAdapterNo = pRgstInfo->ucAdapterNo;
                ucDevType = pRgstInfo->ucDevType;
            }

            
            if(0 == atomic_read(&(pHost->rgstCmd.atRgst)))
            {
                pHost->rgstCmd.Cmd.ucAdapterNo = ucAdapterNo;
                pHost->rgstCmd.Cmd.uiEID = ucAdapterNo;
                memcpy(pHost->rgstCmd.Cmd.cmd + 4, destAddr, sizeof(destAddr));
                pHost->rgstCmd.Cmd.cmd[6] = ucDevType;
                pHost->rgstCmd.Cmd.cmd[10] = CCANSWER;    //0x12;
                pHost->rgstCmd.Cmd.cmd[11] = 0x01;    //0x12;
                pHost->rgstCmd.Cmd.len = 16;
                pHost->rgstCmd.Cmd.cmd[pHost->rgstCmd.Cmd.len - 1] = 
                    makeCrc8(0, pHost->rgstCmd.Cmd.cmd, pHost->rgstCmd.Cmd.len - 1);

                atomic_set(&(pHost->rgstCmd.atRgst), 1);    /** 同步之用, 只在tasklet中清零 **/
                pHost->rgstCmd.ucErr = 0;

                printk("[%s-%d]info: adapterNo: %x, Address: 0x%x:0x%x, deviceType: %x!\n", 
                        __func__, __LINE__,
                        pHost->rgstCmd.Cmd.ucAdapterNo,
                        pHost->rgstCmd.Cmd.cmd[4],
                        pHost->rgstCmd.Cmd.cmd[5],
                        pHost->rgstCmd.Cmd.cmd[6]
                        );
            }
            else
            {
                pHost->rgstCmd.ucErr = ((pHost->rgstCmd.ucErr + 1) & 0x03);
            }

            for(i = 0; i < 3; i++)
            {

                lRemain = wait_event_interruptible_timeout(pHost->rgstCmd.rgstq, (0 == atomic_read(&(pHost->rgstCmd.atRgst))), HZ);
                if(lRemain == 0)
                {
                    tasklet_schedule(&pHost->can_tasklet);  /** 超时!!!(在进程上下文中，此操作似乎不合理) **/
                }
            }
#if 1
            
#else
            if(1)
            {
                atomic_set(&(AdapterNoTab[(ucAdapterNo  & 0x1f)].atCompleteNo), 1);

                if(host.iEnable != 0)
                {
                    tasklet_schedule(&host.can_tasklet);
                }
            }

#endif           
            kfree(pRgstInfo);

            break;
        case MDevCtrl:      /** device control **/
            copy_from_user(destAddr, (char *)arg, sizeof(destAddr));
            break;
        case MDevSType:      /** device control **/
            copy_from_user(&ucDevType, (char *)arg, sizeof(ucDevType));
            break;
        case MDevSAdap:      /** device control **/
            copy_from_user(&ucAdapterNo, (char *)arg, sizeof(ucAdapterNo));
            break;
        case MDevSAddr:      /** device control **/
            copy_from_user(destAddr, (char *)arg, sizeof(destAddr));
            break;
        default:            /** get ID data from memory **/
            printk("[%s-%d]: Unrecognized command !\n", __func__, __LINE__);
            iRet = -EINVAL;

            break;
    }

//err1:
    
err0:
    return  iRet;
}

static struct file_operations mcp2510_fops = {
    .owner = THIS_MODULE,
    .read = read_from_mcp2510,
    .write = write_to_mcp2510,
    .ioctl = ioctl_mcp2510,
    .open = open_mcp2510,
    .release = release_mcp2510,
};
static struct miscdevice mcp2510Device = { 
    .minor = MISC_DYNAMIC_MINOR,
    .name = "mcp2510",
    .fops = &mcp2510_fops,
};

static int  __init mcp2510Dev_init(void)
{
    int ret = 0; 

    ret = hostCD_init(&host);
    if(ret != 0)
    {
        printk("kfifo_alloc failed !\n");
        ret = -EFAULT;
        goto err1;
    }

    pBhMsg = kzalloc(sizeof(struct CANBus_Message), GFP_KERNEL);
    if(pBhMsg  == NULL)
    {    
        printk("[%s-%d]kzalloc failed !\n", __func__, __LINE__);
        ret = -ENOMEM;
        goto err3;
    }    

    pIntMsg = kzalloc(sizeof(struct CANBus_Message), GFP_KERNEL);
    if(pIntMsg  == NULL)
    {    
        printk("[%s-%d]kzalloc failed !\n", __func__, __LINE__);
        ret = -ENOMEM;
        goto err4;
    } 

    /** proc process **/
    proc_intf.proc_dir = proc_mkdir("huaRain", NULL);   /** directory **/
    if(NULL == proc_intf.proc_dir)
    {
        ret = -ENOMEM;
        goto err5;
    }

    proc_intf.proc_entry[0] = create_proc_entry("version", 0644, proc_intf.proc_dir);
    proc_intf.proc_entry[0]->read_proc = read_version;
    proc_intf.proc_entry[0]->write_proc = write_version;

    proc_intf.proc_entry[1] = create_proc_entry("nodeList", 0644, proc_intf.proc_dir);
#if 1
    proc_intf.proc_entry[1]->proc_fops = &huaRain_nodeList_file_ops;
#else
    proc_intf.proc_entry[1]->data = &host;
    proc_intf.proc_entry[1]->read_proc = read_nodeList;
    proc_intf.proc_entry[1]->write_proc = write_nodeList;
#endif
    proc_intf.proc_entry[2] = create_proc_entry("whiteList", 0644, proc_intf.proc_dir);
    proc_intf.proc_entry[2]->data = &(whiteListHead.whiteListRoot_rb);
    proc_intf.proc_entry[2]->read_proc = read_whiteList;
    proc_intf.proc_entry[2]->write_proc = write_whiteList;

    proc_intf.proc_entry[3] = create_proc_entry("accessList", 0644, proc_intf.proc_dir);
    proc_intf.proc_entry[3]->read_proc = read_accessList;
    proc_intf.proc_entry[3]->write_proc = write_accessList;

    proc_intf.proc_entry[4] = create_proc_entry("whiteListEnable", 0644, proc_intf.proc_dir);
    proc_intf.proc_entry[4]->data= &whiteListHead.iEnable;
    proc_intf.proc_entry[4]->read_proc = read_Enable;
    proc_intf.proc_entry[4]->write_proc = write_Enable;

    proc_intf.proc_entry[5] = create_proc_entry("hostEnable", 0644, proc_intf.proc_dir);
    proc_intf.proc_entry[5]->data= &host.iEnable;
    proc_intf.proc_entry[5]->read_proc = read_Enable;
    proc_intf.proc_entry[5]->write_proc = write_Enable;

    proc_intf.proc_entry[6] = create_proc_entry("TimerPollPeriod", 0644, proc_intf.proc_dir);
    proc_intf.proc_entry[6]->data = &host.iTimerPollPeriod;
    proc_intf.proc_entry[6]->read_proc = read_value;
    proc_intf.proc_entry[6]->write_proc = write_value;

    proc_intf.proc_entry[7] = create_proc_entry("TimerPollDelay", 0644, proc_intf.proc_dir);
    proc_intf.proc_entry[7]->data= &host.iTimerPollDelay;
    proc_intf.proc_entry[7]->read_proc = read_value;
    proc_intf.proc_entry[7]->write_proc = write_value;

    proc_intf.proc_entry[8] = create_proc_entry("devNodeFromWhite", 0644, proc_intf.proc_dir);
    proc_intf.proc_entry[8]->data= &host.devNodeFromWhite;
    proc_intf.proc_entry[8]->read_proc = read_Enable;
    proc_intf.proc_entry[8]->write_proc = write_Enable;


#if 0
    stTmp.stDevCmd.stDevCmdA.ucpData = (unsigned char *)kmalloc(1024, GFP_KERNEL);
    if(stTmp.stDevCmd.stDevCmdA.ucpData == NULL)
    {    
        printk("[%s-%d]kzalloc failed !\n", __func__, __LINE__);
        ret = -ENOMEM;
        goto err5;
    }
#endif

    if(request_mem_region(rGPECON, 12, "rGPECON") == 0)
    {
        printk("[%s-%d]Warning: MCP2510Dev init fault!\n", __func__, __LINE__);
        ret = -EFAULT;
        goto err6;
    }

     __config_spi();

    if( __config_mcp2510())
    {
        printk("[%s-%d]Warning: __config_mcp2510 false!\n", __func__, __LINE__);
        ret = -EFAULT;
        goto err7;
    }
    disable_irq(IRQ_EINT21);    /**   **/


    /** 
     * 操作mcp2510设备的接口 
     *
     * 非必须的接口，失败仅作提示警告
     **/
    if(misc_register(&mcp2510Device))
    {
        printk("misc register mcp2510Dev false!\n");
        //release_mem_region(rGPECON, 12);
        //result = -EFAULT;
    }

    host.HuaRain_class = class_create(THIS_MODULE, "HuaRain");
    if(host.HuaRain_class == NULL)
    {
        printk("[%s-%d]Warning: class_create false!\n", __func__, __LINE__);
        ret = -EFAULT;
        goto err8;
    }

#if 0
   if (misc_register(&mcp2510Device))
    {
        printk("[%s-%d]Warning: misc register mcp2510Dev false!\n", __func__, __LINE__);
        ret = -EFAULT;
        goto err9;
    }
#endif

    DevNoClr(devTypeTab, MMaxTypeNum);

    INIT_LIST_HEAD(&(stNoNodeList.stNoNode));
    spin_lock_init(&(stNoNodeList.spinlock));

    //INIT_LIST_HEAD(&(host.stRmtCmdList.stRmtCmd));
    //INIT_LIST_HEAD(&(host.stRmtCmdList.stRmtCmd));
    host.stRmtCmdList.RcvSmplexRmtCmdRoot_rb = RB_ROOT;
    host.stRmtCmdList.DoingRmtCmdRoot_rb = RB_ROOT;
    spin_lock_init(&(host.stRmtCmdList.spinlock));
    init_waitqueue_head(&(host.stRmtCmdList.rmtCmdq));  /** initial wait queue for read stat **/
    init_waitqueue_head(&(host.stRmtCmdList.rawDataq));  /** initial wait queue for read raw data **/
    host.stRmtCmdList.task = kthread_run(kthread_rmtFunc, &(host.stRmtCmdList), "huaRainRmtProc");  /** 创建并启动kthread_rmtFunc线程 **/

    //INIT_WORK(&can_work, canwork_handle);
    INIT_WORK(&host.can_work, canwork_handle);
    tasklet_init(&host.can_tasklet, tasklet_handle, (unsigned long)&host);

#if 1
    init_timer(&host.can_timer);
    host.can_timer.function = timer_handle;
    host.can_timer.data = 0;
    host.can_timer.expires = jiffies + 3 * HZ;
    add_timer(&host.can_timer);
    host.iTimerEnable = 1;
#endif
    enable_irq(IRQ_EINT21);    /** enable the irq  **/

    return ret;

    //misc_deregister(&mcp2510Device);
//err9:
    class_destroy(host.HuaRain_class);
err8:
    __deconfig_mcp2510();
err7:
    release_mem_region(rGPECON, 12);
err6:
    //kfree(stTmp.stDevCmd.stDevCmdA.ucpData);
err5:
    kfree(pIntMsg);
err4:
    kfree(pBhMsg);
err3:
err1:
    return ret;
}

static void __exit mcp2510Dev_exit(void)
{

    iounmap(GPGDAT);
    iounmap(SPTDAT0);
    iounmap(SPRDAT0);
    iounmap(SPSTA0);
    misc_deregister(&mcp2510Device);

    release_mem_region(rGPECON, 12);
    PLOG("MCP2510dev Exit!\n");
}

module_init(mcp2510Dev_init);
module_exit(mcp2510Dev_exit);

