#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <sched.h>
#include <unistd.h>
//#include <types.h>

//#include "../../mcp2510.h"
#include "../../../protocol.h"
#include "../../../crc8.c"

#define min(x,y) (((x) < (y)) ? (x) : (y)) 
#define CanAdapterNo  0x01

/*****************************************************************************
 * R: {0xff, 0xff, 0xff, 0xff, 0x11, 0x01, CRC-B1, CRC-B2}              
 * R: {0x12, 0x34, 0x56, 0x78, 0x21, 0x01, CRC-B1, CRC-B2}
 * W: {0x12, 0x34, 0x56, 0x78, 0x22} 
 *    {0x12, 0x34, 0x56, 0x78, 0x11, 0x22}
 *    {0x33, 0x44, 0x01, 0x00, 0xdc, 0x07}
 *    {0x03, 0x0e, 0x00, 0x00, 0x00, 0x00}
 *    {0x00, 0x00, 0x00, 0x00}
 * W: {0x12, 0x34, 0x56, 0x78, 0x12, 0x00, CRC-B1, CRC-B2} 
 *
 * R: {0x12, 0x34, 0x56, 0x78, 0x31, 0x01, CRC-B1, CRC-B2}
 * W :{0x12, 0x34, 0x56, 0x78, 0x33, 0x01, 0x01, CRC-B1, CRC-B2} 
 *
 * R :{0x12, 0x34, 0x56, 0x78, 0x32, 0x01, 0x01, CRC-B1, CRC-B2} 
 * W :{0x12, 0x34, 0x56, 0x78, 0x33, 0x01, 0x01, CRC-B1, CRC-B2} 
 *
 * R: {0x12, 0x34, 0x56, 0x78, 0x41, 0x01, 0x01, 0x00, CRC-B1, CRC-B2}
 * W: {0x12, 0x34, 0x56, 0x78, 0x43, 0x01, 0x01, 0x00, CRC-B1, CRC-B2}
 *
 * R: {0x12, 0x34, 0x56, 0x78, 0x42, 0x01, 0x01, 0x00, CRC-B1, CRC-B2}
 * W: {0x12, 0x34, 0x56, 0x78, 0x43, 0x01, 0x01, 0x00, CRC-B1, CRC-B2}
 *
 ****************************************************************************/

/****************************************************************************/
#if 1

struct CANBus_Message{
    unsigned int EID;
    unsigned short SID;
    unsigned char Length;
    unsigned char SRR;
    unsigned char Messages[8];
};

/** 静态信息  **/
typedef struct IdentifyMsg_s {
    unsigned char ucPhysAddr[4];    /** 物理地址 **/
    unsigned short usVendorNo;      /** 厂商编号 **/
    unsigned short usProductNo;     /** 产品编号 **/
    unsigned short usDevType;       /** 设备类型 (作用参看deviceType_t数组) **/
    /** 出厂日期 **/
    unsigned short usYear;          /** 0 - 65535 **/
    unsigned char  ucMonth;         /** 1 - 12 **/
    unsigned char  ucDay;           /** 1 - 31 **/
    unsigned char other[8];
}IdMsg_t;

/** 协议中的A类命令 **/
typedef struct DevCmdA_s {
    unsigned char ucPhysAddr[4];
    unsigned char ucOpcode;

    unsigned char ucAttr;
    unsigned short usValue;
    unsigned short usCRC;

//    unsigned short usPointer;   /** As a buffer pointer **/
//    unsigned short usLength;
//    unsigned char *ucpData;
}DevCmdA_t;

/** 协议中的B类命令 **/
typedef struct DevCmdB_s {
    unsigned char ucPhysAddr[4];
    unsigned char ucOpcode;

    struct IdentifyMsg_s stIdmsg;   /** 静态信息，也是身份识别信息 **/
    unsigned short usCRC;

//    unsigned short usPointer;       /** As a buffer pointer **/
}DevCmdB_t;

/** 协议中的C类命令 **/
typedef struct DevCmdC_s {
    unsigned char ucPhysAddr[4];
    unsigned char ucOpcode;

    unsigned char ucQuery;
    unsigned short usCRC;
}DevCmdC_t;

