#ifndef __X10QUE_H__
#define __X10QUE_H__

unsigned char num2house(unsigned char __num);
unsigned char num2key(unsigned char __num);
unsigned char num2func(unsigned char __num);

unsigned char house2num(unsigned char __house);
unsigned char key2num(unsigned char __key);


int X10QueueIn(X10Buf_queue_t *q, X10Data_t *X10data);
int X10QueueOut(X10Buf_queue_t *q, X10Data_t *X10data);
int X10QueueIn_irq(X10Buf_queue_t *q, X10Data_t *X10data) ;

void dev_allRegister(canFrame_queue_t *canFrame);
void dev_oneRegister(canFrame_queue_t *canFrame, unsigned int __NUM);

#endif	/** __X10QUE_H__ **/
