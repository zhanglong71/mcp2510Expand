/******************** (C) COPYRIGHT 2007 STMicroelectronics ********************
* File Name          : stm32f10x_it.c
* Author             : MCD Application Team
* Date First Issued  : 05/21/2007
* Description        : Main Interrupt Service Routines.
*                      This file can be used to describe all the exceptions 
*                      subroutines that may occur within user application.
*                      When an interrupt happens, the software will branch 
*                      automatically to the corresponding routine.
*                      The following routines are all empty, user can write code 
*                      for exceptions handlers and peripherals IRQ interrupts.
********************************************************************************
* History:
* 05/21/2007: V0.3
********************************************************************************
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "string.h"
#include "stm32f10x_it.h"
#include "main.h"

#include "inc/CONST.h"
#include "inc/ctype.h"
#include "inc/global.h"
#include "src/arch.h"
#include "src/bitmapOp.h" 
#include "src/charQue.h" 
#include "src/X10Que.h"

#include "inc/debug.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

vu16 CCR1_Val = 0xBB8;			/** f = 8kHz **/
//vu16 CCR1_Val = 0x1770;			/** f = 4kHz **/
//vu16 CCR1_Val = 0x2EE1;       /** 非常接近1000Hz, 示波器手工测量2f得1000Hz, 自动显示499.984 **/
//vu16 CCR1_Val = 0x2EE0;       /** 非常接近1000Hz, 示波器手工测量2f得1000Hz, 自动显示500.026 **/

//vu16 CCR4_Val = 0x177;			/** 128kHz **/
vu16 CCR4_Val = 0xC8;			/** 示波器手工测量得: f = 119.99kHz **/
//vu16 CCR4_Val = 0xCB;			/** 示波器手工测量得: f = 118.2kHz **/


charBuf_queue_t g_canRevBuf;             /** data fragment **/
charBuf_queue_t g_comRevBuf;             /** data fragment **/

/** 
 * note注意：对此全局数据的处理没有同步保护，因为收一个x10数据帧需要0.5s以上，时间足够
 * 在处理完接收到的x10数据之前，不可能会再次收到一个22bit的x10数据帧
 * 
 **/
 
X10RevData_t	g_x10Revframe;
X10Data_t	g_x10Sndframe;	
X10Data_t	g_x10bakframe;	
//unsigned int g_x10RcvAddr;		/** x10收到地址 **/
bitmap_t    bmpRcvCan;

typedef enum {
	CX10_IDLE = 0,
	CX10_RCV1,
	CX10_RCV2,
	CX10_SSND,
	CX10_DSND,
	CX10_PSND,
	
} X10Status_t;
X10Status_t x10_status;					/** 描述x10收发时的状态 **/
unsigned int x10_timer;					/** 8kHz计数. **/

unsigned int x10_zcross_timer;						/** 进入过零点校正状态检查计数 **/
unsigned char x10_zcross_count = 0;					/** 过零点时刻偏移计数器 **/
unsigned char g_x10_zcross_calibration = 0;			/** 过零点时刻校正 **/

/** 
 * 电力载波偏移同步 
 * 将其理解为，电力线过零点到触发中断的时间差。
 * 理论上，在电力线过零点时发生中断信号。
 * 实际上, 过零点信号的传递要经过两次BJT三极管放大，还通过光电耦合，有一定时间延迟。
 * 此变量即对此延迟进行补偿
 * 由于电子元件的个体差异, 生产后另外需要再校正
 **/
unsigned char g_x10_sync_offset = CX10_SYNC_OFFSET;

unsigned int x10_RcvBit = 0;		/** 接收进的多次采样数据 **/
volatile unsigned int x10_RcvXData = 0;		/** 接收进的数据 **/
unsigned int x10_Rcv1stData;		/** Rcv1接收进的数据 **/
//unsigned int x10_Rcv2Data;		/** 间隔3个cycle之后Rcv2接收进的数据 **/
//unsigned int x10_tmpData;		/** 发送出的数据 **/

/** 
 * 以过零点为中心的前后范围(unit: 0.125ms)
 * 此数据非常敏感。测试发现将其定义成4的时候，数据发送成功率非常低
 **/
