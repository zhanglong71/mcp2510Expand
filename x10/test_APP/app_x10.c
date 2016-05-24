#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
//#include <types.h>

//#include "../mcp2510.h"

#define min(x,y) (((x) < (y)) ? (x) : (y)) 
#define CanAdapterNo  0x01

#ifndef ERESTARTSYS
#define ERESTARTSYS 512
#endif
            

#if 0
void fillCmdA(struct DevCmdA_s* pCmdA)
{
    pCmdA->ucPhysAddr[0] = 0x12;
    pCmdA->ucPhysAddr[1] = 0x34;
    pCmdA->ucPhysAddr[2] = 0x56;
    pCmdA->ucPhysAddr[3] = 0x78;

    pCmdA->ucOpcode = 0x43;
    pCmdA->ucAttr = 0x01;
    pCmdA->usValue = 0x01;

    pCmdA->usCRC = 0x0;
}
#endif

unsigned long ulPassNum;
unsigned long ulFailNum;

void handler(int intNo)
{
    printf("[%s-%d]Info: Total %lu test, %lu psss, %lu failed !\n",
            __func__, __LINE__, 
            ulPassNum + ulFailNum, 
            ulPassNum, 
            ulFailNum
            );

    exit(0);
}

/***************************************************************************/
int main(int argc,char *argv[])
{
    int fd;
    int ret;
    //int err;
    unsigned short rAttr = 0;
    unsigned short wAttr = 0;
    char *filename = "/dev/xlight-1-1";
    //union DevCmd_s cmdFrame;

    if(argc == 2)
    {
        filename = argv[1];
    }
    else if(argc >= 3)
    {
        filename = argv[1];
        wAttr = atoi(argv[2]);
    }
    //fd = open("/dev/xlight-1-1", O_RDWR | O_SYNC);
    fd = open(filename, O_RDWR | O_SYNC);
    if (fd < 0)
    {
        printf("Open %s Failed! err string: %s\n", filename, strerror(errno));
        return -1;
    }

    signal(SIGINT,handler);


    ulFailNum = 0;
    ulPassNum = 0;

    //ret = write(fd, (char *)&wAttr , sizeof(unsigned short));
    ret = write(fd, (char *)&wAttr , 2);
    if(ret == 2)
    {
        if((wAttr == 0x05) || (wAttr == 0x07))   /** 只有指定单元的开关才有查证的必要 **/
        {
            ret = read(fd, (char *)&rAttr , sizeof(unsigned short));
            if(ret == sizeof(unsigned short))
            {
                //if(rAttr == wAttr)
                if((((wAttr == 0x03) || (wAttr == 0x05) || (wAttr == 0x09) || (wAttr == 0x0b)) && (rAttr == 0x1b)) ||     /** set on and responsed on **/
                        (((wAttr == 0x01) || (wAttr == 0x07) || (wAttr == 0x0d) || (wAttr == 0x09) || (wAttr == 0x0b)) && (rAttr == 0x1d)))  /** set off and responsed off **/
                {
                    ulPassNum++;
                    printf("[%s-%d]Info: Ok! rAttr matched wAttr[%u <==> %u]\n", __func__, __LINE__, rAttr, wAttr);
                }
                else
                {
                    ulFailNum++;
                    printf("[%s-%d]Info: Fail! rAttr and wAttr are not match[%u >!!!< %u]\n", __func__, __LINE__, rAttr, wAttr);
                }
            }
            else if(ret < 0)    /** error **/
            {
                if(ERESTARTSYS == errno)
                {
                    /** Try again ulFailNum++; **/
                    printf("[%s-%d]read: async event occured, try again !!! \n", __func__, __LINE__);
                }
                else if(EINVAL == errno)
                {
                    printf("[%s-%d]read: invalid argument !!! \n", __func__, __LINE__);
                }
            }
            else
            {
                ulFailNum++;
                printf("[%s-%d]read: ret = %d, errno = %d !!! \n", __func__, __LINE__, ret, errno);
            }
        }
        else
        {
            printf("[%s-%d]Info: No check status[wAttr == %u]!\n", __func__, __LINE__, wAttr);
        }
    }
    else
    {
        ulFailNum++;
        printf("[%s-%d]write: ret = %d, errno = %d !!! \n", __func__, __LINE__, ret, errno);
    }

    close(fd);
    return 0;
}

