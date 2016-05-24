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

#include "mcp2510.h"

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

static struct kfifo *pRcvBuff;          //CAN�������ݻ�����
static struct kfifo *pSndBuff;          //CAN�������ݻ�����

static spinlock_t rcvfifoLock;          //����CAN�������ݻ�������������
static spinlock_t sndfifoLock;          //����CAN�������ݻ�������������
static spinlock_t canLock;              //����CAN�Ĳ���ͬ��(ʵ�����Ƕ�spi��ͬ��, ����spi�����ĵط���Ҫ�õ�)
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

static void kfifo_check(char* str, int line, struct kfifo* pkfifo) 
{
    if(pkfifo != NULL) 
    {
        printk("[%s-%d]: pkfifo->size = %d\t pkfifo->in = %d\t pkfifo->out = %d\t \n", str, line, pkfifo->size, pkfifo->in, pkfifo->out);
    }
}

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

static irqreturn_t proc_Interrupt(int irq, void *dev_ID)
{
    unsigned char intrNum;
    unsigned char revAddr;
    unsigned char uMes;
    struct CANBus_Message *revMsg;
    int ret;

    spin_lock(&canLock);
    intrNum = MCP2510_Read(0x0E) & 0x0E;            //ֻ����Щ���ж�ʹ�ܵ��жϲŻᱻ��ӳ��ICODλ�С��õ������жϵĽ��ջ������Ĵ���
    if(intrNum)
    {
        revMsg = kzalloc(sizeof(struct CANBus_Message), GFP_ATOMIC);   //���жϴ���ͽ���������֮������������з����ڴ�. �Ӳ�˯��
        revAddr = (intrNum << 3) + 1;               //�õ������жϵĽ��ջ������Ĵ����ĵ�ַ
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

        ret = kfifo_put(pRcvBuff, (unsigned char *)revMsg, sizeof(struct CANBus_Message));  //������д�뻺����
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
        kfree(revMsg);                    //�ͷ��ڴ�
        spin_unlock(&canLock);
        if(waitqueue_active(&wqCanBuff))
        {
            wake_up(&wqCanBuff);
        }
    }
    else
    {
        uMes = MCP2510_Read(0x2C) & 0x1C; //��������ж�
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

        MCP2510_BitModi(0x0F, 0xE0, 0x80); //����MCP2510Ϊ����ģʽ

        //ע������жϷ������
        if(request_irq(IRQ_EINT21, proc_Interrupt, IRQF_SAMPLE_RANDOM | IRQF_TRIGGER_LOW, "MCP2510", NULL))
        {
            result = -EFAULT;
        }
        else
        {
            MCP2510_Write(0x2B, 0x03);      //����(RX1IE, EX0IE)�ж�ʹ�ܼĴ���, ʹ�ܽ��ջ����ж�

            MCP2510_CS_L;                   //���ý������μĴ���RXMn��Ϊ0
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

            //���ý��տ��ƼĴ���
            MCP2510_BitModi(TXBnCTRL[0], 0x0B, 0x03); //����TXB0CTRL -���ͻ�����0��������ȼ�
            MCP2510_BitModi(TXBnCTRL[1], 0x0B, 0x02); //����TXB1CTRL -���ͻ�����1�дθ����ȼ�
            MCP2510_BitModi(TXBnCTRL[2], 0x0B, 0x01); //����TXB2CTRL -���ͻ�����2�е����ȼ�

            MCP2510_BitModi(0x60, 0x64, 0x04); //����RXB0CTRL -���ջ�����0���ƼĴ�����
            //���շ����˲������������д���չ��ʶ�����׼��ʶ������Ч����
            //���RXB0 ���� RXB0 ���յ��ı��Ľ���������RXB1
            MCP2510_BitModi(0x70, 0x60, 0x00); //����RXB1CTRL -���ջ����� 1 ���ƼĴ�����
            //���շ����˲������������д���չ��ʶ�����׼��ʶ������Ч����

            MCP2510_Write(0x2A, 0x02);    //����CNF1
            MCP2510_Write(0x29, 0x9E);    //����CNF2
            MCP2510_Write(0x28, 0x03);    //����CNF3�����ò�����Ϊ125Kbps/s(1, 7, 4, 4)

            MCP2510_Write(0x2C, 0x0);      //����жϱ�־
            MCP2510_BitModi(0x0F, 0xE0, 0x00); //����MCP2510Ϊ����ģʽ
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
    kfifo_reset(pRcvBuff);      //��ջ��λ���
    kfifo_reset(pSndBuff);      //��ջ��λ���
    spin_unlock(&canLock);
    return 0;
}

/**
 * Note: read operation get data from fifo and store into user buf, not read/write mcp2510
 *
 */
static ssize_t read_from_mcp2510(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    int result = 0;
    struct CANBus_Message *recMsg;
    if(sizeof(struct CANBus_Message) != size)
    {
        printk("[%s-%d]: size !!! \n", __func__, __LINE__);
        result = -EINVAL;
    }

    //kfifo_check((char *)__func__, __LINE__, pRcvBuff);
    while(!kfifo_len(pRcvBuff))
    {
        if (filp->f_flags & O_NONBLOCK)
        {
            return -EAGAIN;
        }
        if (wait_event_interruptible(wqCanBuff, kfifo_len(pRcvBuff)))
        {
            PLOG("[%s-%d]: wait_event_interruptible !!! \n", __func__, __LINE__);
            return -ERESTARTSYS;
        }
    }

    recMsg = kzalloc(size, GFP_KERNEL);
    if(recMsg == NULL)
    {
        result = -ENOMEM;
        goto read_err1;
    }
    kfifo_get(pRcvBuff, (unsigned char *)recMsg, size);
    if(copy_to_user(buf, (void *)recMsg, size))
    {
        printk("[%s-%d]: copy_to_user() failed ! \n", __func__, __LINE__);
        result = -EFAULT;
        goto read_err;
    }
    else
    {
        result = size;
    }
    PLOG("[%s-%d]: Message Length is: 0x%x, \tSID is: 0x%x, EID is: 0x%x, RTR is: 0x%x\n fifoMessage is: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", 
                __func__, __LINE__, 
                recMsg->Length, 
                recMsg->SID, 
                recMsg->EID, 
                recMsg->RTR, 
                recMsg->Messages[0], 
                recMsg->Messages[1], 
                recMsg->Messages[2], 
                recMsg->Messages[3], 
                recMsg->Messages[4], 
                recMsg->Messages[5], 
                recMsg->Messages[6], 
                recMsg->Messages[7]);
read_err:
    kfree(recMsg);
read_err1:
    return result;
}
/** ��һ��֡ **/
static ssize_t write_to_mcp2510(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    int result = 0;
    unsigned char reg;
    struct CANBus_Message *SendMsg;

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
    //���������ͻ������Ĵ�������ȷ���ĸ����ͻ��������á�
    for(result = 0; result < 3; result++)
    {
        if(!(MCP2510_Read(TXBnCTRL[result]) & 0x08))
            break;
    }
    //�����������������ã����ش���
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
    {    //����MCP2510Ϊ����ģʽ�򻷻�ģʽ
        if(arg == 1)
        {    //����Ϊ����ģʽ
            MCP2510_BitModi(0x0F, 0xE0, 0x00);
        }
        else if(arg == 0)
        {
            MCP2510_BitModi(0x0F, 0xE0, 0x40);
        }
        else
        {
            result = -EINVAL;
        }
    }
    else if(cmd == 1) 
    {  //����CAN���ߵĲ�����
        MCP2510_BitModi(0x0F, 0xE0, 0x80); //����MCP2510Ϊ����ģʽ
        switch(arg)
        {
            case 0:
                MCP2510_Write(0x2A, 0x00); //����CNF1
                MCP2510_Write(0x29, 0x91); //����CNF2
                MCP2510_Write(0x28, 0x01); //����CNF3�����ò�����Ϊ1Mbps/s(1, 3, 2, 2)
                break;
            case 1:
                MCP2510_Write(0x2A, 0x00); //����CNF1
                MCP2510_Write(0x29, 0x9E); //����CNF2
                MCP2510_Write(0x28, 0x03); //����CNF3�����ò�����Ϊ500Kbps/s(1, 7, 4, 4)
                break;
            case 2:
                MCP2510_Write(0x2A, 0x01); //����CNF1
                MCP2510_Write(0x29, 0x9E); //����CNF2
                MCP2510_Write(0x28, 0x03); //����CNF3�����ò�����Ϊ250Kbps/s(1, 7, 4, 4)
                break;
            case 4: 
                MCP2510_Write(0x2A, 0x02); //����CNF1
                MCP2510_Write(0x29, 0x9E); //����CNF2
                MCP2510_Write(0x28, 0x03); //����CNF3�����ò�����Ϊ125Kbps/s(1, 7, 4, 4)
                break;
            default:
                result = -EINVAL;
                break;
        }
        MCP2510_BitModi(0x0F, 0xE0, 0x00); //����MCP2510Ϊ����ģʽ
    }
    else if (cmd == 2)
    { //�豸���չ��˼Ĵ���
        /* ... ... */
    }
    else if (cmd == 0xC0)
    {
        /* ... �豸��λ ... */
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

    pRcvBuff = kfifo_alloc((sizeof(struct CANBus_Message) << 4),  GFP_KERNEL,  &rcvfifoLock);  //���价�λ���(receive)
    if (pRcvBuff == NULL) 
    {
        printk("kfifo_alloc failed !\n");
        return -EFAULT;
    }

    pSndBuff = kfifo_alloc((sizeof(struct CANBus_Message) << 4),  GFP_KERNEL,  &sndfifoLock);  //���价�λ���(send)
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
            init_waitqueue_head(&wqCanBuff); //��ʼ���ȴ�����
/* GPG(2, 13)*/
            //����GPG2Ϊ���(�׵�ƽ����), GPG13���ⲿ�жϺ�21��Ϊ�ж�
            REGISTER = (unsigned int *)ioremap(rGPGCON, 4);
            PLOG("GPGCON Value is %u\n", ioread32((void *)REGISTER));
            iowrite32((ioread32((void *)REGISTER) & (~((0x03 << 26) | (0x03 << 4)))) | (0b10 << 26) | (0b01 << 4), (void *)REGISTER);
            PLOG("GPGCON Value is %u\n", ioread32((void *)REGISTER));
            iounmap(REGISTER);

            //ʹ��GPG2��13����
            REGISTER = (unsigned int *)ioremap(rGPGUP, 4);
            PLOG("GPGUP Value is %u\n", ioread32((void *)REGISTER));
            iowrite32(ioread32((void *)REGISTER) & (~((0x1 << 13) | (0x1 << 2))), (void *)REGISTER);
            PLOG("GPGUP Value is %u\n", ioread32((void *)REGISTER));
            iounmap(REGISTER);

/* reset mcp2510. Is it necessary ? */
            //����GPG2Ϊ�ߵ�ƽ��GPG2��MCP2510��RST����
            REGISTER = (unsigned int *)ioremap(rGPGDAT, 4);
            PLOG("GPGDAT Value is %u\n", ioread32((void *)REGISTER));
            iowrite32(ioread32((void *)REGISTER) | (0x1 << 2), (void *)REGISTER);
            PLOG("GPGDAT Value is %u\n", ioread32((void *)REGISTER));
            iounmap(REGISTER);

            //����GPG14Ϊ�����SPI_CS�ź�
            REGISTER = (unsigned int *)ioremap(rGPGCON, 4);
            PLOG("GPBCON Value is %u\n", ioread32((void *)REGISTER));
            iowrite32((ioread32((void *)REGISTER) & (~(0b11 << 28))) | (0b01 << 28), (void *)REGISTER);
            PLOG("GPBCON Value is %u\n", ioread32((void *)REGISTER));
            iounmap(REGISTER);

            //ʹ��GPG14����
            REGISTER = (unsigned int *)ioremap(rGPGUP, 4);
            PLOG("GPBUP Value is %u\n", ioread32((void *)REGISTER));
            iowrite32(ioread32((void *)REGISTER) & (~(0x1 << 14)), (void *)REGISTER);
            PLOG("GPBUP Value is %u\n", ioread32((void *)REGISTER));
            iounmap(REGISTER);

            GPGDAT = (unsigned int *)ioremap(rGPGDAT, 4);
            SPTDAT0 = (u8 *)ioremap(rSPTDAT0, 1);
            SPRDAT0 = (u8 *)ioremap(rSPRDAT0, 1);
            SPSTA0 = (u8 *)ioremap(rSPSTA0, 1);

            //����GPG14
            PLOG("GPGDAT Value is %u\n", ioread32((void *)GPGDAT));
            MCP2510_CS_H;
            PLOG("GPGDAT Value is %u\n", ioread32((void *)GPGDAT));

            //��SPI CLKʹ��
            REGISTER = (unsigned int *)ioremap(rCLKCON, 4);
            PLOG("CLKCON Value is %u\n", ioread32((void *)REGISTER));
            iowrite32((ioread32((void *)REGISTER) | (1 << 18)), (void *)REGISTER);
            PLOG("CLKCON Value is %u\n", ioread32((void *)REGISTER));
            iounmap(REGISTER);

            //����GPE11��12��13ΪSPI����
            REGISTER = (unsigned int *)ioremap(rGPECON, 4);
            PLOG("GPECON Value is %u\n", ioread32((void *)REGISTER));
            iowrite32((ioread32((void *)REGISTER) & (0xF03FFFFF)) | (0b10 << 26) | (0b10 << 24) | (0b10 << 22), (void *)REGISTER);
            PLOG("GPECON Value is %u\n", ioread32((void *)REGISTER));
            iounmap(REGISTER);

            //����SPI0������
            REGISTER = (unsigned int *)ioremap(rSPPRE0, 4);
            PLOG("SPPRE0 Value is %u\n", ioread8((void *)REGISTER));
            iowrite8(0x9, (void *)REGISTER);
            PLOG("SPPRE0 Value is %u\n", ioread8((void *)REGISTER));
            iounmap(REGISTER);

            //����SPCON0
            REGISTER = (unsigned int *)ioremap(rSPCON0, 4);
            PLOG("SPCON0 Value is %u\n", ioread8((void *)REGISTER));
            iowrite8((0 << 6)|(0 << 5)|(1 << 4)|(1 << 3)|(0 << 2)|(0 << 1)|(0 << 0), (void *)REGISTER);
            PLOG("SPCON0 Value is %u\n", ioread8((void *)REGISTER));
            iounmap(REGISTER);

            //����SPPIN0
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

