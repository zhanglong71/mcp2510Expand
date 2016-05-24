

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <stdio.h>

/**************************************************************************************
 * Description: 将字符串中的一行取出, 并且将行尾的空格，制表符，回车符清为空字符
 * input: str - 待处理字符串
 * output: no
 * return: 返回下一行的起始地址, 如果无行结束符则返回NULL
 **************************************************************************************/
char * strGetLine(char* str)
{

    char * ptr1 = NULL;
    char * ptr2 = NULL;

    ptr1 = strchr(str, '\n');   /** 取一行数据(str开头，ptr2结尾) **/
    ptr2 = ptr1;

    if(ptr1 != NULL)
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
    else
    {
        return NULL;
    }

    return  ++ptr2;
}

/***
 * 首先匹配key, 再在键的基础上通过":"找到值
 ***/
char* getValueByKey(char* src, char* key)
{
    int iRet = 0;
    char* ptr = NULL;
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

int main(int argc, char * argv[])
{
    char* ptrSrc = NULL;
    char* ptrNext = NULL;
    char* ptrKey = "key00";
    char* ptrValue = NULL;
    char buf[128];

    memset(buf, 0, 128);
    sprintf(buf, "key00:Value00011111111   \t \r            \n");
    strcat(buf, "key00222222222\n");
    strcat(buf, "333333333\n");
    strcat(buf, "44444444444\n");
    ptrSrc = buf;
    printf("1111111111111[%s]\n", ptrSrc);

    ptrSrc = buf;
    ptrNext = buf;
    while((ptrNext != NULL) && (*ptrNext != '\n'))
    {
        ptrNext = strGetLine(ptrSrc);
        if(ptrNext == NULL)     /** 无行结束符则退出循环 **/
        {
            break;
        }
        ptrValue = getValueByKey(ptrSrc, ptrKey);
        if(ptrValue == NULL)    /** 找不到键，或有键无值 **/
        {
            printf("[key:NoValue]-[%s-%s]\n", ptrKey, ptrSrc);
            continue;
        }
        ptrSrc = ptrNext;

        //printf("[key:value]-[%s]\n", ptrNext);
        printf("[key:value]-[%s-%s]\n", ptrKey, ptrValue);
    }
    return  0;
}

/**
 * 字符串处理: 取一行数据，将行尾的空格及制表符换成结束符
 **/


