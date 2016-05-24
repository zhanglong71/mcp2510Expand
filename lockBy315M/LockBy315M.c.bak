
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/rbtree.h>
#include <linux/sched.h>

#include "../mcp2510.h"
#include "../protocol.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zl");

dev_t   devno;
#define MTypeNo_LockBy315M 0x03    /** 类型编号(3 - 315M4Lock) **/
#define usAttrNo 0x01   /** 读写操作时的属性编号**/

static char version[16] = "1.0.0";
static char date[32] = __DATE__" "__TIME__;

//static int x10Device_open(struct inode *inode, struct file *filp)
static int LockBy315MDevice_open(struct inode *inode, struct file *filp)
{
    filp->private_data = container_of(inode->i_cdev, struct canbusDevice_s, dev);
    return  0;
}

//static int x10Device_release(struct inode *inode, struct file *filp)
static int LockBy315MDevice_release(struct inode *inode, struct file *filp)
{
    return  0;
}
/**
 * read from device.
 *
 * try 3 times. if 2 of them response same value. then is see as ok
 *
 **/
#if 1
//static ssize_t x10Device_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
static ssize_t LockBy315MDevice_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    int i;
    int iRet = 0;
    long lRemain;
    //unsigned short usTmp;
    unsigned short usLast = 0xffff;     /** 似乎没用了, 考虑清除 **/
    struct sendCmd_s *pSendCmd;
    struct canbusDevice_s *devInfo = filp->private_data;

    //size = sizeof(devInfo->usAttr[*ppos]);
#if 1
    /** only 1 or 2 byte is allowed **/
    if((size < 1) || (size > 2))
    {
        return -EINVAL; 
    }

    if(buf == NULL)
    {
        return -EFAULT; 
    }
#endif
    /** 新数据在5秒钟内都视为有效数据 **/
    if(time_before(/**(unsigned long)**/(devInfo->ulJiffies + 5 * HZ), jiffies))
    {
        devInfo->ucStatValid = 0;
    }

    if(0 != devInfo->ucStatValid)   /** valid data **/
    {
        copy_to_user(buf, (unsigned char *)&(devInfo->usAttr[0x01]), size);
        return 2;
    }

    pSendCmd = kzalloc(sizeof(struct sendCmd_s), GFP_KERNEL);
    if(pSendCmd == NULL)
    {
        printk("[%s-%d]Warning:  kzalloc() failed !\n", __func__, __LINE__);
        return  -ENOMEM;
    }
    devInfo->ucStatValid = 0;

#if 1
    /** 0..3 dest address **/
    for(iRet = 0; iRet < 4; iRet++)
    {
        pSendCmd->cmd[iRet] = ((struct canbusDevice_s *)devInfo)->stIdMsg.ucPhysAddr[iRet];
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
    /** 10 opCode 0x44 **/
    //pSendCmd->cmd[10] = CRAWCMD;   //cmd;
    pSendCmd->cmd[10] = CGETSTATUS;   //0x41-查状态
    /** 11 attr **/
    pSendCmd->cmd[11] = 0x01;
    /** 12 value7..0 **/
    pSendCmd->cmd[12] = 0x1f;   /** 状态请求L-byte **/
    /** 13 value15..8 **/
    pSendCmd->cmd[13] = 0;      /** 状态请求H-byte **/
    /** 14 reserved **/
    pSendCmd->cmd[14] = 0;
    /** 15 CRC8 **/
    pSendCmd->cmd[15] = makeCrc8(0, pSendCmd->cmd, 15);
    /** 13,14 SID, EID **/

    pSendCmd->len = 16;
    pSendCmd->ucAdapterNo = ((struct canbusDevice_s *)devInfo)->ucAdapterNo;
    pSendCmd->usSID = getAdapterTabAddr()[pSendCmd->ucAdapterNo].usSID;
    pSendCmd->uiEID = getAdapterTabAddr()[pSendCmd->ucAdapterNo].uiEID;
#endif

    for(i = 0; i < 3; i++)  /** try upto 3 times when error **/
    {
        mcp2510Pkt_writeCmdX(pSendCmd, 10);
        //ssleep(2);      /** 数据发出到收到回复，至少2秒. 延时2s，保证传输过程完成 **/ 
        lRemain = wait_event_interruptible_timeout(devInfo->rStatq, (0 != devInfo->ucStatValid), 3 * HZ);
        if(0 != lRemain)
        {
            //printk("[%s-%d]Warn: wait_event_interruptible_timeout async signal.\n", __func__, __LINE__);
            if(0 != devInfo->ucStatValid)
            {
                //if(usLast == devInfo->usAttr[1])
                if(1)
                {
                    copy_to_user(buf, (unsigned char *)&(devInfo->usAttr[0x01]), size);
                    return 2;               /** responsed data okey **/
                }
                usLast = devInfo->usAttr[1];
                devInfo->ucStatValid = 0;
            }
            else    /** other event **/
            {
                printk("[%s-%d]info: read process been waked up, but no valid data received!.\n", __func__, __LINE__);
                return -ERESTARTSYS; 
            }
        }
    }

    //ssleep(2);  /** 延时2s，保证传输过程完成 **/
    kfree(pSendCmd);

    return size;
}
#endif


