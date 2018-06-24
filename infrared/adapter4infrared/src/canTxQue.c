#include "stm32f10x_lib.h"
#include "string.h"
#include "../inc/CONST.h"
#include "../inc/ctype.h"
#include "canTxQue.h"
/*******************************************************************************
 * 
 *******************************************************************************/
static int isCanTXQueEmpty(canFrame_queue_t *q)
{
	if(q->tail == q->head)
	{
		return TRUE;
	}
	return	FALSE;
}

static int isCanTXQueFull(canFrame_queue_t *q)
{
	if((q->tail + 1 == q->head) || (((q->tail + 1) % CANQUEUESIZE) == q->head))
	{
		return TRUE;
	}
	return	FALSE;
}

void	CanTXqueueInit(canFrame_queue_t *q)
{
	q->tail = q->head = q->flag = 0;
}

/***********************************************
 *
 * 以下三个函数并非与所在队列有关，只是用于同步
 * CAN固件的发送，严格来说应该是与CAN相关的数据。
 *
 ***********************************************/

int CanDeviceGetDisableFlag(canFrame_queue_t *q)
{
    return (q->flag & 0x01);
}

void CanDeviceSetDisableFlag(canFrame_queue_t *q)
{  
	q->flag |= 0x01;
}
void CanDeviceClrDisableFlag(canFrame_queue_t *q)
{
	q->flag &= ~0x01;
}
/*************************************************/
int CanTXqueueLen(canFrame_queue_t *q)
{
	 return (CANQUEUESIZE + q->tail - q->head) % CANQUEUESIZE;
}

int CanTXqueueIn(canFrame_queue_t *q, CanTxMsg *canmsg)
{
    if(isCanTXQueFull(q))
    {
        return	FALSE;
    }

    memcpy(&(q->buf[q->tail]), canmsg, sizeof(CanTxMsg));
	q->tail = (q->tail + 1) % CANQUEUESIZE;
    
    return TRUE;
}

int CanTXqueueOut(canFrame_queue_t *q, CanTxMsg *canmsg)
{
    if(isCanTXQueEmpty(q))
    {
        return	FALSE;
    }
    
    memcpy(canmsg, &(q->buf[q->head]), sizeof(CanTxMsg));
	q->head = (q->head + 1) % CANQUEUESIZE;
       
    return TRUE;
}
/////////////////////////////////////////////////////

