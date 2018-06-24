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
#include "inc/macro.h"
#include "inc/global.h"
#include "src/init.h"
#include "src/arch.h" 
#include "src/driver.h"
#include "src/bitmapOp.h"
#include "src/charQue.h"
#include "src/canTxQue.h"
#include "src/wdg.h"
#include "main.h"

#include "src/func.h"
/** global variable **/

fstack_t g_fstack;
msgq_t  g_msgq;
Timer_t g_timer[TIMER_NUM];

canFrame_queue_t g_canTxQue;
//charBuf_queue_t g_comTxQue;
charBuf_queue_t g_com1TxQue;
charBuf_queue_t g_com2TxQue;

//charBuf_queue_t g_wirelessTxQue;
charBuf_queue_t g_infraredTxQue;	/** 相对其它转接板的转发缓冲区，此转发缓冲区并没有大的作用 **/

int g_channel;

/** 可读写的flash空间的起始地址. 选在flash空间的最后一个page **/
//u32 g_flashPageAddr = (0x08000000 + (127 * CFLASH_PAGE_SIZE));
//unsigned char g_page[CFLASH_PAGE_SIZE];

const char g_magic[MAGIC_SIZE] = {0x48, 0x55, 0x41, 0x52}; 		/** 'H', 'U', 'A', 'R'**/

flashPage_t g_flash;
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

#if	0
	/** 如果有，就读取，否则取默认值 **/
	if(memcmp((char *)(MpageAddr(0) + MAGIC_SIZE_OFFSET), g_magic, MAGIC_SIZE) != 0)	//没有写入过数据
	{
		flashWrite((u32 *)g_flash.arrInt, 0);
		memcpy((g_flash.arrChar + MAGIC_SIZE_OFFSET), g_magic, CFLASH_PAGE_SIZE);		/** data valid! read it first **/
	}
#endif
    RCC_Configuration();      /* System Clocks Configuration */
    NVIC_Configuration();     /* NVIC Configuration */
    GPIO_Configuration();     /* GPIO ports pins Configuration */
    USART_Configuration();    /* Configure the USART1 */
     /** read channel NO. **/
    #if 1
    //g_channel = (GPIO_ReadInputData(GPIOC) >> 6) & 0x0f;
    g_channel = 0x04;
    #else
    if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_6)) g_channel |= 0x01; else g_channel &= ~0x01;
    if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7)) g_channel |= 0x02; else g_channel &= ~0x02;
    if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8)) g_channel |= 0x04; else g_channel &= ~0x04;
    if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_9)) g_channel |= 0x08; else g_channel &= ~0x08;
    #endif
    CAN_Configuration((g_channel << 18) | (FILTER_EID));      /* CAN transmit at 125Kb/s and receive by interrupt in normal mode*/
    TIM_Configuration();      /* Configure the TIM */
    EXTI_Configuration();			/** exti **/
    
    
	MINFRARED_IDLE();		/** infrared idle **/
	Mchannel(0xff);			/** not 0 ,1, 2, 3, 4, 5 **/
	
    bitmap_clean(&bmpRcvCan);

    CanTXqueueInit(&g_canTxQue);
    charQueueInit(&g_com1TxQue);
    charQueueInit(&g_com2TxQue);
    charQueueInit(&g_infraredTxQue);
    
    init_fstack(&g_fstack);
    init_queue(&g_msgq);
    
    func.func = func_idle;
    stackpush(&g_fstack, &func);

    for(i = 0; i < TIMER_NUM; i++)
    {
        ClrTimer(&g_timer[i]);
    }
    SetTimer(&g_timer[0], CTIMER_1SEC);

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