//#define	CDELTA_TIMER	5
#define	CDELTA_BC	5					/** 过零点以前 **/
#define	CDELTA_AD	(10 - CDELTA_BC)	/** 过零点以后 **/

static unsigned char EXTI15_count;	/** 外部中断计数, 可用于控制x10的x10_timer计数复位 **/

#define	X10f120KHZ_disable()	(GPIO_SetBits(GPIOA, GPIO_Pin_5))
#define	X10f120KHZ_enable()		(GPIO_ResetBits(GPIOA, GPIO_Pin_5))

#define X10RxLED_ON()		(GPIO_SetBits(GPIOA, GPIO_Pin_1))
#define X10RxLED_OFF()		(GPIO_ResetBits(GPIOA, GPIO_Pin_1))

/** 数据采样，并且采样数据取反 **/
#define	X10_BitRead()	do{\
							x10_RcvBit <<= 1;\
							x10_RcvBit |= !GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4);\
						}while(0)

/** 
 * name:
 * description: count the number of 1 in data 
 * input:
 * output:
 * return: 
 * author: 
 *
 **/
 #define	DEBUG_NOARG	0
#if	DEBUG_NOARG
unsigned int Check1Count(void)
{
	unsigned int i = 0;
	
	while(x10_RcvBit)
	{
		x10_RcvBit = (x10_RcvBit & (x10_RcvBit - 1));
		i++;
	}

	return	i;
}
#else
unsigned int Check1Count(unsigned int input)
{
	unsigned int i = 0;
	
	while(input)
	{
		input = (input & (input - 1));
		i++;
	}

	return	i;
}
#endif

void x10module_init(void)
{
	x10_timer = 0;
	x10_RcvXData = 0;
	x10_status = CX10_IDLE;
	EXTI15_count = 0;
	X10f120KHZ_disable();
}
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : NMIException
* Description    : This function handles NMI exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NMIException(void)
{
}

/*******************************************************************************
* Function Name  : HardFaultException
* Description    : This function handles Hard Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void HardFaultException(void)
{
}

/*******************************************************************************
* Function Name  : MemManageException
* Description    : This function handles Memory Manage exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void MemManageException(void)
{
}

/*******************************************************************************
* Function Name  : BusFaultException
* Description    : This function handles Bus Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void BusFaultException(void)
{
}

/*******************************************************************************
* Function Name  : UsageFaultException
* Description    : This function handles Usage Fault exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void UsageFaultException(void)
{
}

/*******************************************************************************
* Function Name  : DebugMonitor
* Description    : This function handles Debug Monitor exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DebugMonitor(void)
{
}

/*******************************************************************************
* Function Name  : SVCHandler
* Description    : This function handles SVCall exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SVCHandler(void)
{
}

/*******************************************************************************
* Function Name  : PendSVC
* Description    : This function handles PendSVC exception.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void PendSVC(void)
{
}

/*******************************************************************************
* Function Name  : SysTickHandler
* Description    : This function handles SysTick Handler.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SysTickHandler(void)
{
}

/*******************************************************************************
* Function Name  : WWDG_IRQHandler
* Description    : This function handles WWDG interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void WWDG_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : PVD_IRQHandler
* Description    : This function handles PVD interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void PVD_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : TAMPER_IRQHandler
* Description    : This function handles Tamper interrupt request. 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TAMPER_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : RTC_IRQHandler
* Description    : This function handles RTC global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void RTC_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : FLASH_IRQHandler
* Description    : This function handles Flash interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void FLASH_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : RCC_IRQHandler
* Description    : This function handles RCC interrupt request. 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void RCC_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : EXTI0_IRQHandler
* Description    : This function handles External interrupt Line 0 request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void EXTI0_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : EXTI1_IRQHandler
* Description    : This function handles External interrupt Line 1 request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void EXTI1_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : EXTI2_IRQHandler
* Description    : This function handles External interrupt Line 2 request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void EXTI2_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : EXTI3_IRQHandler
* Description    : This function handles External interrupt Line 3 request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void EXTI3_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : EXTI4_IRQHandler
* Description    : This function handles External interrupt Line 4 request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void EXTI4_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : DMAChannel1_IRQHandler
* Description    : This function handles DMA Stream 1 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DMAChannel1_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : DMAChannel2_IRQHandler
* Description    : This function handles DMA Stream 2 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DMAChannel2_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : DMAChannel3_IRQHandler
* Description    : This function handles DMA Stream 3 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DMAChannel3_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : DMAChannel4_IRQHandler
* Description    : This function handles DMA Stream 4 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DMAChannel4_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : DMAChannel5_IRQHandler
* Description    : This function handles DMA Stream 5 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DMAChannel5_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : DMAChannel6_IRQHandler
* Description    : This function handles DMA Stream 6 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DMAChannel6_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : DMAChannel7_IRQHandler
* Description    : This function handles DMA Stream 7 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void DMAChannel7_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : ADC_IRQHandler
* Description    : This function handles ADC global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void ADC_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : USB_HP_CAN_TX_IRQHandler
* Description    : This function handles USB High Priority or CAN TX interrupts 
*                  requests.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USB_HP_CAN_TX_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : USB_LP_CAN_RX0_IRQHandler
* Description    : This function handles USB Low Priority or CAN RX0 interrupts 
*                  requests.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
/**
 * i_flag 0..3 - reserved
 *          4 - canbus receiving
 *          5..31 reserved
 **/
