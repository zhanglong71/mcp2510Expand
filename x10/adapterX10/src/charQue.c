#include "stm32f10x_lib.h"
#include "string.h"
#include "../inc/CONST.h"
#include "../inc/ctype.h"
#include "../main.h"
#include "charQue.h"
/*******************************************************************************
 * 
 *******************************************************************************/
static int isCharQueEmpty(charBuf_queue_t *q)
{
	if(q->tail == q->head)
	{
		return TRUE;
	}
	return	FALSE;
}

static int isCharQueFull(charBuf_queue_t *q)
{
	if((q->tail + 1 == q->head) || (((q->tail + 1) % CHARQUEUESIZE) == q->head))
	{
		return TRUE;
	}
	return	FALSE;
}


void	charQueueInit(charBuf_queue_t *q)
{
	q->tail = 0;
	q->head = 0;
	q->len = 0;
}
#define charBufInit(x) charQueueInit(x)


int charQueueIn(charBuf_queue_t *q, charData_t *chardata)
{
    if(isCharQueFull(q))
    {
        return	FALSE;
    }

    //memcpy(&(q->buf[q->tail]), chardata, sizeof(CanTxMsg));
    
    q->buf[q->tail] = chardata->ucVal;
//    q->ctrl[q->tail] = chardata->ctrl;
	q->tail = (q->tail + 1) % CHARQUEUESIZE;
    
    return TRUE;
}

int charBufIn(charBuf_queue_t *q, charData_t *chardata)
{
    if(q->len >= CHARQUEUESIZE)
    {
        return	FALSE;
    }

    q->buf[q->len] = chardata->ucVal;
//    q->ctrl[q->len] = chardata->ctrl;
	q->len += 1;
    
    return TRUE;
}

int charQueueOut(charBuf_queue_t *q, charData_t *chardata)
{
    if(isCharQueEmpty(q))
    {
        return	FALSE;
    }
    
    //memcpy(chardata, &(q->buf[q->tail]), sizeof(CanTxMsg));
    chardata->ucVal = q->buf[q->head];
//    chardata->ctrl = q->ctrl[q->head];
	q->head = (q->head + 1) % CHARQUEUESIZE;
       
    return TRUE;
}


int charQueueOut_irq(charBuf_queue_t *q, charData_t *chardata)
{
    int iRet;

    IRQ_disable();
    iRet = charQueueOut(q, chardata);
	IRQ_enable();
	    
	return	iRet;
}
/////////////////////////////////////////////////////

