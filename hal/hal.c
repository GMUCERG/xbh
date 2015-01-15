/**
 * Hardware-specific code for XBH running on Connected Tiva-C
 * (C) 2014, John Pham, George Mason University <jpham4@gmu.edu>
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>

#include <driverlib/rom.h>

#include <inc/hw_memmap.h>
#include <inc/hw_ints.h>
#include <driverlib/gpio.h>
#include <driverlib/rom_map.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include "driverlib/uart.h"


#include "FreeRTOSConfig.h"
#include "util.h"
#include "hal/lwip_eth.h"
#include "hal/i2c.h"
#include "hal/measure.h"



#define RESET_PIN 0x10

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
                                             120000000L);

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
//    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //Configure reset pin for XBD
    MAP_GPIODirModeSet(GPIO_PORTP_BASE, RESET_PIN, GPIO_DIR_MODE_OUT);

    //Configure UART
    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    MAP_UARTConfigSetExpClk(UART0_BASE, g_syshz, 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));


    //Configure measurement stuff
    measure_setup();

    //Configure xbd i2c comm
    i2c_comm_setup();
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

/**
 * Sets reset pin to given value
 * @param value true to bring reset high, false for low
 */
void xbd_reset(bool value){
    if(value){
        MAP_GPIOPinWrite(GPIO_PORTP_BASE, RESET_PIN, RESET_PIN);
    }else{
        MAP_GPIOPinWrite(GPIO_PORTP_BASE, RESET_PIN, 0);
    }

}
