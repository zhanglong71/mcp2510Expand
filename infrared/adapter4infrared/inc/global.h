#ifndef __GLOBAL_H__
#define __GLOBAL_H__
/*******************************************************************************/
extern u8 g_ucCanseq;
extern u8 g_destAddr;
extern int g_channel;
extern int g_tmr_delay;
extern fstack_t g_fstack;
extern msgq_t  g_msgq;
extern Timer_t g_timer[TIMER_NUM];
extern bitmap_t bmpRcvCan;

extern canFrame_queue_t g_canTxQue; /** canTx from comRx**/
extern charBuf_queue_t g_comRevBuf; /** comRx ==> canTxQue **/
extern charBuf_queue_t g_wlsRevBuf;	/** wirelsee ==> canTxQue **/

extern charBuf_queue_t g_com1TxQue;
extern charBuf_queue_t g_com2TxQue;

extern charBuf_queue_t g_infraredTxQue;


extern charBuf_queue_t g_canRevBuf; /** canRx ==> x10TxQue data fragment **/

extern u32 g_flashPageAddr; 	/** 可读写的flash空间的起始地址 **/
//extern unsigned char g_x10_sync_offset;
//extern unsigned char g_x10_zcross_calibration;
extern const char g_magic[MAGIC_SIZE];
extern flashPage_t g_flash;
extern Phase_sampling_t g_Phase_sending;
/*******************************************************************************/
#endif
