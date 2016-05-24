#ifndef _MEMDEV_H_
#define _MEMDEV_H_

//#define PDEBUG
#undef PDEBUG
#ifdef PDEBUG
#define PLOG(fmt,args...) printk(KERN_WARNING "DEBUG:" fmt,##args)
#else
#define PLOG(frm,args...)
#endif

/***********************************************/
#if 0
struct CANBus_Message{
 unsigned int EID:18;
 unsigned int SID:11;
 unsigned int Length:4;
 unsigned int RTR:1;
 unsigned char Messages[8];
};

#else

#if 0

struct CANBus_stdMsg {
    unsigned short SID:11;
    unsigned char RTR:1;
    unsigned char Length:4;
    unsigned char Messages[8];
};

struct CANBus_extMsg {
    unsigned int EID:18;
    unsigned short SID:11;
    unsigned char Length:4;
    unsigned char RTR:1;
    unsigned char Messages[8];
};

struct CANBus_rmtMsg {
    unsigned int EID;
    unsigned short SID;
    unsigned char Length;
    unsigned char RTR;
    unsigned char Messages[8];
};

#endif

struct CANBus_Message{
    unsigned int EID;
    unsigned short SID;
    unsigned char Length;
    unsigned char RTR;
    unsigned char Messages[8];
};
#endif
/***********************************************/

#endif

/**********************************************************
s3c2440与mcp2510之间的连接
SPIMISO0  GPE11
SPIMOSI0  GPE12
SPICLK0   GPE13
SPI CS0   GPB7 changed to GPG14
CAN INT   GPG13
CAN RST   GPG2
**********************************************************/
 