int i_flag = 0;
/**
 *  timer for canbus receiving
 **/
int i_canRxTmr = 0;
void USB_LP_CAN_RX0_IRQHandler(void)
{
    CanRxMsg RxMessage;
    u8 currSeq;

    while(0 != CAN_MessagePending(CAN_FIFO0))
    {
        //memset(&RxMessage, 0, sizeof(RxMessage));
        CAN_Receive(CAN_FIFO0, &RxMessage);
        
        g_canRevBuf.len = RxMessage.Data[1];
        currSeq = RxMessage.Data[3];
        
        /**
        * Note: 1、由于乱序的可能性较高，故而不能用入队列的标准接口
        *      2、当数据接收完成后, 组装成可直接供串口发送的数据
        *      3、
        **/
        memcpy(&g_canRevBuf.buf[currSeq << 2], &RxMessage.Data[4], 4);
        //memset(&g_canRevBuf.ctrl[currSeq << 2], CTRL_CONTI, 4);
        
        if(0x0 != bitmap_test_set(&bmpRcvCan, currSeq))
        {
            ;/** warning: the frame which specified NO. had been received again **/
        }
        
        if(bitmap_isfull(&bmpRcvCan, (g_canRevBuf.len + 3) >> 2))          /** 判断数据包结束. currpktSeq从0开始的序号， 加1后就是的1的个数  **/
        {
            bitmap_clean(&bmpRcvCan);
            /** ????????????????????????????????????????????????????????????????? **/
            #if 1
            g_canRevBuf.head = RxMessage.StdId;
            g_canRevBuf.tail = RxMessage.ExtId;
            #endif
            /** ????????????????????????????????????????????????????????????????? **/
        
            inq(&g_msgq, CMSG_CANRX);
            /***************************************************************************/
            GPIO_ResetBits(GPIOA, GPIO_Pin_0);
            /***************************************************************************/
            i_flag &=  ~(1 << 4);
        }
        else
        {
            /***************************************************************************/
            GPIO_SetBits(GPIOA, GPIO_Pin_0);
            /***************************************************************************/
            i_flag |=  (1 << 4);
        }
    }
}

/*******************************************************************************
* Function Name  : CAN_RX1_IRQHandler
* Description    : This function handles CAN RX1 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void CAN_RX1_IRQHandler(void)
{
    /** Same as USB_LP_CAN_RX0_IRQHandler ? **/
}

/*******************************************************************************
* Function Name  : CAN_SCE_IRQHandler
* Description    : This function handles CAN SCE interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void CAN_SCE_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : EXTI9_5_IRQHandler
* Description    : This function handles External lines 9 to 5 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void EXTI9_5_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : TIM1_BRK_IRQHandler
* Description    : This function handles TIM1 Break interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TIM1_BRK_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : TIM1_UP_IRQHandler
* Description    : This function handles TIM1 overflow and update interrupt 
*                  request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TIM1_UP_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : TIM1_TRG_COM_IRQHandler
* Description    : This function handles TIM1 Trigger and Commutation interrupts 
*                  requests.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TIM1_TRG_COM_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : TIM1_CC_IRQHandler
* Description    : This function handles TIM1 capture compare interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TIM1_CC_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : TIM2_IRQHandler
* Description    : This function handles TIM2 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
int comm_flag = 0;
/** 
 * bit3..0 计数
 * bit4 - uart2正在接收标志
 * bit5 - 过零中断产生在事实上的零点前后0.5ms范围内
 * bit6 .. - reserved
 **/
