/**
 * Hardware-specific code for XBH running on Connected Tiva-C
 * (C) 2014, John Pham, George Mason University <jpham4@gmu.edu>
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>

#include <inc/hw_memmap.h>
#include <driverlib/debug.h>
#include <driverlib/gpio.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/emac.h>
#include "driverlib/uart.h"


#include "FreeRTOSConfig.h"
#include "util.h"
#include "lwip_eth.h"



/**
 * Clock rate in HZ
 */
uint32_t g_syshz;




/**
 * Hardware setup
 * Initializes pins, clocks, etc
 */
void HAL_setup(void){/*{{{*/

    //Configure clock to run at 120MHz
    //configCPU_CLOCK_HZ = 120MHz
    //Needs to be set for FreeRTOS
    g_syshz =  MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480),
                                             configCPU_CLOCK_HZ);

    MAP_SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);

    //Enable all GPIOs
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
    init_ethernet();

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //Configure UART
    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    MAP_UARTConfigSetExpClk(UART0_BASE, g_syshz, 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

}/*}}}*/


// Serial output code
/*{{{*/

void uart_write_str(char *str) {
	while (*str)
	{
		MAP_UARTCharPut(UART0_BASE, *str++);
	}
}

void uart_write_char(char c){
    MAP_UARTCharPut(UART0_BASE, c);
}

/*}}}*/

// Timer capture code

void start_capture(void){
#define TIMER_PIN GPIO_PM0_T2CCP0
#define TIMER_BASE TIMER2_BASE
    MAP_IntPrioritySet(INT_TIMER2A, TIMER_CAP_ISR_PRIO);
    MAP_IntPrioritySet(INT_TIMER2B, TIMER_WRAP_ISR_PRIO);

// PM0 and PM1 are attached to timer2
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
    MAP_SysCtlPeripheralReset(SYSCTL_PERIPH_TIMER2);
    MAP_GPIOPinConfigure(TIMER_PIN);
    

    // Split needed for timer capture
    // We use timer A for capture and timer B to count wraparounds of timer A
    TimerConfigure(TIMER_BASE, TIMER_CFG_SPLIT_PAIR|TIMER_CFG_A_CAP_TIME_UP|TIMER_CFG_B_PERIODIC_UP);

    // Synchronize both counters
    TimerSynchronize(TIMER0_BASE, TIMER2A_SYNC, TIMER2B_SYNC);

    // Keep counting even in debug
    TimerControlStall(TIMER_BASE, TIMER_BOTH, false);

    // Detect both rising and falling edges
    TimerControlEvent(TIMER_BASE, TIMER_A, TIMER_EVENT_BOTH_EDGES);


        


