#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/kfifo.h>
#include <linux/wait.h>  
#include <linux/sched.h>  
#include <mach/irqs.h>

#include "../../protocol.h"
#include "../../mcp2510.h"
#include "../../lib/rbtreeRcvData.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sXie");
MODULE_VERSION("V1.0");
MODULE_DESCRIPTION("MCP2510 CANbusModule");

#define rCLKCON   0x4C00000C

#define rGPECON   0x56000040
#define rGPEDAT   0x56000044
#define rGPEUP   0x56000048

#define rGPGCON  0x56000060
#define rGPGDAT   0x56000064
#define rGPGUP   0x56000068

#define rSPCON0   0x59000000
#define rSPSTA0   0x59000004
#define rSPPIN0   0x59000008
#define rSPPRE0   0x5900000C
#define rSPTDAT0  0x59000010
#define rSPRDAT0  0x59000014

#define SPI0_READY  (ioread8((void *)SPSTA0) & 0x1)

#define MCP2510_CS_H iowrite32(ioread32((void *)GPGDAT) | (0x1<<14), (void *)GPGDAT)
#define MCP2510_CS_L iowrite32(ioread32((void *)GPGDAT) & (~(0x1<<14)), (void *)GPGDAT)


volatile unsigned int *GPGDAT;
volatile u8 *SPTDAT0;
volatile u8 *SPRDAT0;
volatile u8 *SPSTA0;

static unsigned char spiOpened;
static unsigned char TXBnCTRL[3] = {0x30, 0x40, 0x50};
struct rb_root rcvDataRoot_rb;

static struct kfifo *pRcvBuff;          //CAN接收数据缓冲区
static struct kfifo *pSndBuff;          //CAN发送数据缓冲区

static spinlock_t rcvfifoLock;          //用于CAN接收数据缓冲区的自旋锁
static spinlock_t sndfifoLock;          //用于CAN发送数据缓冲区的自旋锁
static spinlock_t canLock;              //用于CAN的操作同步(实际上是对spi的同步, 凡是spi操作的地方都要用到)
static wait_queue_head_t wqCanBuff;

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

#if 0
static void kfifo_check(char* str, int line, struct kfifo* pkfifo) 
{
    if(pkfifo != NULL) 
    {
        printk("[%s-%d]: pkfifo->size = %d\t pkfifo->in = %d\t pkfifo->out = %d\t \n", str, line, pkfifo->size, pkfifo->in, pkfifo->out);
    }
}
#endif

static void MCP2510_reset(void)
{
    MCP2510_CS_L;
    iowrite8(0xc0, (void *)SPTDAT0);
    while(!SPI0_READY);
    MCP2510_CS_H;
}

