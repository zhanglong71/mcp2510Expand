#ifndef __MACRO_H__
#define __MACRO_H__
/*******************************************************************************/
/** 数组大小 **/
#define	TABSIZE(xxx)	(sizeof(xxx)/sizeof(xxx[0]))
/***********************************************************/
#define	MCANLED_ON() (GPIO_ResetBits(GPIOA, GPIO_Pin_0))
#define	MCANLED_OFF() (GPIO_SetBits(GPIOA, GPIO_Pin_0))
#define	MCANLED_GET() (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_0))
#define	MCANLED_INVERSE() do{\
	if(MCANLED_GET()){MCANLED_ON();}else{MCANLED_OFF();}\
							}while(0)
												
/***********************************************************/
#define	MINFLED_ON() (GPIO_SetBits(GPIOA, GPIO_Pin_1))
#define	MINFLED_OFF() (GPIO_ResetBits(GPIOA, GPIO_Pin_1))
#define	MINFLED_GET() (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_1))
#define	MINFLED_INVERSE() do{\
	if(MINFLED_GET()){MINFLED_OFF();}else{MINFLED_ON();}\
							}while(0)
/***********************************************************/
	/** 正逻辑 **/
#define	MINFRARED_OUT_L()	(GPIO_ResetBits(GPIOA, GPIO_Pin_5))
#define	MINFRARED_OUT_H()	(GPIO_SetBits(GPIOA, GPIO_Pin_5))
#define	MINFRARED_OUT_GET() (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_5))
#define	MINFRARED_IDLE()	MINFRARED_OUT_L()
//#define	MINFRARED_IDLE()	MINFRARED_OUT_H()
#define	MINFRARED_SIGNAL_INVERSE() do{\
	if(MINFRARED_OUT_GET()){MINFRARED_OUT_L();}else{MINFRARED_OUT_H();}\
							}while(0)


#if	1	//use other pin for test 
	#define	MINFRARED_READ()	(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_9))			/** 读取输入的数据 **/
	#define	MINFRARED_READ_H()	(GPIO_SetBits(GPIOC, GPIO_Pin_9))
	#define	MINFRARED_IsRECORDING()	(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7) == 0)	/** 状态 **/
#else
	#define	MINFRARED_READ()	(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4))			/** 读取输入的数据 **/
	#define	MINFRARED_READ_H()	(GPIO_SetBits(GPIOA, GPIO_Pin_4))
	#define	MINFRARED_IsRECORDING()	(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6) == 0)	/** 状态 **/
#endif
/***********************************************************/
#define	MDEBUG_PA7(x) do{\
	if((x) == 0){GPIO_ResetBits(GPIOA, GPIO_Pin_7);}else{GPIO_SetBits(GPIOA, GPIO_Pin_7);}\
							}while(0)
							
#define	MDEBUG_PA7_INVERSE() do{\
	if(GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_7)){GPIO_ResetBits(GPIOA, GPIO_Pin_7);}else{GPIO_SetBits(GPIOA, GPIO_Pin_7);}\
							}while(0)
/***********************************************************/
#define	Mchannel(x)	do{\
						GPIO_ResetBits(GPIOB, GPIO_Pin_0);\
						GPIO_ResetBits(GPIOB, GPIO_Pin_1);\
						GPIO_ResetBits(GPIOB, GPIO_Pin_2);\
						GPIO_ResetBits(GPIOB, GPIO_Pin_3);\
						GPIO_ResetBits(GPIOB, GPIO_Pin_4);\
					if(x == 0){GPIO_SetBits(GPIOB, GPIO_Pin_0);}\
					else if(x == 1){GPIO_SetBits(GPIOB, GPIO_Pin_1);}\
					else if(x == 2){GPIO_SetBits(GPIOB, GPIO_Pin_2);}\
					else if(x == 3){GPIO_SetBits(GPIOB, GPIO_Pin_3);}\
					else if(x == 4){GPIO_SetBits(GPIOB, GPIO_Pin_4);}\
				}while(0)

#if	0				
#define	Mchannel(x)	do{\
	GPIO_Write(GPIOB, (GPIO_ReadOutputData(GPIOB) & ~0x1f) | (1 << x));\
				}while(0)
#endif
/*******************************************************************************/
#define Mpage2Addr(x)	(unsigned int)(0x0801FC00 - (0x400 * (x)))
/*******************************************************************************/
#endif
