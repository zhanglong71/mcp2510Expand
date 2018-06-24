/*******************************************************************************
* File Name          : main.c
* Author             : HuNan HuaRain Science & Technology Co.Ltd, zhanglong
* Date First Issued  : 2012-7-25
* Description        : Adapter
********************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_lib.h"														
#include "string.h"
#include "inc/CONST.h"
#include "inc/ctype.h"
#include "inc/global.h"
#include "src/init.h"
#include "src/arch.h"
#include "src/bitmapOp.h"
#include "src/charQue.h"
#include "src/canTxQue.h"
#include "src/wdg.h"
#include "main.h"
/* Local includes ------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

//RetStatus CAN_loop(void);

#include "src/func.h"
/** global variable **/
//unsigned char Can2ComBuf[CSEND_BUF_NUM][CSEND_BUF_SIZE];		//暂存来自canbus转发给com串口的数据(CANbus ==> com)
//unsigned char Com2CanBuf[CSEND_BUF_NUM][CSEND_BUF_SIZE];		//暂存来自com串口转发给CAN的数据(com ==> CANbus)

fstack_t g_fstack;
msgq_t  g_msgq;
Timer_t g_timer[TIMER_NUM];

canFrame_queue_t g_canTxQue;
//charBuf_queue_t g_comTxQue;
charBuf_queue_t g_com1TxQue;
charBuf_queue_t g_com2TxQue;

X10Buf_queue_t g_x10TxQue;

int g_channel;

unsigned char g_X10addrCode;
unsigned char g_X10funcCode;

const char g_magic[MAGIC_SIZE] = {0x12, 0x34, 0x56, 0x78};
/*******************************************************************************
* Function Name  : main
* Description    : Main program
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
int main(void)
{
    #ifdef DEBUG
    debug();
    #endif

    msgType_t msg;
    func_t func;
    s32 i = 0;

	/** 如果有，就读取zero-cross校正值，否则取默认值 **/
	if(memcmp((char *)g_flashPageAddr, g_magic, MAGIC_SIZE) == 0)
	{
		g_x10_sync_offset = ((unsigned char *)g_flashPageAddr)[MAGIC_SIZE + 0];
	}
	else
	{
		g_x10_sync_offset = CX10_SYNC_OFFSET;
	}
    RCC_Configuration();      /* System Clocks Configuration */
    NVIC_Configuration();     /* NVIC Configuration */
    GPIO_Configuration();     /* GPIO ports pins Configuration */
    USART_Configuration();    /* Configure the USART1 */
     /** read channel NO. **/
    #if 1
    g_channel = (GPIO_ReadInputData(GPIOC) >> 6) & 0x0f;
    #else
    if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_6)) g_channel |= 0x01; else g_channel &= ~0x01;
    if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7)) g_channel |= 0x02; else g_channel &= ~0x02;
    if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8)) g_channel |= 0x04; else g_channel &= ~0x04;
    if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_9)) g_channel |= 0x08; else g_channel &= ~0x08;
    #endif
    CAN_Configuration((g_channel << 18) | (FILTER_EID));      /* CAN transmit at 125Kb/s and receive by interrupt in normal mode*/
    TIM_Configuration();      /* Configure the TIM */
    EXTI_Configuration();			/** exti **/

    bitmap_clean(&bmpRcvCan);

    CanTXqueueInit(&g_canTxQue);
    charQueueInit(&g_com1TxQue);
    charQueueInit(&g_com2TxQue);
    
	x10module_init();
    init_fstack(&g_fstack);
    init_queue(&g_msgq);
    
    func.func = func_idle;
    stackpush(&g_fstack, &func);

    for(i = 0; i < TIMER_NUM; i++)
    {
        ClrTimer(&g_timer[i]);
    }
    SetTimer(&g_timer[0], TIMER_1SEC);

    /** WDG **/
    wdg_init();
    /**
    * Now all initial ok. then enable interrupt
    **/
	
    inq(&g_msgq, CMSG_INIT);

    IRQ_enable();


    while(1)
    {
        wdg_feed();
        
        DAEMON_USARTx_Send(USART1, &g_com1TxQue);   /** output for debug **/
        //DAEMON_USARTx_Send(USART2, &g_com2TxQue);
        DAEMON_CAN_Send();
        
        if(outq_irq(&g_msgq, &msg) == FALSE)    /** 有消息吗? **/
        {
            continue;
        }
        if(sysProcess(&msg) == TRUE)    /** 是系统消息吗? **/
        {
            continue;
        }
        if(stacktop(&g_fstack, &func) == FALSE)     /** 当前处于工作状态吗? **/
        {
            continue;
        }

        func.func((unsigned *)&msg);
    }
}

/*******************************************************************************
 * IRQ_disable()/IRQ_enable()
 *******************************************************************************/
void IRQ_disable(void)
{
    #if 0
        //NVIC_SETPRIMASK();
        __disable_irq();
    #else
        CAN_ITConfig(CAN_IT_FMP0, DISABLE);
        //CAN_ITConfig(CAN_IT_TME, DISABLE);
        TIM_ITConfig(TIM2, TIM_IT_CC1/** | TIM_IT_CC4 **/, DISABLE);
        //USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
        USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
    #endif
}

void IRQ_enable(void)
{
    #if 0
        //NVIC_RESETPRIMASK();
        __enable_irq();
    #else
        CAN_ITConfig(CAN_IT_FMP0, ENABLE);
        //CAN_ITConfig(CAN_IT_TME, ENABLE);
        TIM_ITConfig(TIM2, TIM_IT_CC1/** | TIM_IT_CC4 **/, ENABLE);
        //USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
        USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    #endif
}


#ifdef  DEBUG
/*******************************************************************************
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void assert_failed(u8* file, u32 line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
/******************* (C) COPYRIGHT 2007 STMicroelectronics *****END OF FILE****/