static int isValid(unsigned short __data)
{
    if((__data >= 1) && (__data <= 0x1f) && (__data & 0x01))
    {
        return  1;
    }

    return 0;
}

//static ssize_t LockBy315MDevice_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
static ssize_t LockBy315MDevice_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    int iRet = 0;
    unsigned short usTmp;
    struct sendCmd_s *pSendCmd;
    struct canbusDevice_s *devInfo = filp->private_data;

    //size = sizeof(devInfo->usAttr[*ppos]);
    if((size < 1) || (size > 2))
    {
        return -EINVAL; 
    }

    copy_from_user(&usTmp, buf, size);
    if(isValid(usTmp) == 0)
    {
        return -EINVAL; 
    }

    if(buf == NULL)
    {
        return -EFAULT; 
    }

    pSendCmd = kzalloc(sizeof(struct sendCmd_s), GFP_KERNEL);
    if(pSendCmd == NULL)
    {
        printk("[%s-%d]Warning:  kzalloc() failed !\n", __func__, __LINE__);
        return  -ENOMEM;
    }
    devInfo->ucStatValid = 0;

#if 1
    /** 0..3 dest address **/
    for(iRet = 0; iRet < 4; iRet++)
    {
        pSendCmd->cmd[iRet] = ((struct canbusDevice_s *)devInfo)->stIdMsg.ucPhysAddr[iRet];
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
    /** 10 opCode 0x44 **/
    //pSendCmd->cmd[10] = CRAWCMD  ;   //cmd;
    pSendCmd->cmd[10] = CSETSTATUS;   //0x42-写数据
    /** 11 attr **/
    pSendCmd->cmd[11] = 0x01;
    /** 12 value7..0 **/
    pSendCmd->cmd[12] = (usTmp & 0xff);
    /** 13 value15..8 **/
    pSendCmd->cmd[13] = ((usTmp >> 8) & 0xff);
    /** 14 reserved **/
    pSendCmd->cmd[14] = 0;
    /** 15 CRC8 **/
    pSendCmd->cmd[15] = makeCrc8(0, pSendCmd->cmd, 15);
    /** 13,14 SID, EID **/

    pSendCmd->len = 16;
    pSendCmd->ucAdapterNo = ((struct canbusDevice_s *)devInfo)->ucAdapterNo;
    pSendCmd->usSID = getAdapterTabAddr()[pSendCmd->ucAdapterNo].usSID;
    pSendCmd->uiEID = getAdapterTabAddr()[pSendCmd->ucAdapterNo].uiEID;
#endif

    mcp2510Pkt_writeCmdX(pSendCmd, 10);

    kfree(pSendCmd);

    //ssleep(2);  /** 延时2s，保证传输过程完成 **/
    ssleep(1);  /** 延时1s，保证传输过程完成 **/
    /** should sleep **/

    return size; 
}

/** Is it used ? **/
#include "../huaRain-ioctl.h"
//static int x10Device_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
static int LockBy315MDevice_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    char *pStr = NULL;
    switch(cmd)
    {
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
                            "x10-module Version: %s \n\t"
                            "compile date time: %s \n\n", 
                            version, 
                            date);
            copy_to_user((char *)arg, pStr, VERSION_STRING_SIZE);

            kfree(pStr);
            break;
        //case READ_STATUS:
        default:            /** get ID data from memory **/
            printk("[%s-%d]: no implement the command !\n", __func__, __LINE__);
            break;
    }

    return  0;
}

static struct file_operations x10Device_fops = {
    .owner = THIS_MODULE,
    .read = LockBy315MDevice_read,
    .write = LockBy315MDevice_write,
    .ioctl = LockBy315MDevice_ioctl,
    .open = LockBy315MDevice_open,
    .release = LockBy315MDevice_release,
};

static int  __init x10Device_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&devno, 0, MMaxMinorNum, "LockBy315M");
    if (ret) {
        printk("[%s-%d]Warn: Alloc device number failed.\n", __func__, __LINE__);
        return ret;
    }

    register_canDev(devno, MTypeNo_LockBy315M, &x10Device_fops);

    return  0;
}

static void __exit x10Device_exit(void)
{

    unregister_canDev(MTypeNo_LockBy315M);
    unregister_chrdev_region(devno, MMaxMinorNum);
    printk("exit !\n");
}

module_init(x10Device_init);
module_exit(x10Device_exit);