/** recv/send data packet format **/
typedef union DevCmd_s {
    struct DevCmdA_s stDevCmdA;
    struct DevCmdB_s stDevCmdB;
    struct DevCmdC_s stDevCmdC;
}DevCmd_ut;

/** 用于临时整理数据之用 **/
typedef struct CANdev_Message_s {
    union DevCmd_s stDevCmd;
    unsigned short usLength;
    unsigned char ucAdapterNo;
}CANdev_Message_t;

#endif

void printCmdA(const char* string, int lineNo, struct DevCmdA_s* pCmdA)
{
    printf("[%s-%d]CmdA: PhysAddr: 0x%x,0x%x,0x%x,0x%x Opcode: 0x%x Attr: 0x%x Value: 0x%x CRC: 0x%x \n",
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
void fillCmdA(struct DevCmdA_s* pCmdA, unsigned short __Attr, unsigned short __Value)
{
    pCmdA->ucPhysAddr[0] = 0x12;
    pCmdA->ucPhysAddr[1] = 0x34;
    pCmdA->ucPhysAddr[2] = 0x56;
    pCmdA->ucPhysAddr[3] = 0x78;

/** CRC creat **/
    pCmdA->usCRC = makeCrc8(0, &(pCmdA->ucPhysAddr[0]), 4);

    pCmdA->ucOpcode = 0x43;
/** CRC creat **/
    pCmdA->usCRC = makeCrc8(pCmdA->usCRC, (unsigned char *)&(pCmdA->ucOpcode), 1);

    pCmdA->ucAttr = __Attr;
/** CRC creat **/
    pCmdA->usCRC = makeCrc8(pCmdA->usCRC, (unsigned char *)&(pCmdA->ucAttr), 1);

    pCmdA->usValue = __Value;
/** CRC creat **/
    pCmdA->usCRC = makeCrc8(pCmdA->usCRC, (unsigned char *)&(pCmdA->usValue), 2);

    //pCmdA->usCRC = 0x0;
}

void printIdMsg(const char *string, int lineNo, struct IdentifyMsg_s* pIdMsg)
{
    printf("[%s-%d]DevId: PhysAddr: [0x%x,0x%x,0x%x,0x%x] [VNo: 0x%x PNo 0x%x Type: 0x%x] [YMD: %u-%u-%u] [0x%x.%x.%x.%x.%x.%x.%x.%x]\n",
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
    printf("[%s-%d]CmdA: PhysAddr: 0x%x,0x%x,0x%x,0x%x Opcode: 0x%x CRC: 0x%x \n",
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
void fillCmdB(struct DevCmdB_s* pCmdB)
{
    int i;

    pCmdB->ucPhysAddr[0] = 0x12;
    pCmdB->ucPhysAddr[1] = 0x34;
    pCmdB->ucPhysAddr[2] = 0x56;
    pCmdB->ucPhysAddr[3] = 0x78;
/** CRC creat **/
    pCmdB->usCRC = makeCrc8(0, &(pCmdB->ucPhysAddr[0]), 4);

    pCmdB->ucOpcode = 0x22;

/** CRC creat **/
    pCmdB->usCRC = makeCrc8(pCmdB->usCRC, &(pCmdB->ucOpcode), 1);

    for(i = 0; i < 4; i++)
    {
        pCmdB->stIdmsg.ucPhysAddr[i] = pCmdB->ucPhysAddr[i];
    }
/** CRC creat **/
    pCmdB->usCRC = makeCrc8(pCmdB->usCRC, (unsigned char *)&(pCmdB->stIdmsg.ucPhysAddr), 4);

    pCmdB->stIdmsg.usVendorNo = 0x1111;
    pCmdB->usCRC = makeCrc8(pCmdB->usCRC, (unsigned char *)&(pCmdB->stIdmsg.usVendorNo), 2);
    pCmdB->stIdmsg.usProductNo = 0x2222;
    pCmdB->usCRC = makeCrc8(pCmdB->usCRC, (unsigned char *)&(pCmdB->stIdmsg.usProductNo), 2);
    pCmdB->stIdmsg.usDevType = 0x01;
    pCmdB->usCRC = makeCrc8(pCmdB->usCRC, (unsigned char *)&(pCmdB->stIdmsg.usDevType), 2);
    pCmdB->stIdmsg.usYear = 2012;
    pCmdB->usCRC = makeCrc8(pCmdB->usCRC, (unsigned char *)&(pCmdB->stIdmsg.usYear), 2);
    pCmdB->stIdmsg.ucMonth = 3;
    pCmdB->usCRC = makeCrc8(pCmdB->usCRC, (unsigned char *)&(pCmdB->stIdmsg.ucMonth), 1);
    pCmdB->stIdmsg.ucDay = 14;
    pCmdB->usCRC = makeCrc8(pCmdB->usCRC, (unsigned char *)&(pCmdB->stIdmsg.ucDay), 1);

    for(i = 0; i < 8; i++)
    {
        pCmdB->stIdmsg.other[i] = 0;
    }
    pCmdB->usCRC = makeCrc8(pCmdB->usCRC, (unsigned char *)&(pCmdB->stIdmsg.other), 8);
}

void printCmdC(const char *string, int lineNo, struct DevCmdC_s* pCmdC)
{
    printf("[%s-%d]CmdC: PhysAddr: 0x%x,0x%x,0x%x,0x%x Opcode: 0x%x Query 0x%x CRC: 0x%x \n",
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
void fillCmdC(struct DevCmdC_s* pCmdC)
{
    pCmdC->ucPhysAddr[0] = 0x12;
    pCmdC->ucPhysAddr[1] = 0x34;
    pCmdC->ucPhysAddr[2] = 0x56;
    pCmdC->ucPhysAddr[3] = 0x78;

    pCmdC->ucOpcode = 0x12;

    pCmdC->ucQuery = 0x01;
    pCmdC->usCRC = 0x0;

}
/***************************************************************************/
int main(int argc,char *argv[])
{
    //int i;
    int fd;
    int ret;
    //int err;
    int result;
    //struct sched_param param;
    struct CANBus_Message *RcvMsg;
    union DevCmd_s cmdFrame;
    struct timeval tv;
    static unsigned char ucCanseq = 0;      /** canbus上的流水帧号，每发一个帧加1, 每新起一个数据包加3 **/

    unsigned short usAttr[4];

#if 0
    ret = sched_get_priority_max(SCHED_RR);
    if(ret < 0)
    {   
        printf("sched_get_priority_max failed !\n");
    }   

    param.sched_priority = ret;
    ret = sched_setscheduler(getpid(), SCHED_RR, &param);
    if(ret < 0)
    {   
        err = errno;
        perror("sched_setscheduler");
        printf("errno = %d !\n", err);
    } 
#endif

    tv.tv_sec = 0;
    tv.tv_usec = 1000000;

    if(NULL == (RcvMsg = malloc(sizeof(struct CANBus_Message))))
    {
        printf("[%s-%d]: malloc failed !\n", __func__, __LINE__);
        return -1;
    }

    //fd = open("/dev/mcp2510Dev", O_RDWR | O_SYNC | O_NONBLOCK);
    fd = open("/dev/mcp2510Dev", O_RDWR | O_SYNC);
    if (fd < 0)
    {
        printf("Open spiDev Failed! err string:%s\n", strerror(errno));
        return -1;
    }

    //result = ioctl(fd, 0, 0);   /** loopback mode  **/
    result = ioctl(fd, 0, 1);   /** normal mode  **/
    if(result) 
    {
        printf("fcntl err:%s\n", strerror(result));
    }

    while(1)
    {
        /**then read data**/
        //result = read(fd, (char *)RcvMsg, sizeof(struct CANBus_Message));
        result = read(fd, (char *)&cmdFrame, sizeof(union DevCmd_s));
        //if((result == sizeof(struct DevCmdA_s)) && (MIsCmdA(cmdFrame.stDevCmdA.ucOpcode)))
        if((result == sizeof(struct DevCmdA_s)) && (MBaseCMD(cmdFrame.stDevCmdA.ucOpcode)))
        {
            /** CmdA **/
            //printCmdA(__func__, __LINE__, &(cmdFrame.stDevCmdA));

        }
        else if((result == sizeof(struct DevCmdC_s)) && (MIsCmdC(cmdFrame.stDevCmdC.ucOpcode)))
        {
            /** CmdC **/
            //printCmdC(__func__, __LINE__, &(cmdFrame.stDevCmdC));
        }
        else if((result == sizeof(struct DevCmdB_s)) && (MIsCmdB(cmdFrame.stDevCmdB.ucOpcode)))
        {
            /** CmdB **/
            //printCmdB(__func__, __LINE__, &(cmdFrame.stDevCmdB));
        }
        else if(result > 0)
        {
            printf("[%s-%d]Warn: error message received !\n", __FILE__, __LINE__); 
        }

        if(result <= 0)
        {
            printf("[%s-%d]No message received ! %s!\n", __FILE__, __LINE__, strerror(errno)); 
            continue;
        }

        //printf("[%s-%d] SID is: 0x%x, EID is 0x%x, SRR is 0x%x,  recMsg length: 0x%x, content: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", ...);

        if((cmdFrame.stDevCmdB.ucPhysAddr[0] == 0xff)
                && (cmdFrame.stDevCmdB.ucPhysAddr[1] == 0xff)
                && (cmdFrame.stDevCmdB.ucPhysAddr[2] == 0xff)
                && (cmdFrame.stDevCmdB.ucPhysAddr[3] == 0xff))
        {/** 查设备信息的主播 **/
            fillCmdB(&(cmdFrame.stDevCmdB));
            if(ucCanseq > 240)
            {
                ucCanseq = 0;
            }
            else
            {
                ucCanseq  += 3;
            }

            /** start **/
            RcvMsg->SID = 0x123;
            //RcvMsg->SID = 0x02;
            RcvMsg->EID = 0x456;
            //RcvMsg->EID = 0x00;
            RcvMsg->SRR = 0;
            RcvMsg->Length = 8;
            RcvMsg->Messages[0] = (1 | 0x80);
            RcvMsg->Messages[1] = 28;   /** 22+4(PhysAddr)+1(cmd)+1(CRC)  **/
            ucCanseq++;
            RcvMsg->Messages[2] = ucCanseq ;
            RcvMsg->Messages[3] = 0;

            /** first frame**/
            RcvMsg->Messages[4] = cmdFrame.stDevCmdB.ucPhysAddr[0];
            RcvMsg->Messages[5] = cmdFrame.stDevCmdB.ucPhysAddr[1];
            RcvMsg->Messages[6] = cmdFrame.stDevCmdB.ucPhysAddr[2];
            RcvMsg->Messages[7] = cmdFrame.stDevCmdB.ucPhysAddr[3];
            ret = write(fd, (char *)RcvMsg, sizeof(struct CANBus_Message));
            //if(ret != sizeof(struct CANBus_Message))
            if(ret != RcvMsg->Length + 5)
            {
                
                printf("[%s-%d]write: ret = %d, errno = %d !!! \n", __func__, __LINE__, ret, errno);
            }
            //sleep(1);

            /** second frame**/
            ucCanseq++;
            RcvMsg->Messages[2] = ucCanseq ;
            RcvMsg->Messages[3] += 1;
            RcvMsg->Messages[4] = cmdFrame.stDevCmdB.ucOpcode;
            RcvMsg->Messages[5] = cmdFrame.stDevCmdB.stIdmsg.ucPhysAddr[0];
            RcvMsg->Messages[6] = cmdFrame.stDevCmdB.stIdmsg.ucPhysAddr[1];
            RcvMsg->Messages[7] = cmdFrame.stDevCmdB.stIdmsg.ucPhysAddr[2];
            ret = write(fd, (char *)RcvMsg, sizeof(struct CANBus_Message));
            //if(ret != sizeof(struct CANBus_Message))
            if(ret != RcvMsg->Length + 5)
            {
                printf("[%s-%d]write: ret = %d, errno = %d !!! \n", __func__, __LINE__, ret, errno);
            }
            //sleep(1);

            /** third frame**/
            ucCanseq++;
            RcvMsg->Messages[2] = ucCanseq ;
            RcvMsg->Messages[3] += 1;
            RcvMsg->Messages[4] = cmdFrame.stDevCmdB.stIdmsg.ucPhysAddr[3];
            RcvMsg->Messages[5] = cmdFrame.stDevCmdB.stIdmsg.usVendorNo & 0xff;
            RcvMsg->Messages[6] = (cmdFrame.stDevCmdB.stIdmsg.usVendorNo >> 8) & 0xff;
            RcvMsg->Messages[7] = cmdFrame.stDevCmdB.stIdmsg.usProductNo & 0xff;
            ret = write(fd, (char *)RcvMsg, sizeof(struct CANBus_Message));
            //if(ret != sizeof(struct CANBus_Message))
            if(ret != RcvMsg->Length + 5)
            {
                printf("[%s-%d]write: ret = %d, errno = %d !!! \n", __func__, __LINE__, ret, errno);
            }
            //sleep(1);

            /** forth frame**/
            ucCanseq++;
            RcvMsg->Messages[2] = ucCanseq ;
            RcvMsg->Messages[3] += 1;
            RcvMsg->Messages[4] = (cmdFrame.stDevCmdB.stIdmsg.usProductNo >> 8) & 0xff;
            RcvMsg->Messages[5] = cmdFrame.stDevCmdB.stIdmsg.usDevType & 0xff;
            RcvMsg->Messages[6] = (cmdFrame.stDevCmdB.stIdmsg.usDevType >> 8) & 0xff;
            RcvMsg->Messages[7] = cmdFrame.stDevCmdB.stIdmsg.usYear & 0xff;
            ret = write(fd, (char *)RcvMsg, sizeof(struct CANBus_Message));
            //if(ret != sizeof(struct CANBus_Message))
            if(ret != RcvMsg->Length + 5)
            {
                printf("[%s-%d]write: ret = %d, errno = %d !!! \n", __func__, __LINE__, ret, errno);
            }
            //sleep(1);

            /** fifth frame**/
            ucCanseq++;
            RcvMsg->Messages[2] = ucCanseq ;
            RcvMsg->Messages[3] += 1;
            RcvMsg->Messages[4] = (cmdFrame.stDevCmdB.stIdmsg.usYear >> 8) & 0xff;
            RcvMsg->Messages[5] = cmdFrame.stDevCmdB.stIdmsg.ucMonth;
            RcvMsg->Messages[6] = cmdFrame.stDevCmdB.stIdmsg.ucDay;
            RcvMsg->Messages[7] = cmdFrame.stDevCmdB.stIdmsg.other[0];
            ret = write(fd, (char *)RcvMsg, sizeof(struct CANBus_Message));
            //if(ret != sizeof(struct CANBus_Message))
            if(ret != RcvMsg->Length + 5)
            {
                printf("[%s-%d]write: ret = %d, errno = %d !!! \n", __func__, __LINE__, ret, errno);
            }
            //sleep(1);

            /** sixth frame**/
            ucCanseq++;
            RcvMsg->Messages[2] = ucCanseq ;
            RcvMsg->Messages[3] += 1;
            RcvMsg->Messages[4] = cmdFrame.stDevCmdB.stIdmsg.other[1];
            RcvMsg->Messages[5] = cmdFrame.stDevCmdB.stIdmsg.other[2];
            RcvMsg->Messages[6] = cmdFrame.stDevCmdB.stIdmsg.other[3];
            RcvMsg->Messages[7] = cmdFrame.stDevCmdB.stIdmsg.other[4];
            ret = write(fd, (char *)RcvMsg, sizeof(struct CANBus_Message));
            //if(ret != sizeof(struct CANBus_Message))
            if(ret != RcvMsg->Length + 5)
            {
                printf("[%s-%d]write: ret = %d, errno = %d !!! \n", __func__, __LINE__, ret, errno);
            }
            //sleep(1);

            /** seventh frame**/
            ucCanseq++;
            RcvMsg->Messages[2] = ucCanseq ;
            RcvMsg->Messages[3] += 1;
            RcvMsg->Messages[4] = cmdFrame.stDevCmdB.stIdmsg.other[5];
            RcvMsg->Messages[5] = cmdFrame.stDevCmdB.stIdmsg.other[6];
            RcvMsg->Messages[6] = cmdFrame.stDevCmdB.stIdmsg.other[7];
            RcvMsg->Messages[7] = cmdFrame.stDevCmdB.usCRC & 0xff;
            ret = write(fd, (char *)RcvMsg, sizeof(struct CANBus_Message));
            //if(ret != sizeof(struct CANBus_Message))
            if(ret != RcvMsg->Length + 5)
            {
                printf("[%s-%d]write: ret = %d, errno = %d !!! \n", __func__, __LINE__, ret, errno);
            }
            //sleep(1);
#if 0
            /** eigth frame**/
            ucCanseq++;
            RcvMsg->Messages[2] = ucCanseq ;
            RcvMsg->Messages[3] += 1;
            RcvMsg->Messages[4] = (cmdFrame.stDevCmdB.usCRC >> 8) & 0xff;
            RcvMsg->Length = 5;
            ret = write(fd, (char *)RcvMsg, sizeof(struct CANBus_Message));
            //if(ret != sizeof(struct CANBus_Message))
            if(ret != RcvMsg->Length + 5)
            {
                printf("[%s-%d]write: ret = %d, errno = %d !!! \n", __func__, __LINE__, ret, errno);
            }
            //sleep(1);

            RcvMsg->Messages[3] += 1;
#endif
        }
        else if(cmdFrame.stDevCmdC.ucOpcode == 0x11)
        {
            fillCmdC(&(cmdFrame.stDevCmdC));

            if(ucCanseq > 240)
            {
                ucCanseq = 0;
            }
            else
            {
                ucCanseq  += 3;
            }

            /** start **/
            /** RcvMsg->SID = (0b001 | (0x12 << 3)); **/
            RcvMsg->SID = 0x123;
            RcvMsg->EID = 0x456;
            RcvMsg->SRR = 0;
            RcvMsg->Length = 8;
            RcvMsg->Messages[0] = (1 | 0x80) ;    /** can.Adapter.No and S=>H **/
            RcvMsg->Messages[1] = 8;    /** length  **/
            ucCanseq++;
            RcvMsg->Messages[2] = ucCanseq ;
            RcvMsg->Messages[3] = 0;    /** sequence **/

            RcvMsg->Messages[4] = cmdFrame.stDevCmdC.ucPhysAddr[0];
            RcvMsg->Messages[5] = cmdFrame.stDevCmdC.ucPhysAddr[1];
            RcvMsg->Messages[6] = cmdFrame.stDevCmdC.ucPhysAddr[2];
            RcvMsg->Messages[7] = cmdFrame.stDevCmdC.ucPhysAddr[3];
            ret = write(fd, (char *)RcvMsg, sizeof(struct CANBus_Message));
            if(ret != RcvMsg->Length + 5)
            {
                printf("[%s-%d]write: ret = %d, errno = %d !!! \n", __func__, __LINE__, ret, errno);
            }
            //sleep(1);

            /** second frame**/
            ucCanseq++;
            RcvMsg->Messages[2] = ucCanseq ;
            RcvMsg->Messages[3] += 1;
            RcvMsg->Messages[4] = cmdFrame.stDevCmdC.ucOpcode;
            RcvMsg->Messages[5] = cmdFrame.stDevCmdC.ucQuery;
            RcvMsg->Messages[6] = cmdFrame.stDevCmdC.usCRC & 0xff;
            RcvMsg->Messages[7] = (cmdFrame.stDevCmdC.usCRC >> 8) & 0xff;
            ret = write(fd, (char *)RcvMsg, sizeof(struct CANBus_Message));
            if(ret != RcvMsg->Length + 5)
            {
                printf("[%s-%d]write: ret = %d, errno = %d !!! \n", __func__, __LINE__, ret, errno);
            }
            //sleep(1);
        }
        else if((cmdFrame.stDevCmdA.ucOpcode == 0x41) || (cmdFrame.stDevCmdA.ucOpcode == 0x42))
        {
            if((cmdFrame.stDevCmdA.ucAttr > 4) || (cmdFrame.stDevCmdA.ucAttr < 0))
            {
                printf("[%s-%d]Warning: Attr NO. = %u error\n", __func__, __LINE__, cmdFrame.stDevCmdA.ucAttr);
            }
            else
            {
#if 0
                printf("[%s-%d]Info: Received Opcode = 0x%x, Attr = 0x%x, Value = 0x%x \n",
                        __func__, __LINE__,
                        cmdFrame.stDevCmdA.ucOpcode,      
                        cmdFrame.stDevCmdA.ucAttr,      
                        cmdFrame.stDevCmdA.usValue      
                        );
#endif
            }

            usAttr[cmdFrame.stDevCmdA.ucAttr] = cmdFrame.stDevCmdA.usValue;
            fillCmdA(&(cmdFrame.stDevCmdA), cmdFrame.stDevCmdA.ucAttr, usAttr[cmdFrame.stDevCmdA.ucAttr]);
            if(ucCanseq > 240)
            {
                ucCanseq = 0;
            }
            else
            {
                ucCanseq  += 3;
            }

            /** start **/
            RcvMsg->SID = 0x123;
            RcvMsg->EID = 0x456;
            RcvMsg->SRR = 0;
            RcvMsg->Length = 8;
            RcvMsg->Messages[0] = (1 | 0x80);    /** can.Adapter.No  **/
            RcvMsg->Messages[1] = 10;   /** length  **/
            ucCanseq++;
            RcvMsg->Messages[2] = ucCanseq ;
            RcvMsg->Messages[3] = 0;    /** sequence **/

            RcvMsg->Messages[4] = cmdFrame.stDevCmdA.ucPhysAddr[0];
            RcvMsg->Messages[5] = cmdFrame.stDevCmdA.ucPhysAddr[1];
            RcvMsg->Messages[6] = cmdFrame.stDevCmdA.ucPhysAddr[2];
            RcvMsg->Messages[7] = cmdFrame.stDevCmdA.ucPhysAddr[3];
            ret = write(fd, (char *)RcvMsg, sizeof(struct CANBus_Message));
            if(ret != RcvMsg->Length + 5)
            {
                printf("[%s-%d]write: ret = %d, errno = %d !!! \n", __func__, __LINE__, ret, errno);
            }
            //sleep(0.6);

            ucCanseq++;
            RcvMsg->Messages[2] = ucCanseq ;
            RcvMsg->Messages[3] += 1;
            /** second frame**/
            RcvMsg->Messages[4] = cmdFrame.stDevCmdA.ucOpcode;
            RcvMsg->Messages[5] = cmdFrame.stDevCmdA.ucAttr;
            RcvMsg->Messages[6] = cmdFrame.stDevCmdA.usValue & 0xff;
            RcvMsg->Messages[7] = (cmdFrame.stDevCmdA.usValue >> 8) & 0xff;
            ret = write(fd, (char *)RcvMsg, sizeof(struct CANBus_Message));
            if(ret != RcvMsg->Length + 5)
            {
                printf("[%s-%d]write: ret = %d, errno = %d !!! \n", __func__, __LINE__, ret, errno);
            }
            //sleep(0.6);

            ucCanseq++;
            RcvMsg->Messages[2] = ucCanseq ;
            RcvMsg->Messages[3] += 1;
            RcvMsg->Length = 6;
            RcvMsg->Messages[4] = cmdFrame.stDevCmdA.usCRC & 0xff;
            RcvMsg->Messages[5] = (cmdFrame.stDevCmdA.usCRC >> 8) & 0xff;
            ret = write(fd, (char *)RcvMsg, sizeof(struct CANBus_Message));
            if(ret != RcvMsg->Length + 5)
            {
                printf("[%s-%d]write: ret = %d, errno = %d !!! \n", __func__, __LINE__, ret, errno);
            }
            //sleep(0.6);

        }
        else
        {
            printf("[%s-%d]Unknow cmd. All data:[0x%x,0x%x,0x%x,0x%x 0x%x 0x%x 0x%x]!!! \n", 
                    __func__, __LINE__, 
                    cmdFrame.stDevCmdA.ucPhysAddr[0], 
                    cmdFrame.stDevCmdA.ucPhysAddr[1], 
                    cmdFrame.stDevCmdA.ucPhysAddr[2], 
                    cmdFrame.stDevCmdA.ucPhysAddr[3],

                    cmdFrame.stDevCmdA.ucOpcode,
                    cmdFrame.stDevCmdA.ucAttr,
                    cmdFrame.stDevCmdA.usValue);
        }

        memset(RcvMsg, 0, sizeof(struct CANBus_Message));
        //sleep(5);
    }

    close(fd);
    return 0;
}

