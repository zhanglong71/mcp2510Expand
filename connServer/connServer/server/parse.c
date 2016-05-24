
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>

#include    "ptype.h"
#include    "parse.h"

char* g_format[] = {
    "idle",
    "packet-format",
#define CPACKETFORMAT   "packet-format"

    "login-request",
    "login-success",
    "login-fail",
    "login-ok",
    "login-out",
    "logout",

    "data-forward",
    "data-send",
    "data-ACK",
    "data-raw",
    "data-CGI",
};

char* headField[] = {
    "dstClientId",
    "srcClientId", 

    "dstUserCode",
    "srcUserCode",

    "dstHouseCode",
    "srcHouseCode",

    "dstKeyCode",      /** unit **/
    "srcKeyCode",      /** unit **/

    "dstFunctionCode",
    "srcFunctionCode",

    "dstDeviceType",
    "srcDeviceType",

    "packetLength", /** total length(bytes), except crc checksum **/
    "checkSum", /** checksum **/
};

char* actionRequest[] = {
    "action-request",

    "qizhong-request", 
    "luozhong-request",
    "jiazhong-request",
};

char* actionResponse[] = {
    "action-response",

    "qizhong-success", 
    "qizhong-failed", 
    "luozhong-success",
    "luozhong-failed",
    "jiazhong-success",
    "jiazhong-failed",
};


#define TABLE_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

int getFormatIdByString(char* str)
{
    int i;
    for(i = 0; i < TABLE_SIZE(g_format); i++)
    {
        if(strncmp(str, g_format[i], 4) == 0)
        {
            return i;
        }
    }
    return 0;
}
char* getStringByFormatId(int __formatId)
{
    return  g_format[__formatId];
}

/****************************************************************************
 * 处理一个字符串，其中有多个形如"key:value\n"的行. 
 *      从__source中将key对应的value取出并存放在__value中
 * name parseValueByKey
 * input : __source
 *          __key
 * output: __value
 * return no
 * other: 
 ****************************************************************************/
int parseValueByKey(char *__source /** input **/, char* __key/** input **/, char* __value/** output **/)
{
    char* ptrSrc = NULL;
    char* ptrNext = NULL;
    char* ptrValue = NULL;

    if((__source== NULL) || (__key == NULL) || __value == NULL)
    {
        return 0;
    }

    *__value = '\0';
    ptrSrc = __source;
    ptrNext = __source;
    while(ptrNext != NULL/** 最后一行已经处理完成 **/)
    {
        ptrNext = strGetLine(ptrSrc);               /** 先找到并保存下一行的地址 **/
        ptrValue = getValueByKey(ptrSrc, __key);    /** 找到了行结束符 **/
        if(ptrValue == NULL)                        /** 找不到键，或有键无值 **/
        {
            //printf("[key:NoValue]-[%s-%s]\n", ptrKey, ptrSrc);
            //continue;
        }
        else
        {
            strcpy(__value, ptrValue);
            //printf("[key:value]-[%s-%s]\n", ptrKey, ptrValue);
        }
        ptrSrc = ptrNext;
        //printf("[%s-%d]ptrNext[%s]/ptrValue[%s]!\n", __FILE__, __LINE__, ptrNext,  ptrValue);
    }
    return  1;
}

/*****************************************************************
 * PacketFormat 
 *****************************************************************/
int parsePacketFormatId(char *__source, char* __value)
{
    return parseValueByKey(__source, "packet-format", __value);
}

int setPacketFormat(char *__source, char* __value)
{
    if((__source!= NULL) && (__value != NULL))
    {
        strcat(__source, "packet-format:");
        strcat(__source, __value);
        strcat(__source, "\n");

        return 1;   /** ok **/
    }
    return  0;      /** failed **/
}

/*****************************************************************
 * DstImsi
 * description: 目标移动用户识别码
 *****************************************************************/
int parseDstImsi(char *__source, char *__value)
{
    return parseValueByKey(__source, "Destination-Imsi", __value);
}


int setDstImsi(char *__source, char *__value)
{
    if((__source!= NULL) && (__value!= NULL))
    {
        strcat(__source, "Destination-Imsi:");
        strcat(__source, __value);
        strcat(__source, "\n");

        return 1;   /** ok **/
    }
    return  0;      /** failed **/
}

/*****************************************************************
 * SrcImsi
 * description: 源移动用户识别码
 *****************************************************************/
int parseSrcImsi(char *__source, char *__value)
{
    return parseValueByKey(__source, "Source-Imsi", __value);
}

