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
#include "../crc8.c"
#include "../protocol.h"

#if 0
/** 临时发送数据之用 **/
typedef struct sendCmd_s {
    #define MMaxRcvLen      32
    unsigned char cmd[MMaxRcvLen];  /** note: the size must bigger than the longest cmd **/
    unsigned int len;

    unsigned int uiEID;
    unsigned short usSID;
    unsigned char ucAdapterNo;
}sendCmd_t;

#endif
/** 
 * 用法: toolName deviceName dataString adapterNo
 * dataString部分不包括最后的Crc8校验
 * eg. on:      ./app_raw /dev/mcp510 010f0000,00000000,0000,4201000500 e
 * eg. all on:  ./app_raw /dev/mcp510 010f0000,00000000,0000,4201000300 e
 * eg. off:     ./app_raw /dev/mcp510 010f0000,00000000,0000,4201000700 e
 * eg. all off: ./app_raw /dev/mcp510 010f0000,00000000,0000,4201000100 e
 *
 **/

#define min(x,y) (((x) < (y)) ? (x) : (y)) 
#define CanAdapterNo  0x01
           
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
    int iRet;
    int iLen;
    unsigned char tmp;
    unsigned char adapterNo = 0;
    struct sendCmd_s *pSendCmd;
    char *filename = "/dev/mcp2510";
    char *fileVal = NULL;
    //union DevCmd_s cmdFrame;



    for(i = 1; i < argc; i++)
    {
        if((argv[i][0] == '-') && (argv[i][1] == 'v'))
        {
            printf("%s version: %s - %s\n", basename(argv[0]), version, date);
            goto err0;
        }
    }
    
    pSendCmd = malloc(sizeof(struct sendCmd_s));
    if(pSendCmd == NULL)
    {
        printf("[%s-%d] malloc failed!\n", __FILE__, __LINE__);
        goto err0;
    }
    memset(pSendCmd, 0, sizeof(struct sendCmd_s));

    if(argc >= 4)
    {
        filename = argv[1];

        fileVal = argv[2];
        iLen = strlen(argv[2]);
        iLen = iLen > 64 ? 64 : iLen;
        pSendCmd->len = (iLen >> 1);       /** 从第３个参数的长度得到数据长度 **/
        for(i = 0; i < iLen; i++)
        {
            if(isxdigit(argv[2][i]))
            {
                if((argv[2][i] >= '0') && (argv[2][i] <= '9'))
                {
                    tmp = argv[2][i] - '0';
                }
                else if((argv[2][i] >= 'a') && (argv[2][i] <= 'f'))
                {
                    tmp = argv[2][i] - 'a' + 10;
                }
                else if((argv[2][i] >= 'A') && (argv[2][i] <= 'F'))
                {
                    tmp = argv[2][i] - 'A' + 10;
                }

                if(i & 0x01)
                {
                    pSendCmd->cmd[i >> 1] |= tmp & 0x0f; 
                }
                else
                {
                    pSendCmd->cmd[i >> 1] |= ((tmp << 4) & 0xf0); 
                }
            }
            else
            {
                printf("[%s-%d]Warning: data(argv[2]) argument error!\n", __FILE__, __LINE__);
                goto err1;
            }
        }

#if 0
        for(i = 0; i < iLen; i++)   /** 命令的第３个参数作为数据内容 **/
        {
            pSendCmd->cmd[i] = argv[2][i];
        }
#endif

        if(isxdigit(argv[3][0]))    /** 命令的第四个参数作为目的设备的 adapterNo **/
        {
            if((argv[3][0] >= '0') && (argv[3][0] <= '9'))
            {
                adapterNo = argv[3][0] - '0';
            }
            else if((argv[3][0] >= 'A') && (argv[3][0] <= 'F'))
            {
                adapterNo = argv[3][0] - 'A' + 10;
            }
            else if((argv[3][0] >= 'a') && (argv[3][0] <= 'f'))
            {
                adapterNo = argv[3][0] - 'a' + 10;
            }
            else
            {
                printf("[%s-%d]Warning: adapterNo(argv[3][0]) argument error!\n", __FILE__, __LINE__);
                goto err1;
            }
        }
    }
    else
    {
        printf("[%s-%d]Warning: argument error!\n", __FILE__, __LINE__);
        goto err1;
    }
    pSendCmd->ucAdapterNo = adapterNo;
    pSendCmd->usSID = 0;
    pSendCmd->uiEID = pSendCmd->ucAdapterNo;

#if 1
#else
    /** 0..3 dest address **/
    for(iLen = 0; iLen < 4; iLen++)
    {
        //pSendCmd->cmd[iLen] = ((struct canbusDevice_s *)devInfo)->stIdMsg.ucPhysAddr[iLen];
    }
    /** 4..7 src address **/
    for(iLen = 0; iLen < 4; iLen++)
    {
        pSendCmd->cmd[iLen + 4] = 0x0;
    }
    /** 8 version 0x0 **/
    pSendCmd->cmd[8] = 0x0;
    /** 9 reserved 0x0 **/
    pSendCmd->cmd[9] = 0x0;
    /** 10 opCode 0x44 **/
    //pSendCmd->cmd[10] = CRAWCMD;   //cmd;
    pSendCmd->cmd[10] = CGETSTATUS;  //0x41-查状态
    /** 11 attr **/
    pSendCmd->cmd[11] = 0x01;
    /** 12 value7..0 **/
    pSendCmd->cmd[12] = 0x1f;   /** 状态请求L-byte **/
    /** 13 value15..8 **/
    pSendCmd->cmd[13] = 0;      /** 状态请求H-byte **/
    /** 14 reserved **/
    pSendCmd->cmd[14] = 0;
    /** 15 CRC8 **/
    pSendCmd->cmd[15] = makeCrc8(0, pSendCmd->cmd, 15);
    /** 13,14 SID, EID **/
    pSendCmd->len = 16;
    pSendCmd->ucAdapterNo = adapterNo;
    pSendCmd->usSID = 0;
    pSendCmd->uiEID = pSendCmd->ucAdapterNo;
#endif
    signal(SIGINT,handler);

    fd = open(filename, O_RDWR | O_SYNC);
    if (fd < 0)
    {
        printf("Open %s Failed! err string: %s\n", filename, strerror(errno));
        goto err1;
    }

    /** Note: the makeCrc **/
    pSendCmd->cmd[(iLen >> 1)] = makeCrc8(0, pSendCmd->cmd, (iLen >> 1));
    //pSendCmd->cmd[15] = makeCrc8(0, pSendCmd->cmd, 15);
    iRet = write(fd, (char *)pSendCmd, sizeof(struct sendCmd_s));
    if(iRet > 0)
    {
        printf("[%s-%d]info: write ok! \n", __func__, __LINE__);
    }
    else
    {
        printf("[%s-%d]write: iRet = %d, errno = %d !!! \n", __func__, __LINE__, iRet, errno);
    }

    close(fd);
err1:
    free(pSendCmd);
err0:
    return 0;
}

