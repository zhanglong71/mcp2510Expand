
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
#define MTypeNo 0x01    /** 类型编号 **/
#define usAttrNo 0x01   /** 读写操作时的属性编号**/

static int CanDevLight_open(struct inode *inode, struct file *filp)
{
    filp->private_data = container_of(inode->i_cdev, struct canbusDevice_s, dev);
    return  0;
}

static int CanDevLight_release(struct inode *inode, struct file *filp)
{
    return  0;
}
/**
 * read from device.
 *
 * try 3 times. if 2 of them response same value. then is see as ok
 *
 **/
static ssize_t CanDevLight_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    int i, j;
    int ret;
    long lRemain;
    //unsigned short usTmp;
    unsigned char ucTmp[16];
    unsigned short usLast = 0xffff;
    struct canbusDevice_s *devInfo = filp->private_data;

    //size = sizeof(devInfo->usAttr[1]);

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
    for(i = 0; i < 5; i++)
    {
        //printk("[%s-%d]Info: goto sleep and wait for wakeup 111.\n", __func__, __LINE__);
        for(j = 0; j < 3; j++)
        {
            //ret = mcp2510Pkt_write(CGETSTATUS, &usTmp, size /**ret**/, devInfo);
            ret = mcp2510Pkt_write(CGETSTATUS, ucTmp, size /**ret**/, devInfo);
            if(ret > 0)
            {
                break;
            }
            printk("[%s-%d]Warn: mcp2510Pkt_write error !.\n", __func__, __LINE__);
            //msleep(10);
        }

        lRemain = wait_event_interruptible_timeout(devInfo->rStatq, (0 != devInfo->ucStatValid), HZ);
        if(0 != lRemain)
        {
            //printk("[%s-%d]Warn: wait_event_interruptible_timeout async signal.\n", __func__, __LINE__);
            if(0 != devInfo->ucStatValid)
            {
                if(usLast == devInfo->usAttr[1])
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
    /** error, can't get data **/
    return 0;
}

static ssize_t CanDevLight_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{

    int i;
    int ret;
    long lRemain;
    //unsigned short usTmp;
    unsigned char ucTmp[16];
    struct canbusDevice_s *devInfo = filp->private_data;

    //size = sizeof(devInfo->usAttr[*ppos]);
    if(size < 1)
    {
        return -EINVAL; 
    }

    if(buf == NULL)
    {
        return -EFAULT; 
    }
    devInfo->ucStatValid = 0;
    //ret = copy_from_user(&usTmp, buf, size);
    ret = copy_from_user(ucTmp, buf, size);
    for(i = 0; i < 3; i++)    /** try up to 3 times **/
    {
        //ret = mcp2510Pkt_write(CSETSTATUS, &usTmp, size /**ret**/, devInfo);
        ret = mcp2510Pkt_write(CSETSTATUS, ucTmp, size /**ret**/, devInfo);
        if(ret < 0)
        {
            printk("[%s-%d]mcp2510Pkt_write: canbus busy !!!\n", __func__, __LINE__);
        }
        //msleep(10);

        lRemain = wait_event_interruptible_timeout(devInfo->rStatq, (0 != devInfo->ucStatValid), HZ);
        if(0 != lRemain)
        {
            //printk("[%s-%d]Warn: wait_event_interruptible_timeout async signal.\n", __func__, __LINE__);
            //if((0 != devInfo->ucStatValid) && (usTmp == devInfo->usAttr[1]))
            if((0 != devInfo->ucStatValid) && (*(unsigned short *)ucTmp == devInfo->usAttr[1]))
            {
                return 2;   /** responsed data okey **/
            }
            return -ERESTARTSYS; 
        }
    }

    return -ERESTARTSYS; 
}

/** Is it used ? **/
static int CanDevLight_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret;
    struct canbusDevice_s *devInfo = filp->private_data;
    struct IdentifyMsg_s *pIdMsg = &(devInfo->stIdMsg);     /** static message **/
    unsigned short *pVarMsg = (devInfo->usAttr);            /** dyncmic message **/
//#define TEST_TYPE   'Z'
#define TEST_TYPE   'Z'
#define READ_STATUS    _IOR(TEST_TYPE, 0, char)
    switch(cmd)
    {
        case READ_STATUS:   /** get data from memory. it is different from read function **/
            ret = copy_to_user((char *)arg, (char *)pVarMsg, sizeof(devInfo->usAttr));
            break;
        default:        /** get ID data from memory **/
            ret = copy_to_user((char *)arg, (char *)pIdMsg, sizeof(struct IdentifyMsg_s));
            break;
    }
    return  ret;
}

loff_t CanDevLight_lseek (struct file *filp, loff_t offset, int whence)
{
    int max_pos = sizeof(struct canbusDevice_s);

    switch (whence) {
        case SEEK_SET:  /** 0 **/
            filp->f_pos = offset;
            break;

        case SEEK_CUR:  /** 1 **/
            filp->f_pos += offset;
            break;

        case SEEK_END:  /** 2 **/
            filp->f_pos = max_pos + offset;
            break;
        default:
            break;
    }
    filp->f_pos = ((filp->f_pos < 0) ? 0 : filp->f_pos);
    filp->f_pos = ((filp->f_pos > max_pos) ? max_pos : filp->f_pos);

    return  filp->f_pos;
}

static struct file_operations CanDevLight_fops = {
    .owner = THIS_MODULE,
    .read = CanDevLight_read,
    .write = CanDevLight_write,
    .llseek = CanDevLight_lseek,
    .ioctl = CanDevLight_ioctl,
    .open = CanDevLight_open,
    .release = CanDevLight_release,
};

static int  __init CanDevLight_init(void)
{
    int ret;
    //unsigned int majorNo;

    //ret = alloc_chrdev_region(&devno, 0, MMaxMinorNum, "huarain");
    ret = alloc_chrdev_region(&devno, 0, MMaxMinorNum, "my dev");
    if (ret) {
        printk("[%s-%d]Warn: Alloc device number failed.\n", __func__, __LINE__);
        return ret;
    }

    register_canDev(devno, MTypeNo, &CanDevLight_fops);

#if 0   /** 将设备的接点挂在cdev链表上, 此一步骤在创建节点时进行 **/
    majorNo = MAJOR(devno);
    my_dev[i].no = MKDEV(majorNo, i);
    cdev_init(&my_dev[i].dev, &my_ops);
    cdev_add(&my_dev[i].dev, my_dev[i].no, 1);
#endif

    return  0;
}

static void __exit CanDevLight_exit(void)
{

    unregister_canDev(MTypeNo);
    unregister_chrdev_region(devno, MMaxMinorNum);
    printk("exit !\n");
}

module_init(CanDevLight_init);
module_exit(CanDevLight_exit);