int setSrcImsi(char *__source, char *__value)
{
    if((__source!= NULL) && (__value!= NULL))
    {
        strcat(__source, "Source-Imsi:");
        strcat(__source, __value);
        strcat(__source, "\n");

        return 1;   /** ok **/
    }
    return  0;      /** failed **/
}

/*****************************************************************
 * SrcUser
 * description: src UserCode
 *****************************************************************/
int parseSrcUser(char *__source, char *__value)
{
    return parseValueByKey(__source, "srcUserCode", __value);
}

int setSrcUser(char *__source, char *__value)
{
    if((__source!= NULL) && (__value!= NULL))
    {
        strcat(__source, "srcUserCode:");
        strcat(__source, __value);
        strcat(__source, "\n");

        return 1;   /** ok **/
    }
    return  0;      /** failed **/
}

/*****************************************************************
 * dstUser
 * description: dst UserCode
 *****************************************************************/
int parseDstUser(char *__source, char *__value)
{
    return parseValueByKey(__source, "dstUserCode", __value);
}

int setDstUser(char *__source, char *__value)
{
    if((__source!= NULL) && (__value!= NULL))
    {
        strcat(__source, "dstUserCode:");
        strcat(__source, __value);
        strcat(__source, "\n");

        return 1;   /** ok **/
    }
    return  0;      /** failed **/
}

/*****************************************************************
 * SrcHouse
 * description: src HouseCode
 *****************************************************************/
int parseSrcHouse(char *__source, char *__value)
{
    return parseValueByKey(__source, "srcHouseCode", __value);
}

int setSrcHouse(char *__source, char *__value)
{
    if((__source!= NULL) && (__value!= NULL))
    {
        strcat(__source, "srcHouseCode:");
        strcat(__source, __value);
        strcat(__source, "\n");

        return 1;   /** ok **/
    }
    return  0;      /** failed **/
}

/*****************************************************************
 * DstHouse
 * description: dst houseCode
 *****************************************************************/
int parseDstHouse(char *__source, char *__value)
{
    return parseValueByKey(__source, "dstHouseCode", __value);
}

int setDstHouse(char *__source, char *__value)
{
    if((__source!= NULL) && (__value!= NULL))
    {
        strcat(__source, "dstHouseCode:");
        strcat(__source, __value);
        strcat(__source, "\n");

        return 1;   /** ok **/
    }
    return  0;      /** failed **/
}

/*****************************************************************
 * srcKey
 * description: src keyCode
 *****************************************************************/
int parseSrcKey(char *__source, char *__value)
{
    return parseValueByKey(__source, "srcKeyCode", __value);
}

int setSrcKey(char *__source, char *__value)
{
    if((__source!= NULL) && (__value!= NULL))
    {
        strcat(__source, "srcKeyCode:");
        strcat(__source, __value);
        strcat(__source, "\n");

        return 1;   /** ok **/
    }
    return  0;      /** failed **/
}


/*****************************************************************
 * DstKey
 * description: dst keyCode
 *****************************************************************/
int parseDstKey(char *__source, char *__value)
{
    return parseValueByKey(__source, "dstKeyCode", __value);
}

int setDstKey(char *__source, char *__value)
{
    if((__source!= NULL) && (__value!= NULL))
    {
        strcat(__source, "dstKeyCode:");
        strcat(__source, __value);
        strcat(__source, "\n");

        return 1;   /** ok **/
    }
    return  0;      /** failed **/
}

/*****************************************************************
 * functionCode
 * description: functionCode
 *****************************************************************/
int parseFunction(char *__source, char *__value)
{
    return parseValueByKey(__source, "functionCode", __value);
}

int setFunction(char *__source, char *__value)
{
    if((__source!= NULL) && (__value!= NULL))
    {
        strcat(__source, "functionCode:");
        strcat(__source, __value);
        strcat(__source, "\n");

        return 1;   /** ok **/
    }
    return  0;      /** failed **/
}

/*****************************************************************
 * checksum
 * description: checksum
 *****************************************************************/
int parseCheckSum(char *__source, char *__value)
{
    return parseValueByKey(__source, "checkSum", __value);
}

int setCheckSum(char *__source, char *__value)
{
    if((__source!= NULL) && (__value!= NULL))
    {
        strcat(__source, "checkSum:");
        strcat(__source, __value);
        strcat(__source, "\n");

        return 1;   /** ok **/
    }
    return  0;      /** failed **/
}

/*****************************************************************
 * srcDevType
 * description: src DevType
 *****************************************************************/
int parseSrcDevType(char *__source, char *__value)
{
    return parseValueByKey(__source, "srcDevType", __value);
}