int Timer_comBusy = 0;      /** 计时 **/

void TIM2_IRQHandler(void)
{
    static unsigned char x10sig;
    u16 capture1 = 0;
    u16 capture4 = 0;
    u16 i;
    
    /* TIM2_CH1 toggling with frequency = 8000 Hz */
    if (TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_CC1 );
        capture1 = TIM_GetCapture1(TIM2);
        TIM_SetCompare1(TIM2, capture1 + CCR1_Val );
        
		#if	1
        for(i = 0; i < TIMER_NUM; i++)
        {
            if(g_timer[i].tick_bak > 0)
            {
                if(g_timer[i].tick > 0)
                {
                    g_timer[i].tick--;
                }
                else
                {
                    g_timer[i].tick = g_timer[i].tick_bak;
                    //g_timer[i].count++;
                    inq(&g_msgq, CMSG_TMR);
                }
            }
        }
        #endif
        /***************************************************/
        if(comm_flag & (1 <<4))    /** com方向100ms超时恢复 **/
        {
            Timer_comBusy++;
            if(Timer_comBusy >= (100 * 8))
            {
                Timer_comBusy = 0;
                comm_flag &= ~(1 <<4);
                
                inq(&g_msgq, CMSG_COMTIMEOUT);
            }
        }
        else
        {
            Timer_comBusy = 0;
        }
        /***************************************************/
        
        if(g_tmr_delay != 0)
        {
            g_tmr_delay--;
        }
        
        /***************************************************/
        if(i_flag & (1 <<4))    /** canbus方向100ms接收超时恢复 **/
        {
            if(i_canRxTmr++ >= (100 * 8))
            {
                bitmap_clean(&bmpRcvCan);
                i_flag &= ~(1 <<4);
                i_canRxTmr = 0;
            }
        }
        else
        {
            i_canRxTmr = 0;
        }
        /***************************************************/
        if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6))
        {
        	x10_zcross_timer = 0;
        }
        else 
        {
        	if(x10_zcross_timer++ > CTIMER_CALIB)
        	{
                inq(&g_msgq, CX10_CALIBR);
        		x10_zcross_timer = 0;
        	}
        }
        
        x10_zcross_count++;
        x10_timer++;
        switch(x10_status)
        {
        case CX10_IDLE:
        	
        	/** synchronous clock at every cycle in **/
        	if((x10_timer >= 160) && (EXTI15_count != 0))	/** from 80-x to 80...160, so it pass two cross-zero, and initial it at next cross-zero **/
        	{
        		EXTI15_count = 0;
        	}
        	
        	if(((x10_timer % 80) >= (80 - 2)) || ((x10_timer % 80) <= 3))	/** 数据采样时间, 在80n前后共5次 **/
        	{
        		X10_BitRead();
        	}
        	
			if((x10_timer % 80) == 3)	/** 本轮的最后一次采样后，是接收检测时间点 **/
        	{
        		/** read x10 data **/
        		x10_RcvXData <<= 1;
        		#if	DEBUG_NOARG
        			x10_RcvXData |= (Check1Count() >= 3);
        		#else
        			x10_RcvXData |= (Check1Count(x10_RcvBit) >= 3);
        			x10_RcvBit = 0;
				#endif

        		if((x10_RcvXData & 0x0F) == 0x0E)	/** startCode 0xE received **/
        		{
        			x10_status = CX10_RCV1;
        			x10_Rcv1stData = 0;
        			
        			X10RxLED_ON();
        		}
        		else 	/** 发送检测时间点。是否最合适　? **/
        		{
					/** 
        			 * 进入发送状态的条件(当前在空闲状态是前提条件)：
        			 *  		1.交流电过零时间点前0.5ms
        			 *			2.最近4个cycle没有收到1脉冲
        			 *			3.当前没有1脉冲
        			 *			4.有数据等待发送
        			 **/
        			if((0x0 == (x10_RcvXData & 0x01)) &&	/** 当前时刻没有信号 **/
        				(TRUE == X10QueueOut(&g_x10TxQue, &g_x10bakframe)))		/** 有数据等待发送 **/
					{
						x10_status = CX10_SSND;
						/** 
						 * 发送开始码1110的第一个120kHz脉冲? 或者只是进入发送开始码状态
						 *
						 * 如果start的第一个bit在80 - CDELTA_TIMER的时间点发出则(x10_timer % 80) == 0
						 * 如果start的第一个bit在160 - CDELTA_TIMER的时间点发出则(x10_timer % 80) == 1
						 * 具体是哪一种与电力线过零点的相位及外部中断的边延有关
						 *
						 * 当前情形是start的第一个bit在80 - CDELTA_TIMER的时间点发出．
						 *
						 **/
						 
						//X10f120KHZ_enable();
						
					}
        		}
        	}
        	else if((x10_timer % 80) == (80 - CDELTA_BC))
			{
			}
        	
        	break;
        case CX10_RCV1:
        	if(((x10_timer % 80) >= (80 - 2)) || ((x10_timer % 80) <= 3))	/** 数据采样时间, 在80n前后共5次 **/
        	{
        		X10_BitRead();
        	}
        		
        	if((x10_timer % 80) == 3)	/** 本周期最后一次采样已完成，开始查验数据 **/
        	{
        		/** read x10 data first **/
        		x10_RcvXData <<= 1;
        		#if	DEBUG_NOARG
        			x10_RcvXData |= (Check1Count() >= 3);
        		#else	
        			x10_RcvXData |= (Check1Count(x10_RcvBit) >= 3);
        			x10_RcvBit = 0;
        		#endif
        		
        		#if	1
        			/** check newest data if it is OK (1110xx,xxxx,xxxx,xxxx,xxxx) **/
        			if(((x10_RcvXData >> 18) & 0x0F) == 0x0E)
        			//if((x10_RcvXData & 0x03C0000) == 0x0380000)
        			{
        				x10_Rcv1stData = x10_RcvXData;
        				x10_status = CX10_RCV2;
        				EXTI15_count = 0;
        			}
        		#endif
        	}
        	
        	break;
        case CX10_RCV2:
			if(((x10_timer % 80) >= (80 - 2)) || ((x10_timer % 80) <= 3))	/** 数据采样时间, 在80n前后共5次 **/
        	{
        		X10_BitRead();
        	}
        	
        	if((x10_timer % 80) == 3)	/** 本周期最后一次采样已完成，开始查验数据 **/
        	{
        		/** read x10 data first **/
        		x10_RcvXData <<= 1;
        		#if	DEBUG_NOARG
        			x10_RcvXData |= (Check1Count() >= 3);
        		#else	
        			x10_RcvXData |= (Check1Count(x10_RcvBit) >= 3);
        			x10_RcvBit = 0;
        		#endif
        		/** check newest data if it is 0xE (1110xx,xxxx,xxxx,xxxx,xxxx) **/
        		if(((x10_RcvXData >> 18) & 0x0F) == 0x0E)
        		{
        			/** Received OK. **/
        			if(((x10_RcvXData ^ x10_Rcv1stData) & 0x3FFFFF) == 0)
        			{
        				/** data is perfect **/
        				if((x10_RcvXData & 0x03) == 0x02)	/** end with 0bxxx10, functionCode **/
        				{
        					g_x10Revframe.funcCode = x10_RcvXData;
        					inq(&g_msgq, CMSG_X10RX);
        				}
        				else if((x10_RcvXData & 0x03) == 0x01)	/** end with 0bxxx01, addressCode **/
        				{
        					g_x10Revframe.addrCode = x10_RcvXData;
        					g_x10Revframe.addrCode |= 0x01;		/** the address is valid **/
        				}
        				else 
        				{
        					/** error **/
        				}
        				//GPIO_ResetBits(GPIOA, GPIO_Pin_1);
        				X10RxLED_OFF();
        				x10_RcvXData = 0;
        			}
        			else
        			{
        				/** error happend, two data are different, give up the data or ... **/
        			}
        			
        			x10_status = CX10_IDLE;
        			x10_RcvXData = 0;
        		}
        		else if((x10_timer / 80) > 28)	/** 错误场景，自主恢复到空闲状态(在26次过零还没有完成RCV2) **/
        		{
        			//GPIO_ResetBits(GPIOA, GPIO_Pin_1);
        			X10RxLED_OFF();
        			x10_status = CX10_IDLE;
        			x10_RcvXData = 0;
        		}
        	}
        	
        	break;
        	
        case CX10_SSND:		/** start send 1110 **/
        	//if((x10_timer % 80) == CDELTA_AD)	/** pluse endPoint **/
        	if(((x10_timer % 80) == CDELTA_AD) || 
        		((x10_timer % 80) == (CDELTA_AD + 27)) ||
        	 	((x10_timer % 80) == (CDELTA_AD + 54)))		/** pluse endPoint **/
        	{
        		//X10_disable();	/** disable the 120kHz pulse, no matter the x10status value**/
        		X10f120KHZ_disable();
        	}
        	//else if((x10_timer % 80) == (80 - CDELTA_BC))
        	else if(((x10_timer % 80) == (80 - CDELTA_BC)) || 
        			((x10_timer % 80) == (80 - CDELTA_BC + 27)) || 
        			((x10_timer % 80) == (80 - CDELTA_BC + 54)))
        	{
        		if(((x10_timer / 80) == 1) || 
        			((x10_timer / 80) == 2) || 
        			((x10_timer / 80) == 3) || 
        			((x10_timer / 80) == 29) || 
        			((x10_timer / 80) == 30) || 
        			((x10_timer / 80) == 31))	/** 1st, 2nd, 3rd are 1**/
        		{
        			X10f120KHZ_enable();
        			
        		}
        		else if(((x10_timer / 80) == 4) ||
        			((x10_timer / 80) == 32))	/** 4th is 0 **/
        		{
					x10_status = CX10_DSND;
					
					memcpy(&g_x10Sndframe, &g_x10bakframe, sizeof(g_x10Sndframe));
        		}
        	}
        	/** 
        	 * 中断触发时刻在实际的过零点前(后)0.5ms(实际上是CDELTA_BC * 0.125)范围内,需要特殊处理
        	 * 由于处理过程中已经过了CDELTA_BC * 0.125时刻，所以在正常处理过程中开始码的第一个120kHz
        	 * 脉冲信号并没有启动，需要在这里启动之。
        	 *
        	***/
        	else if((x10_timer == (1 + 80 - g_x10_sync_offset)) && (comm_flag & (1 << 5)))
        	{
        		X10f120KHZ_disable();
        	}
        	
        	break;        

        case CX10_DSND:	
        	
        	//if((x10_timer % 80) == CDELTA_AD)
        	if(((x10_timer % 80) == CDELTA_AD) || 
        		((x10_timer % 80) == CDELTA_AD + 27) || 
        		((x10_timer % 80) == CDELTA_AD + 54))
        	{
        		X10f120KHZ_disable();		/** disable the 120kHz pulse, no matter the x10status value**/
        		
        		if((x10_timer / 80) == 23)	/** 发完第22bit后可暂停3个cycle共6个bit的时间 **/
        		{
					x10_status = CX10_PSND;
					
        		}
        		
        		if((x10_timer / 80) == 51)	/** 发完第22bit后可暂停3个cycle共6个bit的时间 **/
        		{
        			x10_status = CX10_PSND;
        			
        		}
        	}
        	//else if((x10_timer % 80) == (80 - CDELTA_BC))
        	else if(((x10_timer % 80) == (80 - CDELTA_BC)) || 
        		((x10_timer % 80) == (80 - CDELTA_BC + 27)) || 
        		((x10_timer % 80) == (80 - CDELTA_BC + 54)))
        	{
        		if((((x10_timer / 80) >= 5) && ((x10_timer / 80) <= 12)) ||
        			(((x10_timer / 80) >= 33) && ((x10_timer / 80) <= 40)))		/** 发house码共8bit **/
        		{
        			if(((x10_timer / 80) & 1) == 1)			/** 发送原码 **/
        			{
        				if(g_x10Sndframe.houseCode & 0x08)
        				{
        					
        	
        					X10f120KHZ_enable();
        				}
        				else
        				{
        					/** send 0 **/
        				}
        			}
        			else	/** 发送反码 **/
        			{
        				if(g_x10Sndframe.houseCode & 0x08)
        				{
        					/** send 0 **/
        				}
        				else
        				{
        					//X10_enable();
        					X10f120KHZ_enable();
        					
        				}
        				
        				g_x10Sndframe.houseCode <<= 1;
        			}
        		}
        		else if((((x10_timer / 80) >= 13) && ((x10_timer / 80) <= 22)) ||	
        				(((x10_timer / 80) >= 41) && ((x10_timer / 80) <= 50)))	/** 发key码或功能码共10bit **/
        		{
        			if(((x10_timer / 80) & 1) == 1)	/** 发送原码 **/
        			{
        				if(g_x10Sndframe.keyCode & 0x10)
        				{
        					//X10_enable();
        					X10f120KHZ_enable();
        				}
        				else
        				{
        					/** send 0 **/
        				}
        			}
        			else	/** 发送反码 **/
        			{
        				if(g_x10Sndframe.keyCode & 0x10)
        				{
        					/** send 0 **/
        				}
        				else
        				{
        					//X10_enable();
        					X10f120KHZ_enable();
        				}
        				
        				g_x10Sndframe.keyCode <<= 1;		/** 为发送下一个bit做准备 **/
        			}
        		}
        	}
        	
        	break;        
        case CX10_PSND:
        	
        	if(((x10_timer / 80) == 28) && ((x10_timer % 80) > 0))
        	{
        		x10_status = CX10_SSND;		/** 再次进入发送状态 **/
        	}
        	else if((x10_timer / 80) >= 51)
        	{
        		if(((x10_timer % 80) >= (80 - 2)) || ((x10_timer % 80) <= 2))	/** 数据采样时间, 在80n前后共5次 **/
        		{
        			X10_BitRead();
        		}
        		
        		if((x10_timer % 80) == 2)
        		{
        			/** read x10 data first **/
        			
        			#if	DEBUG_NOARG
        				if(Check1Count() >= 3)	/** 5次采样，3次以上收到1 **/
        			#else
        				if(Check1Count(x10_RcvBit) >= 3)	/** 5次采样，3次以上收到1 **/
        			#endif
        			{
        				x10_RcvXData <<= 1;
        				x10_RcvXData |= 1;
        				
        				x10_status = CX10_IDLE;	
        				#if	DEBUG_NOARG
        				#else
        					x10_RcvBit = 0;
        				#endif
        			}
        		}
        		
        		if((x10_timer / 80) >= 56)				/** 发送完毕 **/
        		{
        			x10_status = CX10_IDLE;	
        			
        		}
        	}
        	break;
        		
        default:
        	break;
        }
        /***************************************************/
    }
    
    /*** toggling with frequency = 120kHz ***/
	if (TIM_GetITStatus(TIM2, TIM_IT_CC4) != RESET) 
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_CC4);
		capture4 = TIM_GetCapture4(TIM2);
		TIM_SetCompare4(TIM2, capture4 + CCR4_Val);
		#if	1
		if(x10sig++ & 1)
			GPIO_SetBits(GPIOA, GPIO_Pin_5);
		else
			GPIO_ResetBits(GPIOA, GPIO_Pin_5); 
		#endif
	}
	 
}

