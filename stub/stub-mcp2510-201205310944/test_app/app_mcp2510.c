#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
//#include <types.h>
#define min(x,y) (((x) < (y)) ? (x) : (y)) 

/****************************************************************************/
#if 1
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
    unsigned char buff[2048];
    struct CANBus_Message *RcvMsg;
    struct CANBus_Message *SendMsg;

    if(NULL == (RcvMsg = malloc(sizeof(struct CANBus_Message))))
    {
        printf("[%s-%d]: malloc failed !\n", __func__, __LINE__);
        return -1;
    }

    if(NULL == (SendMsg = malloc(sizeof(struct CANBus_Message))))
    {
        printf("[%s-%d]: malloc failed !\n", __func__, __LINE__);
        return -1;
    }

    SendMsg->EID = 1;
    SendMsg->SID = 1;
    SendMsg->SRR = 1;

    fd = open("/dev/mcp2510Dev", O_RDWR | O_SYNC | O_NONBLOCK);
    if (fd < 0)
    {
        printf("Open spiDev Failed! err string:%s\n", strerror(errno));
        return -1;
    }

#if 0
    //result = ioctl(fd, 0, 0);   /** loopback mode  **/
    result = ioctl(fd, 0, 1);   /** normal mode  **/
    if(result) 
    {
        printf("fcntl err:%s\n", strerror(result));
    }
#endif

    while(1)
    {
        /**  **/
        if(SendMsg->EID != 0)
        {
            SendMsg->EID++;
        }
        else if(SendMsg->SID != 0)
        {
            SendMsg->SID++;
        }
        else if(SendMsg->SRR != 0)
        {
            SendMsg->SID++;
        }
        else 
        {
            for(i = 0; i < 8; i++)
            {
                if(SendMsg->Messages[i] != 0xff)
                {
                    SendMsg->Messages[i]++;
                    break;
                }
            }
        }

        /**Write data first**/
        SendMsg->Length = min(strlen(buff), 8);
        memcpy(SendMsg->Messages, buff, SendMsg->Length);
        result = write(fd, (char *)SendMsg, sizeof(struct CANBus_Message));
        if(result < 0)
        {
            err = errno;
            perror("write");
            printf("errno = %d\n", err);
        }
        /**then read data**/
        result = read(fd, (char *)SendMsg, sizeof(struct CANBus_Message));
        if(result != sizeof(struct CANBus_Message))
        {
            printf("[%s-%d]Warning: the number of data write from device is not equal to sizeof(struct CANBus_Message)!!!\n");
        }
        printf("recMsg length: %d, content: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", 
                SendMsg->Length, 
                SendMsg->Messages[0], 
                SendMsg->Messages[1],
                SendMsg->Messages[2],
                SendMsg->Messages[3],
                SendMsg->Messages[4],
                SendMsg->Messages[5],
                SendMsg->Messages[6],
                SendMsg->Messages[7]);
        sleep(0.1);
    }

    close(fd);
    return 0;
}

