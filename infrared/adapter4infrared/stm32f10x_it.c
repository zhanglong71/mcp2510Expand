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
#include "inc/macro.h"
#include "inc/global.h"
#include "src/arch.h"
#include "src/bitmapOp.h" 
#include "src/charQue.h" 
//#include "src/X10Que.h"

#include "inc/debug.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

//vu16 CCR1_Val = 0xF0;			/** f = 100kHz **/ 
//vu16 CCR1_Val = 0x177;			/** f = 64kHz **/
//vu16 CCR1_Val = 0x1E0;			/** f = 50kHz **/
//vu16 CCR1_Val = 0x2EE;			/** f = 32kHz **/	  
vu16 CCR1_Val = 0x3C0;			/** f = 25kHz **/
//vu16 CCR1_Val = 0x5DC;			/** f = 16kHz **/	 
//vu16 CCR1_Val = 0x780;			/** f = 12.5kHz **/
//vu16 CCR1_Val = 0xBB8;			/** f = 8kHz **/
//vu16 CCR1_Val = 0x1770;			/** f = 4kHz **/
//vu16 CCR1_Val = 0x2EE1;       /** 非常接近1000Hz, 示波器手工测量2f得1000Hz, 自动显示499.984 **/
//vu16 CCR1_Val = 0x2EE0;       /** 非常接近1000Hz, 示波器手工测量2f得1000Hz, 自动显示500.026 **/

//vu16 CCR4_Val = 0x177;			/** 128kHz **/
vu16 CCR4_Val = 0xC8;			/** 示波器手工测量得: f = 119.99kHz **/


charBuf_queue_t g_canRevBuf;             /** data fragment **/
charBuf_queue_t g_comRevBuf;             /** data fragment **/
//charBuf_queue_t g_wlsRevBuf;             /** data fragment **/

/** 
 * note注意：对此全局数据的处理没有同步保护，因为收一个x10数据帧需要0.5s以上，时间足够
 * 在处理完接收到的x10数据之前，不可能会再次收到一个22bit的x10数据帧
 * 
 **/
 
