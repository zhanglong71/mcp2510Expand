
#include "stm32f10x_lib.h"
#include "string.h"
#include "../inc/CONST.h"
#include "../inc/ctype.h" 
#include "../inc/global.h" 
#include "../inc/debug.h"
#include "arch.h"		  
#include "func.h"		  
#include "register.h"		 
#include "main.h"	 
#include "X10Que.h"	

 
/*******************************************************************************
 * local function
 *******************************************************************************/
int func_idle(unsigned *data)
{
	//int i ;
    //static unsigned char iTmp;
	func_t func;
	//FLASH_Status fl_status = FLASH_COMPLETE;

    switch(*(msgType_t *)data)
	{
	case CMSG_TMR:
		#if	0
			if(iTmp++ & 0x01)
			{
       			GPIO_ResetBits(GPIOA, GPIO_Pin_0);
    		}
    		else
    		{
        		GPIO_SetBits(GPIOA, GPIO_Pin_0);
    		}
		#endif

		break;
		
	case CMSG_RGST:
		/** 
		 * Enter device register status
		 * for x10, the register message is ignored
		 *
		 * 对于x10电力载波, 不必发送自举命令, 忽略之
		 **/
		#if	0	
    		func.func = func_register;
    		stackpush(&g_fstack, &func);
    	
    		SetTimer_irq(&g_timer[0], TIMER_RGST);
		#endif
		break;
	case CMSG_INIT:
		
		//reversion_GPIOA6();	/** ?????????????????????????????????????????????? **/
		
		break;
	case CX10_CALIBR:		/** 进入zero-cross校准模式 **/
    	func.func = func_calibration;
    	stackpush(&g_fstack, &func);
    	
    	SetTimer_irq(&g_timer[0], TIMER_CALIB);
		
		break;
	default:
		
		break;
	}
    
    
    return  0;
}
/////////////////////////////////////////////////////