/*******************************************************************************
* Function Name  : TIM3_IRQHandler
* Description    : This function handles TIM3 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TIM3_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : TIM4_IRQHandler
* Description    : This function handles TIM4 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void TIM4_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : I2C1_EV_IRQHandler
* Description    : This function handles I2C1 Event interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void I2C1_EV_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : I2C1_ER_IRQHandler
* Description    : This function handles I2C1 Error interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void I2C1_ER_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : I2C2_EV_IRQHandler
* Description    : This function handles I2C2 Event interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void I2C2_EV_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : I2C2_ER_IRQHandler
* Description    : This function handles I2C2 Error interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void I2C2_ER_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : SPI1_IRQHandler
* Description    : This function handles SPI1 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPI1_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : SPI2_IRQHandler
* Description    : This function handles SPI2 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPI2_IRQHandler(void)
{
}

/*******************************************************************************
 * Function Name  : USART1_IRQHandler
 * Description    : This function handles USART1 global interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 ******************************************************************************/
void USART1_IRQHandler(void)
{
}

/*******************************************************************************
 * Function Name  : USART2_IRQHandler
 * Description    : This function handles USART2 global interrupt request.
 * Input          : None
 * Output         : None
 * Return         : None
 *
 * Note: 由于串口数据的起始和结束由外部i/o准定，故而考虑以下情况
 *        每收一个字节后，查外部接口，如果是low, 就认为是同一组数据，如果是high, 就认为是结束
 *******************************************************************************/
