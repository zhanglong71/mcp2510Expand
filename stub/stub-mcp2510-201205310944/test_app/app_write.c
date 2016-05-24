#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
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

int main(int argc, char* argv[])
{
    int i;
    int fp;
    int err;
    int iCount;
    int result;

    struct CANBus_Message *SendMsg = malloc(sizeof(struct CANBus_Message));

    SendMsg->EID = 1234;
    SendMsg->SID = 345;
    SendMsg->SRR = 0;
    if(argc > 1)
    {
        SendMsg->Length = argc - 1;
        for(iCount = 0; iCount < SendMsg->Length; iCount++)
        {
            SendMsg->Messages[iCount] = argv[iCount + 1][0]; 
        }
    }
    else
    {
        SendMsg->Length = 8;
        SendMsg->Messages[0] = '\0'; 
        SendMsg->Messages[1] = '\0';
        SendMsg->Messages[2] = '\0';
        SendMsg->Messages[3] = '\0';
        SendMsg->Messages[4] = '\0';
        SendMsg->Messages[5] = '\0';
        SendMsg->Messages[6] = '\0';
        SendMsg->Messages[7] = '\0';
    }

    fp = open("/dev/mcp2510Dev", O_RDWR|O_SYNC|O_NONBLOCK);
    if (fp < 0)
    {
        printf("Open spiDev Failed! err string:%s\n", strerror(errno));
        return -1;
    }

    //result = ioctl(fp, 0, 0);     /** 回环模式 **/
    result = ioctl(fp, 0, 1);       /** 正常模式 **/
    if(result) 
    {
        printf("fcntl err:%s\n", strerror(result));
    }

    printf("Now testing! send data every second ...\n");

    iCount = 0;
    while(1)
    {
        iCount++;
        result = write(fp, (char *)SendMsg, sizeof(struct CANBus_Message));
        if(result < 0)
        {
            err = errno;
            perror("write");
            //printf("errno = %d\n", err);
            sleep(5);
        }
        printf("testing iCount = %d , result = %d !\n", iCount, result);
        sleep(1);

        for(i = 0; i < 8; i++)
        {
            if(SendMsg->Messages[i] != 0xff)
            {
                SendMsg->Messages[i]++; 
                break;
            }
        }

        if(i == 8)
            break;

    }

    close(fp);
    return 0;
}