int setSrcDevType(char *__source, char *__value)
{
    if((__source!= NULL) && (__value!= NULL))
    {
        strcat(__source, "srcDevType:");
        strcat(__source, __value);
        strcat(__source, "\n");

        return 1;   /** ok **/
    }
    return  0;      /** failed **/
}

/*****************************************************************
 * dstDevType
 * description: dst DevType
 *****************************************************************/
int parseDstDevType(char *__source, char *__value)
{
    return parseValueByKey(__source, "dstDevType", __value);
}

int setDstDevType(char *__source, char *__value)
{
    if((__source!= NULL) && (__value!= NULL))
    {
        strcat(__source, "dstDevType:");
        strcat(__source, __value);
        strcat(__source, "\n");

        return 1;   /** ok **/
    }
    return  0;      /** failed **/
}

/*****************************************************************
 * parse/set action/response 
 *****************************************************************/
int parseActionRequest(char *__source, char *__value)
{
    return parseValueByKey(__source, "Action-request", __value);
}

int setActionRequest(char *__source, char *__value)
{
    if((__source!= NULL) && (__value!= NULL))
    {
        strcat(__source, "Action-request:");
        strcat(__source, __value);
        strcat(__source, "\n");

        return 1;   /** ok **/
    }
    return  0;      /** failed **/
}

int parseActionResponse(char *__source, char *__value)
{
    return parseValueByKey(__source, "Action-response", __value);
}

int setActionResponse(char *__source, char *__value)
{
    if((__source!= NULL) && (__value!= NULL))
    {
        strcat(__source, "Action-response");
        strcat(__source, __value);
        strcat(__source, "\n");

        return 1;   /** ok **/
    }
    return  0;      /** failed **/
}

/****************************************************************
 *
 ****************************************************************/
#if 0
unsigned int parseSrcClientId(char *__buf)
{
    struct packetData_s *packet = (struct packetData_s *)__buf;

    return packet->srcClientId;
}
unsigned int parseClientId(char *__buf)
{
    struct packetData_s *packet = (struct packetData_s *)__buf;

    return packet->dstClientId;
}
#endif

#if 0
char *parsePassword(char *__buf)
{
    struct packetAuthentication_s *packet = (struct packetAuthentication_s *)__buf;

    return packet->password;
}

unsigned int parseAuthentication(char *__buf)
{
    struct packetAuthentication_s *packet = (struct packetAuthentication_s *)__buf;

    return packet->passwordLen;
}
#endif

/**************************************************************************************
 * Description: 将字符串中的一行取出, 并且将行尾的空格，制表符，回车符清为空字符
 * input: str - 待处理字符串
 * output: no
 * return: 返回下一行的起始地址, 如果此字符串无行结束符则返回NULL
 *
 * other: 如果第一个字符就是'\n', 也可以处理，只是立即将其置成'\0'
 **************************************************************************************/
char * strGetLine(char* str)
{

    char * ptr1 = NULL;
    char * ptr2 = NULL;

    ptr1 = strchr(str, '\n');   /** 取一行数据(str开头，ptr2结尾) **/
    ptr2 = ptr1;

    if(ptr1 != NULL/** 找到回车符 **/)
    {
        *ptr1 = '\0';   /** 首先将找到的'\n'置成'\0' **/
        ptr1--;

        while(ptr1 > str)       /** 将前面的空格及制表符换成结束符 **/
        {
            if((*ptr1 == '\t') || (*ptr1 == ' ') || (*ptr1 == '\r'))
            {
                *ptr1 = '\0';
                ptr1--;
            }
            else
            {
                //printf("[%s-%d]debug: %s\n", __func__, __LINE__, ptr2);
                break;
            }
        }
    }
    else    /** 无回车符. 已经是最后一行了 **/
    {
        return NULL;
    }

    ++ptr2;
    //printf("next------------%s\n", ptr2);
    return  ptr2;
}

/********************************************
 * 只处理形如"key:value"的字符串
 * 首先匹配key, 再在键的基础上通过":"找到值
 ********************************************/
char* getValueByKey(char* src, char* key)
{
    //int iRet = 0;
    //char* ptr = NULL;
    char* ptrKey = NULL;
    char* ptrValue = NULL;

#if 1
    ptrKey = strstr(src, key);
    if(ptrKey != NULL)   /** 找到键 **/
    {
        ptrValue = strchr(ptrKey, ':');
        if(ptrValue != NULL)    /** 找到':', 返回':'之后的字符串 **/
        {
            ptrValue++;
            return ptrValue;    /** 似乎不可能为NULL, 但所在的值可能为'\0' **/
        }
    }
#endif
    return NULL;    /** 没有"键:值"对(找不到键，或只有键，没有值) **/
}

