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
    int i;
    int opt;
    int iRet;
    unsigned char tmp;
    //unsigned char adapterNo = 0;
    //unsigned char ucType = 0;
    struct rgstInfo_s rgstInfo;
    char *filename = "/dev/mcp2510";
    char *fileVal = NULL;

    memset(&rgstInfo, 0, sizeof(struct rgstInfo_s));

    while((opt = getopt(argc,argv,"a:f:s:t:v")) != -1)    
    {   
        switch(opt)
        {   
            case 'a':               /** address **/
                fileVal = optarg;
                iRet = strlen(fileVal);
                for(i = 0; ((i < 8) && (i < iRet)); i++)
                {
                    tmp = a2digit(fileVal[i]);

                    if(i & 0x01)
                    {
                        rgstInfo.addr[i >> 1] |= tmp & 0x0f; 
                    }
                    else
                    {
                        rgstInfo.addr[i >> 1] |= ((tmp << 4) & 0xf0); 
                    }
                }

                break;
            case 'f':               /** file /dev/mcp2510 **/
                filename = optarg;
                break;
            case 's':
                //adapterNo = a2digit(optarg[0]);
                rgstInfo.ucAdapterNo = a2digit(optarg[0]);
                break;
            case 't':
                //ucType = a2digit(optarg[0]);
                rgstInfo.ucDevType = a2digit(optarg[0]);
                break;
            case 'v':
                printf("%s version: %s - %s\n", basename(argv[0]), version, date);
                if(argc <= 2)
                {
                    return 0;
                }
                break;

            default:
                printf("Invalid optional %c: %s\n", opt, optarg);
                exit(-1);
        }   
    }   
    
    signal(SIGINT,handler);

    fd = open(filename, O_RDWR | O_SYNC);
    if (fd < 0)
    {
        printf("Open %s Failed! err string: %s\n", filename, strerror(errno));
        goto err1;
    }

    //iRet = write(fd, (char *)pSendCmd, sizeof(struct sendCmd_s));
    iRet = ioctl(fd, MDevRgst, &rgstInfo);
    if(iRet >= 0)
    {
        printf("[%s-%d]info: write ok! \n", __func__, __LINE__);
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

