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
//vu16 CCR1_Val = 0x2EE1;       /** �ǳ��ӽ�1000Hz, ʾ�����ֹ�����2f��1000Hz, �Զ���ʾ499.984 **/
//vu16 CCR1_Val = 0x2EE0;       /** �ǳ��ӽ�1000Hz, ʾ�����ֹ�����2f��1000Hz, �Զ���ʾ500.026 **/

//vu16 CCR4_Val = 0x177;			/** 128kHz **/
vu16 CCR4_Val = 0xC8;			/** ʾ�����ֹ�������: f = 119.99kHz **/


charBuf_queue_t g_canRevBuf;             /** data fragment **/
charBuf_queue_t g_comRevBuf;             /** data fragment **/
//charBuf_queue_t g_wlsRevBuf;             /** data fragment **/

/** 
 * noteע�⣺�Դ�ȫ�����ݵĴ���û��ͬ����������Ϊ��һ��x10����֡��Ҫ0.5s���ϣ�ʱ���㹻
 * �ڴ�������յ���x10����֮ǰ�������ܻ��ٴ��յ�һ��22bit��x10����֡
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
        * Note: 1����������Ŀ����Խϸߣ��ʶ�����������еı�׼�ӿ�
        *      	2�������ݽ�����ɺ�, ��װ�ɿ�ֱ�ӹ����ڷ��͵�����
        *      	3��
        **/
        memcpy(&g_canRevBuf.buf[currSeq << 2], &RxMessage.Data[4], 4);
        //memset(&g_canRevBuf.ctrl[currSeq << 2], CTRL_CONTI, 4);
        
        if(0x0 != bitmap_test_set(&bmpRcvCan, currSeq))
        {
            ;/** warning: the frame which specified NO. had been received again **/
        }
        
        if(bitmap_isfull(&bmpRcvCan, (g_canRevBuf.len + 3) >> 2))          /** �ж����ݰ�����. currpktSeq��0��ʼ����ţ� ��1����ǵ�1�ĸ���  **/
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
 * bit3..0 ����
 * bit4 - uart2���ڽ��ձ�־
 * bit5 - �����жϲ�������ʵ�ϵ����ǰ��0.5ms��Χ��
 * bit6 .. - reserved
 **/
int Timer_comBusy = 0;      /** ��ʱ **/
int Timer_INFLED = 0;      /** ����LED��˸��ʱ **/
int Timer_infrared = 0;     /** ¼�볬ʱ��ʱ **/
int Timer_infrared_phase = 0;     	/** ������λ��ʱ�� **/
int Timer_infrared_sampling = 0;    /** ������λ��ʱ�� **/
int Timer_infrared_sampling_timeout = 0;     /** ¼�볬ʱ��ʱ **/
int Phase_sampling_idx = 0;     		/** ������λ��� **/
int Phase_sending_idx = 0;     		/** ������λ��� **/
Phase_sampling_t Phase_sampling;
	/** g_Phase_sending ����ѭ����ʱ���ж����涼��д������ʹ�ó������ƿɱ�֤��д���ݹ��̲�������ͻ **/
