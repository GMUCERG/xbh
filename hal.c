/**
 * Hardware-specific code for XBH running on Connected Tiva-C
 * (C) 2014, John Pham, George Mason University <jpham4@gmu.edu>
 */

#include <inttypes.h>
#include <stdbool.h>

#include <inc/hw_memmap.h>
#include <driverlib/debug.h>
#include <driverlib/gpio.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>



#include "FreeRTOSConfig.h"
/**
 * Clock rate in HZ
 */
uint32_t g_syshz;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

/**
 * Hardware setup
 * Initializes pins, clocks, etc
 */
void HAL_setup(void){

    //Configure clock to run at 120MHz
    //configCPU_CLOCK_HZ = 120MHz
    //Needs to be set for FreeRTOS
    g_syshz =  MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480),
                                             configCPU_CLOCK_HZ);

    MAP_SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);

    // Set Ethernet LED pinouts
    // See Page 269 of spmu298a.pdf
    MAP_GPIOPinConfigure(GPIO_PF0_EN0LED0);
    MAP_GPIOPinConfigure(GPIO_PF4_EN0LED1);
    GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4);
    //Using builtin PHY, so don't need to call GPIOPinTypeEthernetMII()
}
