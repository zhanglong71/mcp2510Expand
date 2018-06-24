#ifndef __ARCH_H__
#define __ARCH_H__

void	init_fstack(fstack_t *s);
int	stackpop(fstack_t *s);
int	stackpush(fstack_t *s, func_t *func);
int	stacktop(const fstack_t *s, func_t *f);
void	init_queue(msgq_t *q);

int	inq(msgq_t *q, msgType_t data);
int	outq(msgq_t *q, msgType_t *data);
int	inq_irq(msgq_t *q, msgType_t data);
int	outq_irq(msgq_t *q, msgType_t *data);

void SetTimer(Timer_t * timer, int tick); 
void SetTimer_irq(Timer_t * timer, int tick);
void ClrTimer(Timer_t * timer);
void delay(int __ms);

int sysProcess(msgType_t *data);

void transmission(unsigned *data);
//void DAEMON_USART2_Send(void);
void DAEMON_USARTx_Send(USART_TypeDef* USARTx, charBuf_queue_t* comTxQue);
void DAEMON_CAN_Send(void);

#if 0
void comTxStart(void);
void comTxStop(void);
int comRxCheck(void);
#endif

#endif
