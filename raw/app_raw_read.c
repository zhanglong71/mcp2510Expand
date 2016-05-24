#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
//#include <types.h>

#include "../huaRainType-base.h"
#include "../huaRain-ioctl.h"
#include "../crc8.c"
#include "../protocol.h"

#define min(x,y) (((x) < (y)) ? (x) : (y)) 
           
static char version[16] = "1.0.0";
static char date[32] = __DATE__" "__TIME__;

/** 
 * 适配器编号及对应的(SID + EID) 
 *
 * Note: 此数据是mcp2510文件中的同名数组的复本
 **/

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

int a2digit(char __ch)
{
    int val = -1;

    if((__ch >= '0') && (__ch <= '9'))
    {
        val = __ch - '0';
    }
    else if((__ch >= 'a') && (__ch <= 'f'))
    {
        val = __ch - 'a' + 10;
    }
    else if((__ch >= 'A') && (__ch <= 'F'))
    {
        val = __ch - 'A' + 10;
    }
    else
    {
        printf("[%s-%d]Warning: argument error!\n", __FILE__, __LINE__);
    }

    return val;
}

void printCanBusMsg(const char* string1, int lineNo, struct CANBus_Message* pCanBusMsg)
{
    printf("[%s-%d]: Length:0x%x, SID:0x%x, EID: 0x%x, RTR: 0x%x\n context: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", 
            string1, lineNo, 
            pCanBusMsg->Length, 
            pCanBusMsg->SID, 
            pCanBusMsg->EID, 
            pCanBusMsg->RTR, 
            pCanBusMsg->Messages[0], 
            pCanBusMsg->Messages[1], 
            pCanBusMsg->Messages[2], 
            pCanBusMsg->Messages[3], 
            pCanBusMsg->Messages[4], 
            pCanBusMsg->Messages[5], 
            pCanBusMsg->Messages[6], 
            pCanBusMsg->Messages[7]);
}


void handler(int intNo)
{
    printf("[%s-%d]Info: exit!\n",
            __func__, __LINE__
            );

    exit(0);
}

/***************************************************************************/
int main(int argc,char *argv[])
{
    int fd;
    //int i;
    int opt;
    int iRet;
    //unsigned char tmp;

    struct CANBus_Message rcvMsg;
    char *filename = "/dev/mcp2510";
    //char *fileVal = NULL;

    memset(&rcvMsg, 0, sizeof(struct CANBus_Message));

#if 1
    while((opt = getopt(argc,argv,"v")) != -1)    
    {   
        switch(opt)
        {   
            case 'v':
                printf("%s version: %s - %s\n", basename(argv[0]), version, date);
                if(argc <= 2)
                {
                    return 0;
                }
                break;

            default:
                break;
        }   
    }   
#endif
    signal(SIGINT,handler);

    fd = open(filename, O_RDWR | O_SYNC);
    if (fd < 0)
    {
        printf("Open %s Failed! err string: %s\n", filename, strerror(errno));
        goto err1;
    }

    iRet = read(fd, (char *)&rcvMsg, sizeof(struct CANBus_Message));
    if(iRet > 0)
    {
        printf("[%s-%d]info: read ok! \n", __func__, __LINE__);
        printCanBusMsg(__func__, __LINE__, &rcvMsg);
    }
    else
    {
        printf("[%s-%d]ioctl: iRet = %d, errno = %d !!! \n", __func__, __LINE__, iRet, errno);
    }

    close(fd);
err1:
//err0:
    return 0;
}

