#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
//#include <types.h>

#include "../mcp2510.h"

#define min(x,y) (((x) < (y)) ? (x) : (y)) 
#define CanAdapterNo  0x01

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

/***************************************************************************/
int main(int argc,char *argv[])
{
    int i;
    int fd;
    int ret;
    //int err;
    unsigned short rAttr;
    unsigned short wAttr;
    union DevCmd_s cmdFrame;

    //fd = open("/dev/mcp2510Dev", O_RDWR | O_SYNC | O_NONBLOCK);
    fd = open("/dev/light-1", O_RDWR | O_SYNC);
    if (fd < 0)
    {
        printf("Open spiDev Failed! err string:%s\n", strerror(errno));
        return -1;
    }
    rAttr = 0;
    wAttr = 0;

    while(1)
    {   
        wAttr++; 

        ret = write(fd, (char *)wAttr , sizeof(unsigned short));
        if(ret <= 0)
        {
            printf("[%s-%d]No message are write wAttr = %d!\n", __FILE__, __LINE__, wAttr); 
            continue;
        }
        if(ret != sizeof(unsigned short))
        {
            printf("[%s-%d]write: ret = %d, errno = %d !!! \n", __func__, __LINE__, ret, errno);
        }
        else    /** Now unsigned short data write ok **/
        {
            ret = read(fd, (char *)rAttr , sizeof(unsigned short));
            if(ret <= 0)
            {
                printf("[%s-%d]No message read rAttr = %d!\n", __FILE__, __LINE__, rAttr); 
                continue;
            }
            if(ret != sizeof(unsigned short))
            {
                perror("read:");
                printf("[%s-%d]Warning: the number of data read from device is %d, not equal to sizeof(struct CANBus_Message)!!!\n", __FILE__, __LINE__, ret);
                //continue;
            }
            else    /** Now unsigned short data read ok **/
            {
                if(rAttr == wAttr)
                {
                    printf("[%s-%d]Info: rAttr == wAttr\n", __func__, __LINE__);
                }
                else
                {
                    printf("[%s-%d]Info: rAttr == wAttr\n", __func__, __LINE__);
                }

            }
        }

        
    }

    close(fd);
    return 0;
}

