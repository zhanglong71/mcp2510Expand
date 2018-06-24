#ifndef __CANTXQUE_H__
#define __CANTXQUE_H__

void CanTXqueueInit(canFrame_queue_t *q);
int CanTXqueueLen(canFrame_queue_t *q);
int  CanDeviceGetDisableFlag(canFrame_queue_t *q);
void CanDeviceSetDisableFlag(canFrame_queue_t *q);
void CanDeviceClrDisableFlag(canFrame_queue_t *q);

int CanTXqueueIn(canFrame_queue_t *q, CanTxMsg *canmsg);
int CanTXqueueOut(canFrame_queue_t *q, CanTxMsg *canmsg);

#endif
