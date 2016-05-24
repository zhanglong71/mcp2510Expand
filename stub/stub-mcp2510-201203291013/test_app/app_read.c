#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
//#include <types.h>
#define min(x,y) (((x) < (y)) ? (x) : (y)) 

/****************************************************************************/
#if 0
struct CANBus_Message{
    unsigned int EID:18;
    unsigned int SID:11;
    unsigned int Length:4;
    unsigned int SRR:1;
    unsigned char Messages[8];
};
#else
struct CANBus_Message{
    unsigned int EID;
    unsigned short SID;
    unsigned char Length;
    unsigned char SRR;
    unsigned char Messages[8];
};
#endif
/***************************************************************************/
int main(int argc,char *argv[])
{
    int i;
    int fd;
    int err;
    int result;
    struct CANBus_Message *RcvMsg;

    if(NULL == (RcvMsg = malloc(sizeof(struct CANBus_Message))))
    {
        printf("[%s-%d]: malloc failed !\n", __func__, __LINE__);
        return -1;
    }

    fd = open("/dev/mcp2510Dev", O_RDWR | O_SYNC | O_NONBLOCK);
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
        result = read(fd, (char *)RcvMsg, sizeof(struct CANBus_Message));
        if(result != sizeof(struct CANBus_Message))
        {
            printf("[%s-%d]Warning: the number of data write from device is not equal to sizeof(struct CANBus_Message)!!!\n", __FILE__, __LINE__);
        }
        printf("[%s-%d] SID is: %u, EID is %u, SRR is %u,  recMsg length: %u, content: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
                __FILE__, __LINE__,
                RcvMsg->SID,
                RcvMsg->EID,
                RcvMsg->SRR,
                RcvMsg->Length, 
                RcvMsg->Messages[0], 
                RcvMsg->Messages[1],
                RcvMsg->Messages[2],
                RcvMsg->Messages[3],
                RcvMsg->Messages[4],
                RcvMsg->Messages[5],
                RcvMsg->Messages[6],
                RcvMsg->Messages[7]);
        sleep(1);
    }

    close(fd);
    return 0;
}

