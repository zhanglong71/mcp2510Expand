
#include "../inc/CONST.h"
#include    "ascii_tab.h"

/** 0..9, A..F, a..f **/
/** code **/ unsigned char ascii2num[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,     /** control **/
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,     /** control **/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /** reserved **/
    0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    ':',  ';',  '<',  '=',  '>',  '?', 
    '@',  10,   11,   12,   13,   14,   15,   'G',  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O', 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /** reserved **/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     /** reserved **/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f,     /** reserved **/
};

/** code **/ unsigned char num2ascii[] = {
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,     /** 0..9 **/
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46                              /** A..F **/
};
/*******************************************************************************
 * function: 
 * input: __string
 *        len - 有效数据的长度。
 * output: __string
 *
 * description: 数字原地转换成ascii码。以0x02起头，0x03结尾
 * 0x12 + 0x34 + ... +
 * 0x02 + 0x31 + 0x32 + 0x33 + 0x34 + ... + 0x03
 *******************************************************************************/
unsigned char data2ascii(unsigned char *__string, unsigned char len)
{
    int i;
    
    for(i = len - 1; i >= 0; i--)
    {
        __string[2 * i + 1] = num2ascii[(__string[i] >> 4) & 0x0f];  /** 高4位 **/
        __string[2 * i + 2] = num2ascii[__string[i] & 0x0f];         /** 低4位 **/
    }
    __string[0] = 0x02;                     /** STX **/
    __string[2 * (len - 1) + 3] = 0x03;     /** ETX **/
    
    return  0;
}
/*******************************************************************************
 * function:  
 * input: __string
 *        len - ascii数据的长度, 包括开始的0x02, 及结束的0x03
 * output: __string
 * return: length of data
 * description: 以0x02起头，0x03结尾的一串ascii码原地转换成字节数字。
 * 0x02 + 0xy1 + 0xy2 + 0xy3 + 0xy4 + ... + 0xyy + 0x03  ==>
 * 0x12 + 0x34 + ... + 0x..
 *******************************************************************************/
unsigned char ascii2data(unsigned char *__string,  unsigned char len)
{
    unsigned char i = 0;
    unsigned char ucTmp;
    
    while((i << 1) < len)
    {
        ucTmp = __string[i * 2 + 1];
        if(((ucTmp >= 0x30) && (ucTmp <= 0x39)) || ((ucTmp >= 0x41) && (ucTmp <= 0x46)))    /** 0..9, A..F **/
        {
            __string[i] = (ascii2num[ucTmp] << 4) & 0xf0;
        }
        else if((ucTmp == ASCII_ETX) || (ucTmp == ASCII_STX))
        {
            break;
        }
        else
        {
            return  0;
        }
        
        ucTmp = __string[i * 2 + 2];
        if(((ucTmp >= 0x30) && (ucTmp <= 0x39)) || ((ucTmp >= 0x41) && (ucTmp <= 0x46)))    /** 0..9, A..F **/
        {
            __string[i] |= (ascii2num[ucTmp] & 0x0f);
        }
        else if((ucTmp == ASCII_ETX) || (ucTmp == ASCII_STX))
        {
            break;
        }
        else
        {
            return  0;
        }
        
        i++;
    }
    
    return  i;
}