static u8 MCP2510_Read(u8 Addr)
{
    u8 result;
    MCP2510_CS_L;
    iowrite8(0x03, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(Addr, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(0xFF, (void *)SPTDAT0);
    while(!SPI0_READY);
    result = ioread8((void *)SPRDAT0);
    MCP2510_CS_H;
    return result;
}

static void MCP2510_BitModi(u8 Addr, u8 Mask, u8 Data)
{
    MCP2510_CS_L;
    iowrite8(0x05, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(Addr, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(Mask, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(Data, (void *)SPTDAT0);
    while(!SPI0_READY);
    MCP2510_CS_H;
}

static void MCP2510_Write(u8 Addr, u8 Data)
{
    MCP2510_CS_L;
    while(!SPI0_READY);
    iowrite8(0x02, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(Addr, (void *)SPTDAT0);
    while(!SPI0_READY);
    iowrite8(Data, (void *)SPTDAT0);
    while(!SPI0_READY);
    MCP2510_CS_H;
}

void printREG(char* str)
{  
#if defined(PDEBUG)
    unsigned char rCANINTF;
    unsigned char rCANCTRL;
    unsigned char rCANSTAT;
    unsigned char rTXB0CTRL;
    unsigned char rTXB1CTRL;
    unsigned char rTXB2CTRL;
    unsigned char rEFLG;

    rTXB0CTRL = MCP2510_Read(0x30);
    rTXB1CTRL = MCP2510_Read(0x40);
    rTXB2CTRL = MCP2510_Read(0x50);
    rCANINTF = MCP2510_Read(0x2C);
    rCANCTRL = MCP2510_Read(0x0f);
    rEFLG = MCP2510_Read(0x2D);
    rCANSTAT = MCP2510_Read(0x0E);

    PLOG("\n[%s-%d]: rCANCTRL = 0x%x, rCANINTF = 0x%x, rCANSTAT = 0x%x, rEFLG = 0x%x, rTXB0CTRL = 0x%x, rTXB1CTRL = 0x%x, rTXB2CTRL = 0x%x\n", 
            str, __LINE__, 
            rCANCTRL, 
            rCANINTF, 
            rCANSTAT,
            rEFLG,
            rTXB0CTRL,
            rTXB1CTRL,
            rTXB2CTRL);
#endif
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
    printk("[%s-%d]CmdA: PhysAddr: 0x%x,0x%x,0x%x,0x%x Opcode: 0x%x CRC: 0x%x \n",
            string, lineNo,
            pCmdB->ucPhysAddr[0],
            pCmdB->ucPhysAddr[1],
            pCmdB->ucPhysAddr[2],
            pCmdB->ucPhysAddr[3],
            pCmdB->ucOpcode,

            pCmdB->usCRC
            );
    printIdMsg(string, lineNo, &(pCmdB->stIdmsg));
}

void printCmdC(const char *string, int lineNo, struct DevCmdC_s* pCmdC)
{
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
}

static int cmpCanseq(void* addr1, void* addr2)
{
    return (int)((*(unsigned char *)addr1) - (*(unsigned char *)addr2));
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
static irqreturn_t proc_Interrupt(int irq, void *dev_ID)
{
    unsigned char intrNum;
    //unsigned char revAddr;
    unsigned char uMes;
    //struct CANBus_Message *revMsg;
    struct RcvCanFrm_s *pRcvMsg;
    //int ret;

    spin_lock(&canLock);
    intrNum = MCP2510_Read(0x0E) & 0x0E;            //只有那些被中断使能的中断才会被反映在ICOD位中。得到触发中断的接收缓冲器寄存器
    if(intrNum)
    {
        //revMsg = kzalloc(sizeof(struct CANBus_Message), GFP_ATOMIC);
        pRcvMsg = kzalloc(sizeof(struct RcvCanFrm_s ), GFP_ATOMIC);   /** 从中断处理和进程上下文之外的其他代码中分配内存. 从不睡眠 **/
#if 0
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
        revMsg->SID = ioread8((void *)SPRDAT0) << 3;
        iowrite8(0xFF, (void *)SPTDAT0);
        while(!SPI0_READY);
        uMes = ioread8((void *)SPRDAT0);
        revMsg->SID |= (uMes >> 5);
        revMsg->EID = ((uMes << 16) & 0x3ffff);
        if(!(uMes & 0x08))
        {
            revMsg->RTR = (uMes >> 4) & 0x1;
        }
        iowrite8(0xFF, (void *)SPTDAT0);
        while(!SPI0_READY);
        revMsg->EID |= (ioread8((void *)SPRDAT0) << 8);
        iowrite8(0xFF, (void *)SPTDAT0);
        while(!SPI0_READY);
        revMsg->EID |= ioread8((void *)SPRDAT0);
        iowrite8(0xFF, (void *)SPTDAT0);
        while(!SPI0_READY);
        revAddr =  ioread8((void *)SPRDAT0);
        if(uMes & 0x08)
        {
            revMsg->RTR = (revAddr >> 6) & 0x1;
        }
        revMsg->Length = revAddr & 0x0F;
        if (revMsg->Length > 8)
        {
            PLOG("[%s-%d]: Message Length is:%u \n", __func__, __LINE__,  revMsg->Length);
            revMsg->Length = 8;
        }
/** read data **/
        for(uMes = 0; uMes < revMsg->Length; uMes++)
        {
            iowrite8(0xFF, (void *)SPTDAT0);
            while(!SPI0_READY);
            revMsg->Messages[uMes] = ioread8((void *)SPRDAT0);
        }
        while(!SPI0_READY);
        MCP2510_CS_H;
/** Clear interrupt flag **/
        uMes = (intrNum >> 1) - 5;               
        MCP2510_BitModi(0x2C, uMes, 0);

        ret = kfifo_put(pRcvBuff, (unsigned char *)revMsg, sizeof(struct CANBus_Message));  //将数据写入缓冲区
        PLOG("[%s-%d]: Message Length is: 0x%x, \tSID is: %u, EID is: 0x%x, RTR is: 0x%x\n revMessage is: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", 
                __func__, __LINE__, 
                revMsg->Length, 
                revMsg->SID, 
                revMsg->EID, 
                revMsg->RTR, 
                revMsg->Messages[0], 
                revMsg->Messages[1], 
                revMsg->Messages[2], 
                revMsg->Messages[3], 
                revMsg->Messages[4], 
                revMsg->Messages[5], 
                revMsg->Messages[6], 
                revMsg->Messages[7]);
#else
        //__mcp2510Pkt_read(revMsg, intrNum);
        __mcp2510Pkt_read(&(pRcvMsg->stRcvCanFrm), intrNum);
        spin_unlock(&canLock);
        if(0 != (pRcvMsg->stRcvCanFrm.Messages[0] & 0x80))
        {
            printk("[%s-%d]Info: ... RcvFrm.Messages[0] = 0x%x received self data or error !!! \n", __func__, __LINE__, pRcvMsg->stRcvCanFrm.Messages[0]);
            return IRQ_RETVAL(IRQ_HANDLED);
        }

        RcvCanFrm_insert(&rcvDataRoot_rb, pRcvMsg, cmpCanseq);

        if(waitqueue_active(&wqCanBuff))
        {
            wake_up(&wqCanBuff);
        }

#endif
        //kfree(revMsg);                    //释放内存
    }
    else
    {
        uMes = MCP2510_Read(0x2C) & 0x1C; //清除发送中断
        MCP2510_BitModi(0x2C, uMes, 0);
        spin_unlock(&canLock);
    }

    return IRQ_RETVAL(IRQ_HANDLED);
}

static int open_mcp2510(struct inode *inode, struct file *filp)
{
    int result;

    result = spin_trylock(&canLock);
    if(spiOpened)
    {
        result = -EBUSY;
    }
    else
    {
        u8 tmp;

        MCP2510_BitModi(0x0F, 0xE0, 0x80); //设置MCP2510为配置模式

        //注册接收中断服务程序
        if(request_irq(IRQ_EINT21, proc_Interrupt, IRQF_SAMPLE_RANDOM | IRQF_TRIGGER_LOW, "MCP2510", NULL))
        {
            result = -EFAULT;
        }
        else
        {
            MCP2510_Write(0x2B, 0x03);      //设置(RX1IE, EX0IE)中断使能寄存器, 使能接收缓冲中断

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

            //设置发送控制寄存器
            MCP2510_BitModi(TXBnCTRL[0], 0x0B, 0x03); //设置TXB0CTRL -发送缓冲器0有最高优先级
            MCP2510_BitModi(TXBnCTRL[1], 0x0B, 0x02); //设置TXB1CTRL -发送缓冲器1有次高优先级
            MCP2510_BitModi(TXBnCTRL[2], 0x0B, 0x01); //设置TXB2CTRL -发送缓冲器2有低优先级

            MCP2510_BitModi(0x60, 0x64, 0x04); //设置RXB0CTRL -接收缓冲器0控制寄存器－
            //接收符合滤波器条件的所有带扩展标识符或标准标识符的有效报文
            //如果RXB0 满， RXB0 接收到的报文将被滚存至RXB1
            MCP2510_BitModi(0x70, 0x60, 0x00); //设置RXB1CTRL -接收缓冲器 1 控制寄存器－
            //接收符合滤波器条件的所有带扩展标识符或标准标识符的有效报文

            MCP2510_Write(0x2A, 0x02);    //设置CNF1
            MCP2510_Write(0x29, 0x9E);    //设置CNF2
            MCP2510_Write(0x28, 0x03);    //设置CNF3，设置波特率为125Kbps/s(1, 7, 4, 4)

            MCP2510_Write(0x2C, 0x0);      //清空中断标志
            MCP2510_BitModi(0x0F, 0xE0, 0x00); //设置MCP2510为正常模式
            spiOpened = 1;
            result = 0;
        }
    }
    spin_unlock(&canLock);

    return result;
}

static int release_mcp2510(struct inode *inode, struct file *filp)
{
    disable_irq(IRQ_EINT21);

    spin_lock(&canLock);
    spiOpened = 0;
    free_irq(IRQ_EINT21, NULL);
    kfifo_reset(pRcvBuff);      //清空环形缓存
    kfifo_reset(pSndBuff);      //清空环形缓存
    spin_unlock(&canLock);
    return 0;
}

/**
 * Note: read operation get data from fifo and store into user buf, not read/write mcp2510
 *
 */
static ssize_t read_from_mcp2510(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
#define MRecvSEQ (recMsg->Messages[3])
#define MTrueLENGTH (recMsg->Messages[1])
    int i = 0;
    int result = 0;
    struct rb_node *node_rb;
    struct rb_node *start_rb;
    union DevCmd_s *unCmd;
    struct CANBus_Message *recMsg = NULL;
    if(sizeof(union DevCmd_s) > size)
    {
        printk("[%s-%d]: buf size not enough !!! \n", __func__, __LINE__);
        result = -EINVAL;
    }

    while(!kfifo_len(pRcvBuff))
    {
        start_rb = NULL;
        if(NULL != (node_rb = RcvCanFrm_complete(&rcvDataRoot_rb)))
        {
            recMsg = &((rb_entry(node_rb, struct RcvCanFrm_s, node_rb))->stRcvCanFrm);
            if(0 != recMsg->Messages[3])
            {
                printk("[%s-%d]Warning: start seq is not 0!!! \n", __func__, __LINE__);
                printCanBusMsg(__func__, __LINE__, recMsg);               /** ????????????  **/
            }

            i = 0;
            start_rb = node_rb;    /* the first node need to do */
            for(/** Nothing **/; node_rb; node_rb = rb_next(node_rb))
            {
                recMsg = &((rb_entry(node_rb, struct RcvCanFrm_s, node_rb))->stRcvCanFrm);
                result = kfifo_put(pRcvBuff, (unsigned char *)recMsg, sizeof(struct CANBus_Message));   /** 将数据写入缓冲区. 此一定是有序的 **/
                //printCanBusMsg(__func__, __LINE__, recMsg);               /** ????????????  **/

                i++;        /** 已处理的包的个数 **/
                if(MTrueLENGTH <= (i << 2))  /** received over or not **/
                {
                    break;
                }
            }
            spin_lock_irq(&canLock);
            result = RcvCanFrm_erase(&rcvDataRoot_rb, start_rb, i);
            if(i != result)
            {
                printk("[%s-%d]Warning: RcvCanFrm_erase: %d node should be delete, but %d node deleted!!! \n", __func__, __LINE__, i , result);
            }
            spin_unlock_irq(&canLock);
        }

        if (filp->f_flags & O_NONBLOCK)
        {
            return -EAGAIN;
        }
        else if (wait_event_interruptible(wqCanBuff, (RcvCanFrm_complete(&rcvDataRoot_rb)) || kfifo_len(pRcvBuff)))
        {
            PLOG("[%s-%d]: wait_event_interruptible !!! \n", __func__, __LINE__);
            return -ERESTARTSYS;
        }
    }

    recMsg = kzalloc(size, GFP_KERNEL);
    if(recMsg == NULL)
    {
        result = -EFAULT;
        goto read_err1;
    }

    unCmd = kzalloc(sizeof(union DevCmd_s), GFP_KERNEL);
    if(unCmd == NULL)
    {
        result = -EFAULT;
        goto read_err2;
    }


    /** first canFrame **/
    kfifo_get(pRcvBuff, (unsigned char *)recMsg, sizeof(struct CANBus_Message));
    if(0 != MRecvSEQ)
    {
        printk("[%s-%d]Warn: sequeue[0] error! \n", __func__, __LINE__);
        printCanBusMsg(__func__, __LINE__, recMsg);    /** ????????????  **/
        kfifo_reset(pRcvBuff);      /** 扔掉剩下的 **/
        result = -EFAULT;
        goto read_err3;
    }

    for(i = 0; i < 4; i++)
    {
        unCmd->stDevCmdA.ucPhysAddr[i] = recMsg->Messages[4 + i];
    }
    /** second canFrame **/
    kfifo_get(pRcvBuff, (unsigned char *)recMsg, sizeof(struct CANBus_Message));
    if(1 != MRecvSEQ)
    {
        printk("[%s-%d]Warn: sequeue[1] error!!!!!!!!!!!!! \n", __func__, __LINE__);
        printCanBusMsg(__func__, __LINE__, recMsg);    /** ????????????  **/
    }
    unCmd->stDevCmdA.ucOpcode = recMsg->Messages[4];
    if(MIsCmdA(unCmd->stDevCmdA.ucOpcode))
    {
        unCmd->stDevCmdA.ucAttr = recMsg->Messages[5]; 
        unCmd->stDevCmdA.usValue = (recMsg->Messages[6] | ((recMsg->Messages[7] << 8) & 0xff00)); 

        /** third canFrame **/
        kfifo_get(pRcvBuff, (unsigned char *)recMsg, sizeof(struct CANBus_Message));
        unCmd->stDevCmdA.usCRC = (recMsg->Messages[4] | ((recMsg->Messages[5] << 8) & 0xff00)); 

        if(copy_to_user(buf, (unsigned char *)&(unCmd->stDevCmdA), sizeof(struct DevCmdA_s) /** 10 **/ ))
        {
            printk("[%s-%d]: copy_to_user() failed ! \n", __func__, __LINE__);
        }
        printCmdA(__func__, __LINE__, &(unCmd->stDevCmdA));
        kfifo_reset(pRcvBuff);      /** 扔掉剩下的 **/
        result = sizeof(struct DevCmdA_s);
    }
    else if(MIsCmdC(unCmd->stDevCmdC.ucOpcode))
    {
        unCmd->stDevCmdC.ucQuery = recMsg->Messages[5]; 
        unCmd->stDevCmdC.usCRC = (recMsg->Messages[6] | ((recMsg->Messages[7] << 8) & 0xff00)); 
        if(copy_to_user(buf, (unsigned char *)&(unCmd->stDevCmdA), sizeof(struct DevCmdC_s)/** 8 **/))
        {
            printk("[%s-%d]: copy_to_user() failed ! \n", __func__, __LINE__);
        }

        printCmdC(__func__, __LINE__, &(unCmd->stDevCmdC));
        kfifo_reset(pRcvBuff);      /** 扔掉剩下的 **/
        result = sizeof(struct DevCmdC_s);
    }
    else if(MIsCmdB(unCmd->stDevCmdB.ucOpcode))
    {
        //printCmdB(__func__, __LINE__, &(unCmd->stDevCmdB));
        printk("[%s-%d]Warn: cmdB received! \n", __func__, __LINE__);
    }
    else
    {
        printk("[%s-%d]: cmd error! \n", __func__, __LINE__);
        result = -EFAULT;
    }

#undef MRecvSEQ
#undef MTrueLENGTH
read_err3:
    kfree(unCmd);
read_err2:
    kfree(recMsg);
read_err1:
    return result;
}
/** 发一个帧 **/
static ssize_t write_to_mcp2510(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    int result = 0;
    unsigned char reg;
    struct CANBus_Message *SendMsg;
    int iCnt;

    if(size != sizeof(struct CANBus_Message))
    {
        return -EINVAL;
    }

    //mdelay(1);  /** !!!!!!!!!!!!!!!!!!!!! **/
    //udelay(100);  /** !!!!!!!!!!!!!!!!!!!!! **/
    result = spin_trylock_irq(&canLock);
    if(!result) 
    {
       printk("[%s-%d] Unable to catch spinLock\n", __func__, __LINE__);
       result = -EBUSY;
    }

    SendMsg = kzalloc(size, GFP_KERNEL);
    if(NULL == SendMsg)
    {
        spin_unlock_irq(&canLock);
        printk("[%s-%d] Unable to alloc memory\n", __func__, __LINE__);
        return -ENOMEM;
    }

    if(copy_from_user((char *)SendMsg, buf, size))
    {
        spin_unlock_irq(&canLock);
        kfree(SendMsg);
        return -EFAULT;
    }
    //读三个发送缓冲器寄存器，以确定哪个发送缓冲器可用。
    for(iCnt = 0; iCnt < 100; iCnt++)
    {
        for(result = 0; result < 3; result++)
        {
            if(!(MCP2510_Read(TXBnCTRL[result]) & 0x08))
                break;
        }
        if(result < 3)
        {
            break;
        }
    }
    //三个缓冲器都不可用，返回错误
    if(result == 3)
    {
        spin_unlock_irq(&canLock);
        kfree(SendMsg);
        printk("[%s-%d]: 3 TX-buffer are busy \n", __func__, __LINE__);
        return -EBUSY;
    }
    PLOG("[%s-%d]: Message Length is:%u, \tSID is:%u, EID is:%u, RTR is:%u\n sndMessage is: %u, %u, %u, %u, %u, %u, %u, %u\n", 
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
    iowrite8(TXBnCTRL[result] + 1, (void *)SPTDAT0);
    while(!SPI0_READY);
    reg = SendMsg->SID >> 3;
    iowrite8(reg, (void *)SPTDAT0);
    while(!SPI0_READY);
    reg = (unsigned char)(SendMsg->SID << 5);
    reg =reg |(SendMsg->EID >> 16);
    if(SendMsg->EID)
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
        SendMsg->Length = 8;
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

    MCP2510_BitModi(TXBnCTRL[result], 0x08, 0x08);
    kfree(SendMsg);
    result = 0;
    spin_unlock_irq(&canLock);

    return SendMsg->Length + 5 ;
}

static int ioctl_mcp2510(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int result;

    result = spin_trylock_irq(&canLock);
    if(!result)
    {
        return -EBUSY;
    }

    result = 0;
    if(cmd == 0)
    {    //设置MCP2510为正常模式或环回模式
        if(arg == 1)
        {    //设置为正常模式
            MCP2510_BitModi(0x0F, 0xE0, 0x00);
        }
        else if(arg == 0)
        {   //环回模式
            MCP2510_BitModi(0x0F, 0xE0, 0x40);
        }
        else
        {
            result = -EINVAL;
        }
    }
    else if(cmd == 1) 
    {  //设置CAN总线的波特率
        MCP2510_BitModi(0x0F, 0xE0, 0x80); //设置MCP2510为配置模式
        switch(arg)
        {
            case 0:
                MCP2510_Write(0x2A, 0x00); //设置CNF1
                MCP2510_Write(0x29, 0x91); //设置CNF2
                MCP2510_Write(0x28, 0x01); //设置CNF3，设置波特率为1Mbps/s(1, 3, 2, 2)
                break;
            case 1:
                MCP2510_Write(0x2A, 0x00); //设置CNF1
                MCP2510_Write(0x29, 0x9E); //设置CNF2
                MCP2510_Write(0x28, 0x03); //设置CNF3，设置波特率为500Kbps/s(1, 7, 4, 4)
                break;
            case 2:
                MCP2510_Write(0x2A, 0x01); //设置CNF1
                MCP2510_Write(0x29, 0x9E); //设置CNF2
                MCP2510_Write(0x28, 0x03); //设置CNF3，设置波特率为250Kbps/s(1, 7, 4, 4)
                break;
            case 4: 
                MCP2510_Write(0x2A, 0x02); //设置CNF1
                MCP2510_Write(0x29, 0x9E); //设置CNF2
                MCP2510_Write(0x28, 0x03); //设置CNF3，设置波特率为125Kbps/s(1, 7, 4, 4)
                break;
            default:
                result = -EINVAL;
                break;
        }
        MCP2510_BitModi(0x0F, 0xE0, 0x00); //设置MCP2510为正常模式
    }
    else if (cmd == 2)
    { //设备接收过滤寄存器
        /* ... ... */
    }
    else if (cmd == 0xC0)
    {
        /* ... 设备复位 ... */
        MCP2510_reset();
    }
    else
    {
        result = -EINVAL;
    }
    spin_unlock_irq(&canLock);

    return result;
}

static struct file_operations dev_fops = {
    .owner = THIS_MODULE,
    .read = read_from_mcp2510,
    .write = write_to_mcp2510,
    .ioctl = ioctl_mcp2510,
    .open = open_mcp2510,
    .release = release_mcp2510,
};

static struct miscdevice mcp2510Device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "mcp2510Dev",
    .fops = &dev_fops,
};

static int  __init mcp2510Dev_init(void)
{
    volatile unsigned int *REGISTER;
    int result = 0; 

    pRcvBuff = kfifo_alloc((sizeof(struct CANBus_Message) << 4),  GFP_KERNEL,  &rcvfifoLock);  //分配环形缓存(receive)
    if (pRcvBuff == NULL) 
    {
        printk("kfifo_alloc failed !\n");
        return -EFAULT;
    }

    pSndBuff = kfifo_alloc((sizeof(struct CANBus_Message) << 4),  GFP_KERNEL,  &sndfifoLock);  //分配环形缓存(send)
    if (pSndBuff == NULL) 
    {
        printk("kfifo_alloc failed !\n");
        return -EFAULT;
    }
    if(request_mem_region(rGPECON, 12, "rGPECON"))
    {
        if (misc_register(&mcp2510Device))
        {
            printk("misc register mcp2510Dev false!\n");
            release_mem_region(rGPECON, 12);
            result = -EFAULT;
        }
        else
        {
            spin_lock_init(&rcvfifoLock);   //Initial receive fifo spinlock 
            spin_lock_init(&sndfifoLock);   //Initial send fifo spinlock 
            spin_lock_init(&canLock);       //Initial mcp2510CAN device spinlock
            init_waitqueue_head(&wqCanBuff); //初始化等待队列
/* GPG(2, 13)*/
            //设置GPG2为输出(底电平重启), GPG13（外部中断号21）为中断
            REGISTER = (unsigned int *)ioremap(rGPGCON, 4);
            PLOG("GPGCON Value is %u\n", ioread32((void *)REGISTER));
            iowrite32((ioread32((void *)REGISTER) & (~((0x03 << 26) | (0x03 << 4)))) | (0b10 << 26) | (0b01 << 4), (void *)REGISTER);
            PLOG("GPGCON Value is %u\n", ioread32((void *)REGISTER));
            iounmap(REGISTER);

            //使能GPG2、13上拉
            REGISTER = (unsigned int *)ioremap(rGPGUP, 4);
            PLOG("GPGUP Value is %u\n", ioread32((void *)REGISTER));
            iowrite32(ioread32((void *)REGISTER) & (~((0x1 << 13) | (0x1 << 2))), (void *)REGISTER);
            PLOG("GPGUP Value is %u\n", ioread32((void *)REGISTER));
            iounmap(REGISTER);

/* reset mcp2510. Is it necessary ? */
            //设置GPG2为高电平，GPG2和MCP2510的RST连接
            REGISTER = (unsigned int *)ioremap(rGPGDAT, 4);
            PLOG("GPGDAT Value is %u\n", ioread32((void *)REGISTER));
            iowrite32(ioread32((void *)REGISTER) | (0x1 << 2), (void *)REGISTER);
            PLOG("GPGDAT Value is %u\n", ioread32((void *)REGISTER));
            iounmap(REGISTER);

            //设置GPG14为输出，SPI_CS信号
            REGISTER = (unsigned int *)ioremap(rGPGCON, 4);
            PLOG("GPBCON Value is %u\n", ioread32((void *)REGISTER));
            iowrite32((ioread32((void *)REGISTER) & (~(0b11 << 28))) | (0b01 << 28), (void *)REGISTER);
            PLOG("GPBCON Value is %u\n", ioread32((void *)REGISTER));
            iounmap(REGISTER);

            //使能GPG14上拉
            REGISTER = (unsigned int *)ioremap(rGPGUP, 4);
            PLOG("GPBUP Value is %u\n", ioread32((void *)REGISTER));
            iowrite32(ioread32((void *)REGISTER) & (~(0x1 << 14)), (void *)REGISTER);
            PLOG("GPBUP Value is %u\n", ioread32((void *)REGISTER));
            iounmap(REGISTER);

            GPGDAT = (unsigned int *)ioremap(rGPGDAT, 4);
            SPTDAT0 = (u8 *)ioremap(rSPTDAT0, 1);
            SPRDAT0 = (u8 *)ioremap(rSPRDAT0, 1);
            SPSTA0 = (u8 *)ioremap(rSPSTA0, 1);

            //上拉GPG14
            PLOG("GPGDAT Value is %u\n", ioread32((void *)GPGDAT));
            MCP2510_CS_H;
            PLOG("GPGDAT Value is %u\n", ioread32((void *)GPGDAT));

            //打开SPI CLK使能
            REGISTER = (unsigned int *)ioremap(rCLKCON, 4);
            PLOG("CLKCON Value is %u\n", ioread32((void *)REGISTER));
            iowrite32((ioread32((void *)REGISTER) | (1 << 18)), (void *)REGISTER);
            PLOG("CLKCON Value is %u\n", ioread32((void *)REGISTER));
            iounmap(REGISTER);

            //设置GPE11、12、13为SPI功能
            REGISTER = (unsigned int *)ioremap(rGPECON, 4);
            PLOG("GPECON Value is %u\n", ioread32((void *)REGISTER));
            iowrite32((ioread32((void *)REGISTER) & (0xF03FFFFF)) | (0b10 << 26) | (0b10 << 24) | (0b10 << 22), (void *)REGISTER);
            PLOG("GPECON Value is %u\n", ioread32((void *)REGISTER));
            iounmap(REGISTER);

            //设置SPI0波特率
            REGISTER = (unsigned int *)ioremap(rSPPRE0, 4);
            PLOG("SPPRE0 Value is %u\n", ioread8((void *)REGISTER));
            iowrite8(0x9, (void *)REGISTER);
            PLOG("SPPRE0 Value is %u\n", ioread8((void *)REGISTER));
            iounmap(REGISTER);

            //设置SPCON0
            REGISTER = (unsigned int *)ioremap(rSPCON0, 4);
            PLOG("SPCON0 Value is %u\n", ioread8((void *)REGISTER));
            iowrite8((0 << 6)|(0 << 5)|(1 << 4)|(1 << 3)|(0 << 2)|(0 << 1)|(0 << 0), (void *)REGISTER);
            PLOG("SPCON0 Value is %u\n", ioread8((void *)REGISTER));
            iounmap(REGISTER);

            //设置SPPIN0
            REGISTER = (unsigned int *)ioremap(rSPPIN0, 4);
            PLOG("SPPIN0 Value is %u\n", ioread8((void *)REGISTER));
            iowrite8(0, (void *)REGISTER);
            PLOG("SPPIN0 Value is %u\n", ioread8((void *)REGISTER));
            iounmap(REGISTER);
        }
    }
    else
    {
        result = -EFAULT;
        PLOG("MCP2510Dev init fault!\n");
    }
    return result;
}

static void __exit mcp2510Dev_exit(void)
{
    kfifo_free(pRcvBuff);
    kfifo_free(pSndBuff);

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

