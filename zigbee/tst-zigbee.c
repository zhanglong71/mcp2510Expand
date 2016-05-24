
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <linux/rbtree.h>
#include <linux/sched.h>

#include "../mcp2510.h"

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

static ssize_t CanDevLight_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    int ret;
    struct canbusDevice_s *devInfo = filp->private_data;

    size = sizeof(devInfo->usAttr[1]);
    //size = 2;

    if(filp->f_flags & O_NONBLOCK)
    {
        if(0 != devInfo->ucStatValid)
        {
            ret = copy_to_user(buf, (unsigned char *)&(devInfo->usAttr[0x01]), size);
            return ret;
        }
        else
        {
            return -EAGAIN;
        }
    }
    else
    {   
        while(0 == devInfo->ucStatValid)
        {
#if 1
            printk("[%s-%d]Info: goto sleep and wait for wakeup 111.\n", __func__, __LINE__);
            if(wait_event_interruptible(devInfo->rStatq, 0 != devInfo->ucStatValid))
            {
                printk("[%s-%d]Warn: Alloc device number failed.\n", __func__, __LINE__);
                return -ERESTARTSYS; 
            }
            printk("[%s-%d]Info: goto sleep and wait for wakeup 222.\n", __func__, __LINE__);
#endif
        }
        ret = copy_to_user(buf, (unsigned char *)&(devInfo->usAttr[0x01]), size);
        return ret;
    }
   
    /** can't goto here **/
    return 0;
}

static ssize_t CanDevLight_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{

    int ret;
    unsigned short usTmp;
    struct canbusDevice_s *devInfo = filp->private_data;

    size = sizeof(devInfo->usAttr[1]);
    ret = copy_from_user(&usTmp, buf, size);

    ret =  mcp2510Pkt_write((const char *)&usTmp, size /**ret**/, devInfo);
    if(ret < 0)
    {
        printk("[%s-%d]Warn: mcp2510Pkt_write error ! \n", __func__, __LINE__);
    }

    return ret;
}

static int  __init tstCanDevLight_init(void)
{
    int ret;

    printk("[%s-%d]Warn: Alloc device number failed.\n", __func__, __LINE__);

    return  0;
}

static void __exit tstCanDevLight_exit(void)
{

    unregister_canDev(MTypeNo);
    unregister_chrdev_region(devno, MMaxMinorNum);
    printk("exit !\n");
}

module_init(tstCanDevLight_init);
module_exit(tstCanDevLight_exit);