void USART2_IRQHandler(void)
{
    charData_t charData;
    
    /** 收到USART2数据 **/
    if(USART_GetFlagStatus(USART2, USART_IT_RXNE) == SET)
    {
      #if 1
        charData.ucVal = USART_ReceiveData(USART2);
        charQueueIn(&g_comRevBuf, &charData);
        
        comm_flag |= (1 <<4);
        if(charData.ucVal == ASCII_STX)
        {
            
        }
        else if(charData.ucVal == ASCII_ETX)
        {
            comm_flag &= ~(1 <<4);
            
            inq(&g_msgq, CMSG_COMRX);           /** 收到一组完整数据才发消息 **/
        }
        else
        {
            /** Do nothing **/
        }
      #endif
    }

    /** 完成发送USART1数据 **/
    if(USART_GetFlagStatus(USART2, USART_IT_TC) == SET)
    {
        inq(&g_msgq, CMSG_COMTX);	
    }
}

/*******************************************************************************
* Function Name  : USART3_IRQHandler
* Description    : This function handles USART3 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USART3_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : EXTI15_10_IRQHandler
* Description    : This function handles External lines 15 to 10 interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void EXTI15_10_IRQHandler(void)
{
/**
 * Ext interrupt occured every cycle, but we get synchronous clock cycle by 8kHz timer
 * we can correct sycnhronous clock after 
 **/	
	if(EXTI_GetITStatus(EXTI_Line13) != RESET)
	{
		/** The 13th external interrupt happend! **/
		
		x10_zcross_count = 0;
		
		/** synchronous clock **/
		if(EXTI15_count++ == 0)
		{
			/**
			 * 修正zero-cross时刻 
			 * 
			 * 实测得：外部中断1ms(也就是8个时钟)之后, 有信号出现在信号输入引脚上. 此时距信号的中心(zero-cross)有1ms/2(也就是4个时钟)的时间差。
			 * 
			 * 所以, 当前是外部中断发生再(8+4=)12次计数之后, 即为zero-cross时刻
			 **/
			x10_timer = 80 - g_x10_sync_offset;		
			
			if(g_x10_sync_offset < CDELTA_BC)
			{
				comm_flag |= (1 << 5);
			}
		}
		/* Clear the EXTI line 13 pending bit */
		EXTI_ClearITPendingBit(EXTI_Line13);
	}
	
	if(EXTI_GetITStatus(EXTI_Line12) != RESET)		/** zero-cross calibration **/
	{
		
		g_x10_zcross_calibration = (x10_zcross_count % 80); 
		g_x10_zcross_calibration = (g_x10_zcross_calibration > 40) ? (80 - g_x10_zcross_calibration):g_x10_zcross_calibration;
        inq(&g_msgq, CMSG_SAVE);           			/** 收到一组完整数据才发消息 **/
        
		/* Clear the EXTI line 12 pending bit */
		EXTI_ClearITPendingBit(EXTI_Line12);
	}
}

/*******************************************************************************
* Function Name  : RTCAlarm_IRQHandler
* Description    : This function handles RTC Alarm interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void RTCAlarm_IRQHandler(void)
{
}

/*******************************************************************************
* Function Name  : USBWakeUp_IRQHandler
* Description    : This function handles USB WakeUp interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBWakeUp_IRQHandler(void)
{
}

/******************* (C) COPYRIGHT 2007 STMicroelectronics *****END OF FILE****/