bitmap_t    bmpRcvCan;

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
 *          5 - sending infrared signal
 *			6 - recording infrared signal
 *			7 - PA4 last leavel
 *			8 - PA4 start invert
 *			9 - infrared signal record ok		 
 *			10..31 reserved
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
        *      	2、当数据接收完成后, 组装成可直接供串口发送的数据
        *      	3、
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
            //GPIO_ResetBits(GPIOA, GPIO_Pin_0);
            MCANLED_ON();
            /***************************************************************************/
            i_flag &=  ~(1 << 4);
        }
        else
        {
            /***************************************************************************/
            //GPIO_SetBits(GPIOA, GPIO_Pin_0);
            MCANLED_OFF();
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
int Timer_INFLED = 0;      /** 红外LED闪烁计时 **/
int Timer_infrared = 0;     /** 录入超时计时 **/
int Timer_infrared_phase = 0;     	/** 发送相位计时数 **/
int Timer_infrared_sampling = 0;    /** 采样相位计时数 **/
int Timer_infrared_sampling_timeout = 0;     /** 录入超时计时 **/
int Phase_sampling_idx = 0;     		/** 采样相位编号 **/
int Phase_sending_idx = 0;     		/** 发送相位编号 **/
Phase_sampling_t Phase_sampling;
	/** g_Phase_sending 在主循环和时钟中断里面都有写操作。使用场景限制可保证其写数据过程不发生冲突 **/
Phase_sampling_t g_Phase_sending;
//int Phase_samplingGuide[16] = 0;     	/** 引导码计数(奇数序号与偶数序号的数据有别) **/
//unsigned char Phase_sampling[64] = 0;  	/** 采样计数暂存 **/

void TIM2_IRQHandler(void)
{
//	charData_t chardata;
    u16 capture1 = 0;
//    u16 capture4 = 0;
    u16 i;
    
    /* TIM2_CH1 toggling with frequency = 8000 Hz */
    if (TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_CC1 );
        capture1 = TIM_GetCapture1(TIM2);
        TIM_SetCompare1(TIM2, capture1 + CCR1_Val);
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
        if(comm_flag & (1 <<4)) {    /** com方向100ms超时恢复 **/
        
            Timer_comBusy++;
            if(Timer_comBusy >= (100 * CTIMER_1MS)) {
                Timer_comBusy = 0;
                comm_flag &= ~(1 <<4);
                
                inq(&g_msgq, CMSG_COMTIMEOUT);
            }
        } else {
            Timer_comBusy = 0;
        }
        /***************************************************/
        if(g_tmr_delay != 0) {
            g_tmr_delay--;
        }
        /***************************************************/
        if(i_flag & (1 <<4)) {    /** canbus方向100ms接收超时恢复 **/
            if(i_canRxTmr++ >= (100 * 8))  {
                bitmap_clean(&bmpRcvCan);
                i_flag &= ~(1 <<4);
                i_canRxTmr = 0;
            }
        } else {
            i_canRxTmr = 0;
        }
        /***************************************************/
        #if	0	/** debug **/
        	if(MINFRARED_READ() == 1) {
        		MDEBUG_PA7(1);
        	} else {
        		MDEBUG_PA7(0);
        	}
        #else
        /** 录入红外信号的优先级高于发送红外信号的优先级 **/
        if(Timer_infrared < CTIMER_INFRARED_TIMEOUT)
        {
       		Timer_infrared++;
    	}
        if((i_flag & (1 << 5)) != 0) {			//工作进行中
       		charQueueInit(&g_infraredTxQue);	/** ignore all data while sending/record remote signal **/
        	if(Timer_infrared >= CTIMER_INFRARED_RECORDTIMEOUT) {	//录入动作10s超时
        		i_flag &=  ~(1 << 6);
        	} 
        								//工作进行且确认需要继续进行
        	if((i_flag & (1 << 6)) != 0) {	/** 录入状态 **/
        		if((i_flag & (1 << 8)) != 0) {			//录入信号发生翻转，采样计时开始
        			Timer_infrared_sampling++;		
        		}
        		/** 录入状态指示灯闪烁, H/L = 100ms为周期 **/
        		Timer_INFLED++;
        		if(Timer_INFLED >= CTIMER_100MS) {
        			Timer_INFLED = 0;
        			MINFLED_INVERSE();	/** 等待录入-LED指示工作 **/
        		}
        		
        		if((i_flag & (1 << 7)) == 0) {	//上次为低  
        			if(MINFRARED_READ() == 1) {	//检测为高.L==>H
        						
        				Phase_sampling_idx = (Phase_sampling_idx < (CSAMPLING_BIT_LEN - 1))? Phase_sampling_idx:(CSAMPLING_BIT_LEN - 1);
        				if(Phase_sampling_idx == 0) {
        					/** 第一次只是开始， 数据 **/
        				} else if(Phase_sampling_idx < 3) {
        					Phase_sampling.head[Phase_sampling_idx - 1] = Timer_infrared_sampling;
        				} else {
        					Phase_sampling.buf[Phase_sampling_idx - 3] = Timer_infrared_sampling;
        				}
        				
        				i_flag |= ((1 << 7) | (1 << 8));  
        				Phase_sampling_idx++;
        				Timer_infrared_sampling = 0;
        			} else {
        				/** L==>L **/
        				#if	0
        				if((i_flag & (1 << 8)) != 0) {
        					if(Timer_infrared_sampling > CTIMER_INFRARED_SAMPLING_MAX) {	//相位采样超过最大次数。采样过程结束
        						i_flag &= ~((1 << 6) | (1 << 8));
                        		i_flag |= (1 << 9);				 	/** record ok **/
								
								Phase_sampling_idx = 0;
								Timer_infrared_phase = ~0;
        					}
        				}
						#endif
        			}
        		} else {						//上次为高
        			if(MINFRARED_READ() == 0) {	//检测为低.H==>L
							   
        				Phase_sampling_idx = (Phase_sampling_idx < (CSAMPLING_BIT_LEN - 1))? Phase_sampling_idx:(CSAMPLING_BIT_LEN - 1);
        				if(Phase_sampling_idx == 0) {
        					/** 第一次只是开始， 没有采样数据 **/
        				} else if(Phase_sampling_idx < 3) {
        					Phase_sampling.head[Phase_sampling_idx - 1] = Timer_infrared_sampling;
        				} else {
        					Phase_sampling.buf[Phase_sampling_idx - 3] = Timer_infrared_sampling;
        				}
        				
        				i_flag |= (1 << 8);
        				i_flag &=  ~(1 << 7); 
        				Phase_sampling_idx++;
        				Timer_infrared_sampling = 0;
        			} else {
        				/** H==>H **/
        				if((i_flag & (1 << 8)) != 0) {
        					if(Timer_infrared_sampling > CTIMER_INFRARED_SAMPLING_MAX) {	//相位采样超过最大次数。采样过程结束
        						if(Phase_sampling_idx > (16 + 2)) {	/** 16bpb, 另加2bit的开始，否则视同干扰 **/
        							i_flag &= ~((1 << 6) | (1 << 8));
        							i_flag |= (1 << 9);				 	/** record ok **/
        							
								 	#if	1
        							memcpy(g_Phase_sending.buf, Phase_sampling.buf, CSAMPLING_BIT_LEN);
        							memcpy(g_Phase_sending.head, Phase_sampling.head, sizeof(Phase_sampling.head));
        							
        							memcpy(&(g_Phase_sending.buf[CHEADER_OF_ENTRY]), Phase_sampling.head, sizeof(Phase_sampling.head));
        							g_Phase_sending.buf[CSAMPLING_BIT_LEN] = Phase_sampling_idx - 2;		/** length **/
        							g_Phase_sending.buf[CSAMPLING_BIT_VALID] = CENTRYFLAG_BUSY;				/** valid **/
        							#endif

									Phase_sampling_idx = 0;
									Timer_infrared_phase = ~0;
        						} else {
        							i_flag &= ~((1 << 6) | (1 << 5));
        						}
        					}
        				}
        			}
        		}        		
        	} else { /** 发送状态 **/
        		Mchannel(g_Phase_sending.addr[0]);	/** 指定遥控器对应的通道 **/
        		/** 发送状态指示灯闪烁, H/L = 30ms为周期 **/
        		Timer_INFLED++;
        		if(Timer_INFLED >= CTIMER_30MS) {
        			Timer_INFLED = 0;
        			MINFLED_INVERSE();	/** 等待录入-LED指示工作 **/
        		}
        			 
        		if(Timer_infrared_phase <= 0) {
        			if((g_Phase_sending.buf[CSAMPLING_BIT_VALID] == CENTRYFLAG_BUSY) 
        							&& (g_Phase_sending.buf[CSAMPLING_BIT_LEN] > 0)) {	/** 此项有效(长度大于0)吗 **/
        				if(Phase_sending_idx < 2) {	   							/** 发送命令头 **/
        					Timer_infrared_phase = g_Phase_sending.head[Phase_sending_idx];
        					Phase_sending_idx++;
        					MINFRARED_SIGNAL_INVERSE();
        				} else if(Phase_sending_idx < (CSAMPLING_BIT_LEN - 1)) {/** 发送信号数据 **/
        					Timer_infrared_phase = g_Phase_sending.buf[Phase_sending_idx - 2];
        					/** 数据发送完毕的条件: 读出非法数据 **/
							if((Timer_infrared_phase <= 0) || (Timer_infrared_phase >= CTIMER_INFRARED_SAMPLING_MAX)) {
        						if((i_flag & (1 << 9)) != 0) {	/** 有新录入的数据需要写入? **/
        							i_flag &= ~(1 << 9);
        							inq(&g_msgq, CINFR_RCV);
        							 
        						}
        						i_flag &= ~(1 << 5);
        						MINFLED_OFF();
        						MINFRARED_IDLE();
        						Mchannel(0xff);
        					} else {	/** 正常范围, 发送下一个电平 **/
								Phase_sending_idx++;
        						MINFRARED_SIGNAL_INVERSE();
							}
        				} else {	/** 超出范围, 正常不会运行到这里 **/
							MINFLED_OFF();
							i_flag &= ~(1 << 5);
						}
        			} else {
        				i_flag &= ~(1 << 5); 
        			}															                      
        		} else {
        			Timer_infrared_phase--;
        		}
        	}
    	} else {
    		if(TRUE != isCharQueEmpty(&g_infraredTxQue)) {		/** 收到命令，准备录入信号及发送信号 **/

    			Timer_infrared = 0;
        		i_flag |= (1 << 5);			/** 工作(发送)标志 **/
        		MINFRARED_IDLE();
        		
    			Phase_sending_idx = 0;
    			Timer_infrared_phase = 0;
    			
        		if(MINFRARED_IsRECORDING()) {		/** 如果此时是录入状态，就设标志 **/
        		//if(MINFRARED_IsRECORDING() && 0) {	/** for test only **/
    				Phase_sampling_idx = 0;
    				Timer_infrared_sampling = 0;
    				memset(Phase_sampling.buf, 0, sizeof(Phase_sampling.buf));
    				
        			i_flag |=  (1 << 6);		/** 录入状态标志 **/
        			if(MINFRARED_READ() == 1) {
        				i_flag |=  (1 << 7);
        			} else {
        				i_flag &=  ~(1 << 7);
        			}
        			i_flag &=  ~((1 << 8) | (1 << 9));
    			}
        	} else {MINFRARED_IDLE();}
    	}
    	#endif
        /***************************************************/
    }
    #if	0
	if (TIM_GetITStatus(TIM2, TIM_IT_CC4) != RESET) 
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_CC4);
		capture4 = TIM_GetCapture4(TIM2);
		TIM_SetCompare4(TIM2, capture4 + CCR4_Val);
	
			//X10f120KHZ_disable();
		
	}
	#endif
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
		
		/** synchronous clock **/
		#if	0
		if(EXTI15_count++ == 0)
		{
			/**
			 * 修正zero-cross时刻 
			 * 
			 * 实测得：外部中断1ms(也就是8个时钟)之后, 有信号出现在信号输入引脚上. 此时距信号的中心(zero-cross)有1ms/2(也就是4个时钟)的时间差。
			 * 
			 * 所以, 当前是外部中断发生再(8+4=)12次计数之后, 即为zero-cross时刻
			 **/
			
		}
		#endif
		/* Clear the EXTI line 13 pending bit */
		EXTI_ClearITPendingBit(EXTI_Line13);
	}
	
	if(EXTI_GetITStatus(EXTI_Line12) != RESET)		/** zero-cross calibration **/
	{
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
