
#include "stm32f10x_lib.h" 
#include "string.h"
#include "../inc/CONST.h"
#include "../inc/ctype.h" 
#include "../inc/global.h" 
#include "../inc/debug.h"
#include "func.h"		 
#include "main.h"	 
#include "src/arch.h"
#include "X10Que.h"	

/** 可读写的flash空间的起始地址. 选在flash空间的最后一个page **/
u32 g_flashPageAddr = (0x08000000 + (127 * CFLASH_PAGE_SIZE));
unsigned char g_page[CFLASH_PAGE_SIZE];
/*******************************************************************************
 * register function
 *******************************************************************************/
int func_register(unsigned *data)
{
    static unsigned int iCount = 0x0f01; 
    //func_t func;
    
    switch(*(msgType_t *)data)
	{
	case CMSG_TMR: 
		dev_oneRegister(&g_canTxQue, iCount);

		if(iCount++ >= 0x0f03)
		{
			iCount = 0x0f01;
			#if	1
				stackpop(&g_fstack);
			#else
    			init_fstack(&g_fstack);
				func.func = func_idle;
    			stackpush(&g_fstack, &func);
			#endif

			SetTimer_irq(&g_timer[0], TIMER_1SEC);
		}
		
		break;
		
	case CMSG_INIT:
		break;
	default:
		
		break;
	}
    
    
    return  0;
}
/*******************************************************************************
 * calibration zero-cross offset
 *******************************************************************************/
int func_calibration(unsigned *data)
{
	int i;
    switch(*(msgType_t *)data)
	{
	case CMSG_TMR: 	 /** time out **/
		#if	1
			stackpop(&g_fstack);
		#else
    		init_fstack(&g_fstack);
			func.func = func_idle;
    		stackpush(&g_fstack, &func);
		#endif

		SetTimer_irq(&g_timer[0], TIMER_1SEC);
		
		break;
	
	case CMSG_SAVE:
		/* Clear All pending flags */
    	FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    	/** FLASH解锁 **/
		FLASH_Unlock();

		/** buckup the old data **/
		memcpy(g_page, (char *)g_flashPageAddr, CFLASH_PAGE_SIZE);	/** old data **/
		memcpy(g_page, g_magic, MAGIC_SIZE);						/** magic data **/
		//g_page[MAGIC_SIZE + 0] += 7;
		g_page[MAGIC_SIZE + 0] = g_x10_zcross_calibration;
		USART_SendData(USART1, g_page[0]);
	
		/** Erases the Flash Page **/ 
		FLASH_ErasePage(g_flashPageAddr);

		for(i = 0; i < (CFLASH_PAGE_SIZE / 4); i++)
		{
			FLASH_ProgramWord(g_flashPageAddr, ((u32 *)g_page)[i]);
		}
    	/** FLASH加锁 **/
		FLASH_Lock();
		
		break;
	case CMSG_INIT:
		
		break;
	default:
		
		break;
	}
    
    
    return  0;
}
/////////////////////////////////////////////////////

