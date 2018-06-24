#ifndef __DRIVER_H__
#define __DRIVER_H__

int sysProcess(msgType_t *data);

void transmission(unsigned *data);
void DAEMON_USARTx_Send(USART_TypeDef* USARTx, charBuf_queue_t* comTxQue);
void DAEMON_CAN_Send(void);
int flashWrite(u32 arr[], int pageNO);

int readRemoteCodeTable(int _remoteNO/* intput */, int _buttonNO/* intput */, unsigned char *_address/** output **/);
int writeRemoteCodeTable(int _remoteNO/* intput */, int _buttonNO/* intput */, unsigned char *_address/** input **/);
int readRemoteCodeAddons(int _remoteNO/* intput */, unsigned char *_address/** output **/);
int writeRemoteCodeAddons(int _remoteNO/* intput */, unsigned char *_address/** input **/);

#endif
