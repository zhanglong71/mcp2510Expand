#include "stm32f10x_lib.h"
#include "../inc/CONST.h"
#include "../inc/ctype.h"
#include "bitmapOp.h"

/** bitMap operation**/

void bitmap_clean(bitmap_t* bitmap)
{
  	bitmap->bitmap = 0;
}

void bitmap_setall(bitmap_t* bitmap)
{
	bitmap->bitmap = ~0;
}

/***************************************************** 
 * ��λͼ�ϵ�ָ��λ����1����������1����֮ǰ��ֵ 
 *
 * input: offset (��0��ʼ��ƫ����)
 *        ucBitMap[]
 * output:ucBitMap[]
 * return: oldval of specified offset of ucBitMap[] 
 *
 * 
 *****************************************************/
unsigned int bitmap_test_set(bitmap_t* bitmap, int offset)
{
    unsigned int mask = (1 << offset);
    unsigned int oldval = bitmap->bitmap & mask;
    
    bitmap->bitmap |= mask;
    
	return 	 oldval;
}
/**************************************************** 
 *
 * �鿴�Ƿ���ָ������Ŀ��ô���λ����1
 *
 * input: num
          ucBitMap[]
 * output: no
 * return: 1 - yes
 *         0 - no
 *
 ****************************************************/
int bitmap_isfull(bitmap_t* bitmap, int num)
{
    unsigned int  mask = (1 << num) - 1;                  /** low num-bit all 1 **/
    
    unsigned int  lowbit = (mask & bitmap->bitmap);       /** get low num-bit **/
    
    return !(lowbit ^ mask);
}

