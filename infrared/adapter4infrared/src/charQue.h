#ifndef __COMTXQUE_H__
#define __COMTXQUE_H__

int isCharQueEmpty(charBuf_queue_t *q);
void charQueueInit(charBuf_queue_t *q);
int charQueueIn(charBuf_queue_t *q, charData_t *data);
int charQueueIn_irq(charBuf_queue_t *q, charData_t *data);
int charQueueOut(charBuf_queue_t *q, charData_t *data);
int charQueueOut_irq(charBuf_queue_t *q, charData_t *data);

#endif
