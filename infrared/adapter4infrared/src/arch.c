
#include "stm32f10x_lib.h"
#include "../inc/CONST.h"
#include "../inc/ctype.h"
#include "../inc/global.h"	
#include "../inc/debug.h"
#include "src/charQue.h"
#include "src/canTxQue.h"
#include "ascii_tab.h"
#include "../main.h"
#include "arch.h"
#include "crc8.h" 
//#include "X10Que.h"

/*******************************************************************************
 *
 * function stack operation
 *
 ********************************************************************************/
void init_fstack(fstack_t *s)
{
	s->top = 0;
}

int	stackpop(fstack_t *s)
{
	if((s->top <= 0) || (s->top >= STACKSIZE))    /** make sure ... top [1..STACKSIZE - 1]  **/
	{
		return	FALSE;
	}
	
	s->top--;
	return	TRUE;
}

int	stackpush(fstack_t *s, func_t *func)   /** make sure ... top [0..STACKSIZE-2]  **/
{
	if((s->top < 0) || (s->top >= STACKSIZE - 1))
	{
		return	FALSE;
	}
	s->top++;
	s->func[s->top].func = func->func;
	s->func[s->top].arg = func->arg;
	
	return	TRUE;
}
/**
 * get the data at the top of the stack
 *
 **/
int	stacktop(const fstack_t *s, func_t *f)   /** make sure ... top [1..STACKSIZE - 1]  **/
{
	if((s->top <= 0) || (s->top >= STACKSIZE))
	{
		return	FALSE;
	}
	f->func = s->func[s->top].func;
	f->arg = s->func[s->top].arg;

	return	TRUE;
}
/*******************************************************************************
 *
 * msg queue operation
 *
 *******************************************************************************/
void init_queue(msgq_t *q)
{
	q->tail = q->head = 0;
}

static int isempty(msgq_t *q)
{
	if(q->tail == q->head)
	{
		return TRUE;
	}
	return	FALSE;
}

static int isfull(msgq_t *q)
{
	if((q->tail + 1 == q->head) || (((q->tail + 1) % QUEUESIZE) == (q->head)))
	{
		return TRUE;
	}
	return	FALSE;
}

int	inq(msgq_t *q, msgType_t val)		    //Note: check queue full is necessary before invoke this routine
{
    if(isfull(q))
    {
        return  FALSE;
    }
    
	q->buf[q->tail] = val;
	q->tail = (q->tail + 1) % QUEUESIZE;

	return TRUE;
}

int	inq_irq(msgq_t *q, msgType_t val)		//Note: check queue full is necessary before invoke this routine
{
    int iRet;
    
    IRQ_disable();
    iRet = inq(q, val);
    IRQ_enable();

	return iRet;
}

int	outq(msgq_t *q, msgType_t *val)		//Note: check queue empty is necessary before invoke this routine
{
    if(isempty(q))
    {
        return  FALSE;
    }
    
	*val = q->buf[q->head];
	q->head = (q->head + 1) % QUEUESIZE;
	    
	return	TRUE;
}
int	outq_irq(msgq_t *q, msgType_t *val)		//Note: check queue empty is necessary before invoke this routine
{
    int iRet;
    
    IRQ_disable();
    iRet = outq(q, val);
	IRQ_enable();
	    
	return	iRet;
}

/*******************************************************************************
 * Timer operation
 *******************************************************************************/
void SetTimer(Timer_t * timer, int tick)
{
//    timer->count = 0;
    timer->tick = tick;
    timer->tick_bak = tick;
}

void SetTimer_irq(Timer_t * timer, int tick)
{
    IRQ_disable();
    
    SetTimer(timer, tick);
    
    IRQ_enable();
}

void ClrTimer(Timer_t * timer)
{
    timer->tick = 0;
    timer->tick_bak = 0;
//    timer->count = 0;
}
void ClrTimer_irq(Timer_t * timer)
{
    IRQ_disable();
    
    ClrTimer(timer);
    
    IRQ_enable();
}
/*******************************************************************************
 * delay() operation
 * 
 * wait until timer out
 *******************************************************************************/
int g_tmr_delay;

void delay(int TickCount)
{
    g_tmr_delay = TickCount;
    
    while(g_tmr_delay);
}

void setDelayTimer(u8 TimerNum, int TickCount)
{
    IRQ_disable();
    if(TimerNum == 0)
    {
        g_tmr_delay = TickCount;
    }
    IRQ_enable();
}

int getDelayTimer(u8 TimerNum)
{
    if(TimerNum == 0) 
    {
        return g_tmr_delay;
    }
    else 
    {
        return	0;
    }
}


/////////////////////////////////////////////////////