Phase_sampling_t g_Phase_sending;
//int Phase_samplingGuide[16] = 0;     	/** ���������(���������ż����ŵ������б�) **/
//unsigned char Phase_sampling[64] = 0;  	/** ���������ݴ� **/

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
        if(comm_flag & (1 <<4)) {    /** com����100ms��ʱ�ָ� **/
        
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
        if(i_flag & (1 <<4)) {    /** canbus����100ms���ճ�ʱ�ָ� **/
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
        /** ¼������źŵ����ȼ����ڷ��ͺ����źŵ����ȼ� **/
        if(Timer_infrared < CTIMER_INFRARED_TIMEOUT)
        {
       		Timer_infrared++;
    	}
        if((i_flag & (1 << 5)) != 0) {			//����������
       		charQueueInit(&g_infraredTxQue);	/** ignore all data while sending/record remote signal **/
        	if(Timer_infrared >= CTIMER_INFRARED_RECORDTIMEOUT) {	//¼�붯��10s��ʱ
        		i_flag &=  ~(1 << 6);
        	} 
        								//����������ȷ����Ҫ��������
        	if((i_flag & (1 << 6)) != 0) {	/** ¼��״̬ **/
        		if((i_flag & (1 << 8)) != 0) {			//¼���źŷ�����ת��������ʱ��ʼ
        			Timer_infrared_sampling++;		
        		}
        		/** ¼��״ָ̬ʾ����˸, H/L = 100msΪ���� **/
        		Timer_INFLED++;
        		if(Timer_INFLED >= CTIMER_100MS) {
        			Timer_INFLED = 0;
        			MINFLED_INVERSE();	/** �ȴ�¼��-LEDָʾ���� **/
        		}
        		
        		if((i_flag & (1 << 7)) == 0) {	//�ϴ�Ϊ��  
        			if(MINFRARED_READ() == 1) {	//���Ϊ��.L==>H
        						
        				Phase_sampling_idx = (Phase_sampling_idx < (CSAMPLING_BIT_LEN - 1))? Phase_sampling_idx:(CSAMPLING_BIT_LEN - 1);
        				if(Phase_sampling_idx == 0) {
        					/** ��һ��ֻ�ǿ�ʼ�� ���� **/
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
        					if(Timer_infrared_sampling > CTIMER_INFRARED_SAMPLING_MAX) {	//��λ�����������������������̽���
        						i_flag &= ~((1 << 6) | (1 << 8));
                        		i_flag |= (1 << 9);				 	/** record ok **/
								
								Phase_sampling_idx = 0;
								Timer_infrared_phase = ~0;
        					}
        				}
						#endif
        			}
        		} else {						//�ϴ�Ϊ��
        			if(MINFRARED_READ() == 0) {	//���Ϊ��.H==>L
							   
        				Phase_sampling_idx = (Phase_sampling_idx < (CSAMPLING_BIT_LEN - 1))? Phase_sampling_idx:(CSAMPLING_BIT_LEN - 1);
        				if(Phase_sampling_idx == 0) {
        					/** ��һ��ֻ�ǿ�ʼ�� û�в������� **/
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
        					if(Timer_infrared_sampling > CTIMER_INFRARED_SAMPLING_MAX) {	//��λ�����������������������̽���
        						if(Phase_sampling_idx > (16 + 2)) {	/** 16bpb, ���2bit�Ŀ�ʼ��������ͬ���� **/
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
        	} else { /** ����״̬ **/
        		Mchannel(g_Phase_sending.addr[0]);	/** ָ��ң������Ӧ��ͨ�� **/
        		/** ����״ָ̬ʾ����˸, H/L = 30msΪ���� **/
        		Timer_INFLED++;
        		if(Timer_INFLED >= CTIMER_30MS) {
        			Timer_INFLED = 0;
        			MINFLED_INVERSE();	/** �ȴ�¼��-LEDָʾ���� **/
        		}
        			 
        		if(Timer_infrared_phase <= 0) {
        			if((g_Phase_sending.buf[CSAMPLING_BIT_VALID] == CENTRYFLAG_BUSY) 
        							&& (g_Phase_sending.buf[CSAMPLING_BIT_LEN] > 0)) {	/** ������Ч(���ȴ���0)�� **/
        				if(Phase_sending_idx < 2) {	   							/** ��������ͷ **/
        					Timer_infrared_phase = g_Phase_sending.head[Phase_sending_idx];
        					Phase_sending_idx++;
        					MINFRARED_SIGNAL_INVERSE();
        				} else if(Phase_sending_idx < (CSAMPLING_BIT_LEN - 1)) {/** �����ź����� **/
        					Timer_infrared_phase = g_Phase_sending.buf[Phase_sending_idx - 2];
        					/** ���ݷ�����ϵ�����: �����Ƿ����� **/
							if((Timer_infrared_phase <= 0) || (Timer_infrared_phase >= CTIMER_INFRARED_SAMPLING_MAX)) {
        						if((i_flag & (1 << 9)) != 0) {	/** ����¼���������Ҫд��? **/
        							i_flag &= ~(1 << 9);
        							inq(&g_msgq, CINFR_RCV);
        							 
        						}
        						i_flag &= ~(1 << 5);
        						MINFLED_OFF();
        						MINFRARED_IDLE();
        						Mchannel(0xff);
        					} else {	/** ������Χ, ������һ����ƽ **/
								Phase_sending_idx++;
        						MINFRARED_SIGNAL_INVERSE();
							}
        				} else {	/** ������Χ, �����������е����� **/
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
    		if(TRUE != isCharQueEmpty(&g_infraredTxQue)) {		/** �յ����׼��¼���źż������ź� **/

    			Timer_infrared = 0;
        		i_flag |= (1 << 5);			/** ����(����)��־ **/
        		MINFRARED_IDLE();
        		
    			Phase_sending_idx = 0;
    			Timer_infrared_phase = 0;
    			
        		if(MINFRARED_IsRECORDING()) {		/** �����ʱ��¼��״̬�������־ **/
        		//if(MINFRARED_IsRECORDING() && 0) {	/** for test only **/
    				Phase_sampling_idx = 0;
    				Timer_infrared_sampling = 0;
    				memset(Phase_sampling.buf, 0, sizeof(Phase_sampling.buf));
    				
        			i_flag |=  (1 << 6);		/** ¼��״̬��־ **/
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
 * Note: ���ڴ������ݵ���ʼ�ͽ������ⲿi/o׼�����ʶ������������
 *        ÿ��һ���ֽں󣬲��ⲿ�ӿڣ������low, ����Ϊ��ͬһ�����ݣ������high, ����Ϊ�ǽ���
 *******************************************************************************/
void USART2_IRQHandler(void)
{
    charData_t charData;
    
    /** �յ�USART2���� **/
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
            
            inq(&g_msgq, CMSG_COMRX);           /** �յ�һ���������ݲŷ���Ϣ **/
        }
        else
        {
            /** Do nothing **/
        }
      #endif
    }

    /** ��ɷ���USART1���� **/
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
			 * ����zero-crossʱ�� 
			 * 
			 * ʵ��ã��ⲿ�ж�1ms(Ҳ����8��ʱ��)֮��, ���źų������ź�����������. ��ʱ���źŵ�����(zero-cross)��1ms/2(Ҳ����4��ʱ��)��ʱ��
			 * 
			 * ����, ��ǰ���ⲿ�жϷ�����(8+4=)12�μ���֮��, ��Ϊzero-crossʱ��
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
