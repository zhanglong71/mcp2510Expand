
#include "stm32f10x_lib.h"
#include "../inc/CONST.h"
#include "../inc/ctype.h"
#include "../inc/global.h"
#include "wdg.h"

void wdg_init(void)
{

    /* IWDG timeout equal to 350ms (the timeout may varies due to LSI frequency
    dispersion) -------------------------------------------------------------*/
    /* Enable write access to IWDG_PR and IWDG_RLR registers */
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    /* IWDG counter clock: 32KHz(LSI) / 32 = 1KHz */
    IWDG_SetPrescaler(IWDG_Prescaler_32);

    /* Set counter reload value to 349 */
    IWDG_SetReload(349);
    //IWDG_SetReload(1999);

    /* Reload IWDG counter */
    IWDG_ReloadCounter();

    /* Enable IWDG (the LSI oscillator will be enabled by hardware) */
    IWDG_Enable();
}

void wdg_feed(void)
{
    /* Reload IWDG counter */
    IWDG_ReloadCounter();
}

