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
    char *filename = "/dev/light-1";
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
    //fd = open("/dev/light-1", O_RDWR | O_SYNC);
    fd = open(filename, O_RDWR | O_SYNC);
    if (fd < 0)
    {
        printf("Open /dev/light-1 Failed! err string: %s\n", strerror(errno));
        return -1;
    }

    signal(SIGINT,handler);


    ulFailNum = 0;
    ulPassNum = 0;

    //ret = write(fd, (char *)&wAttr , sizeof(unsigned short));
    ret = write(fd, (char *)&wAttr , 2);
    if(ret == 2)
    {
        ret = read(fd, (char *)&rAttr , sizeof(unsigned short));
        if(ret == sizeof(unsigned short))
        {
            if(rAttr == wAttr)
            {
                ulPassNum++;
                printf("[%s-%d]Info: rAttr == wAttr[%u - %u]\n", __func__, __LINE__, rAttr, wAttr);
            }
            else
            {
                ulFailNum++;
                printf("[%s-%d]Info: rAttr != wAttr[%u - %u]\n", __func__, __LINE__, rAttr, wAttr);
            }
        }
        else if((ret < 0) && (ERESTARTSYS == errno))
        {
            /** Try again ulFailNum++; **/
            printf("[%s-%d]read: async event occured, try again !!! \n", __func__, __LINE__);
        }
        else
        {
            ulFailNum++;
            printf("[%s-%d]read: ret = %d, errno = %d !!! \n", __func__, __LINE__, ret, errno);
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

