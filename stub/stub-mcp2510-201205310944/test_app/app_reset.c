#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
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

int main(int args,char *argv[])
{
    int fp;
    int err;
    int iCount;
    int result;

    struct CANBus_Message *SendMsg = malloc(sizeof(struct CANBus_Message));

    SendMsg->EID = 1234;
    SendMsg->SID = 345;
    SendMsg->SRR = 0;
    SendMsg->Length = 8;
    SendMsg->Messages[0] = 'a'; 
    SendMsg->Messages[1] = '1';
    SendMsg->Messages[2] = 'b';
    SendMsg->Messages[3] = '2';
    SendMsg->Messages[4] = 'c';
    SendMsg->Messages[5] = '3';
    SendMsg->Messages[6] = 'd';
    SendMsg->Messages[7] = '4';

    fp = open("/dev/mcp2510Dev", O_RDWR|O_SYNC|O_NONBLOCK);
    if (fp > 0)
    {
        //result = ioctl(fp, 0, 0);     /** 回环模式 **/
        //result = ioctl(fp, 0, 1);     /** 正常模式 **/
        result = ioctl(fp, 0xff, 1);
        if(result) 
        {
            printf("fcntl err:%s\n", strerror(result));
            return -1;
        }

    }

    close(fp);
    
    return  0;
}

