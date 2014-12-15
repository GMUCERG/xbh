/**
 * Hardware-specific code for XBH running on Connected Tiva-C
 * (C) 2014, John Pham, George Mason University <jpham4@gmu.edu>
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

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


// 
// Debug output code
// Should output stuff over serial 
// TODO Actually implement
//
///*{{{*/
#ifdef DEBUG

void usart_write_str(char *str) {
	while (*str)
	{
		MAP_UARTCharPut(UART0_BASE, *str++);
	}
}

//from usart.c
#if 1
static void uart_vwriteP(const char *Buffer, va_list *list) {/*{{{*/
	int format_flag;
	char str_buffer[10];
	char str_null_buffer[10];
	char move = 0;
	int tmp = 0;
	char by;
	char *ptr;
    uint8_t base;
    bool is_signed;
    uint8_t field_sz;
		
	//Ausgabe der Zeichen
	for(;;) {
		by = *(Buffer++);
		if(by==0) break; // end of format string
            
		if (by == '%') {
            bool zeropad = false;
			by = *(Buffer++);
            if(by == '0'){
                by = *(Buffer++);
                zeropad = true;
            }

			if (isdigit((uint8_t)by)>0){
				str_null_buffer[0] = by;
				str_null_buffer[1] = '\0';
				move = atoi(str_null_buffer);
				by = *(Buffer++);
			}

			switch (by) {
                case 'h':
                    by = *(Buffer++);
                    if(by == 'h') {
                        field_sz = 1;
                        by = *(Buffer++);
                    }else{
                        field_sz = 2;
                    }
                    break;
                case 'l':
                    by = *(Buffer++);
                    if(by == 'l') {
                        field_sz = 8;
                        by = *(Buffer++);
                    }else{
                        field_sz = 4;
                    }
                    break;
                case 'z':
                    by = *(Buffer++);
                    field_sz = 4;
                    break;
                default:
                    field_sz = 4;
            }

			switch (by) {
				case 's':
					ptr = va_arg(*list,char *);
					while(*ptr) { MAP_UARTCharPut(UART0_BASE, *ptr++); }
					break;
				case 'c':
					//Int to char
					format_flag = va_arg(*list,int);
					MAP_UARTCharPut(UART0_BASE, format_flag++);
					break;
				case 'b':
                    base = 2;
                    is_signed = false;
					goto ConversionLoop;
				case 'i':
				case 'd':
                    is_signed = true;
                    base = 10;
                    goto ConversionLoop;
				case 'u':
                    is_signed = false;
                    base = 10;
					goto ConversionLoop;
				case 'o':
                    is_signed = false;
                    base = 8;
					goto ConversionLoop;
				case 'x':
				case 'p':
                    is_signed = false;
                    base = 16;
					//****************************
                ConversionLoop:
					//****************************
                    switch(field_sz){
                        case 1:
                        case 2:
                        case 4:
                        default:
                            if(is_signed){
                                ltoa((int32_t)va_arg(*list,int), str_buffer, base);
                            }else{
                                ltoa((uint32_t)va_arg(*list,unsigned int), str_buffer, base);
                            }
                            break;
                        case 8:
                            if(is_signed){
                                ltoa((int64_t)va_arg(*list,long long), str_buffer, base);
                            }else{
                                ltoa((uint64_t)va_arg(*list,unsigned long long), str_buffer, base);
                            }
                            break;
                    }


					int b=0;
					while (str_buffer[b++] != 0){};
					b--;
					if (b<move)
					{
						move -=b;
						for (tmp = 0;tmp<move;tmp++)
						{
                            if(zeropad){
                                str_null_buffer[tmp] = '0';
                            }else{
                                str_null_buffer[tmp] = ' ';
                            }
						}
						//tmp ++;
						str_null_buffer[tmp] = '\0';
						strcat(str_null_buffer,str_buffer);
						strcpy(str_buffer,str_null_buffer);
					}
					usart_write_str (str_buffer);
					move =0;
					break;
			}
			
		} else if (by == '\n'){
			MAP_UARTCharPut( UART0_BASE, '\r' );	
			MAP_UARTCharPut( UART0_BASE, by );	
        }else {
			MAP_UARTCharPut( UART0_BASE, by );	
		}
	}
}/*}}}*/
#else
static void uart_vwriteP(const char *Buffer, va_list *list) {/*{{{*/
	int format_flag;
	char str_buffer[10];
	char str_null_buffer[10];
	char move = 0;
	int tmp = 0;
	char by;
	char *ptr;
    uint8_t base;
		
	//Ausgabe der Zeichen
	for(;;) {
		by = *(Buffer++);
		if(by==0) break; // end of format string
            
		if (by == '%') {
			by = *(Buffer++);
			if (isdigit((uint8_t)by)>0){
				str_null_buffer[0] = by;
				str_null_buffer[1] = '\0';
				move = atoi(str_null_buffer);
				by = *(Buffer++);
			}


			switch (by) {
				case 's':
					ptr = va_arg(*list,char *);
					while(*ptr) { MAP_UARTCharPut(UART0_BASE, *ptr++); }
					break;
				case 'c':
					//Int to char
					format_flag = va_arg(*list,int);
					MAP_UARTCharPut(UART0_BASE, format_flag++);
					break;
				case 'b':
                    base = 2;
					goto ConversionLoop;
				case 'i':
				case 'd':
                    base = 10;
                    goto ConversionLoop;
				case 'u':
                    base = 10;
					goto ConversionLoop;
				case 'o':
                    base = 8;
					goto ConversionLoop;
				case 'x':
				case 'p':
                    base = 16;
					//****************************
                ConversionLoop:
					//****************************

                    ltoa(va_arg(*list,int), str_buffer, base);

					int b=0;
					while (str_buffer[b++] != 0){};
					b--;
					if (b<move)
					{
						move -=b;
						for (tmp = 0;tmp<move;tmp++)
						{
							str_null_buffer[tmp] = '0';
						}
						//tmp ++;
						str_null_buffer[tmp] = '\0';
						strcat(str_null_buffer,str_buffer);
						strcpy(str_buffer,str_null_buffer);
					}
					usart_write_str (str_buffer);
					move =0;
					break;
			}
			
		} else if (by == '\n'){
			MAP_UARTCharPut( UART0_BASE, '\r' );	
			MAP_UARTCharPut( UART0_BASE, by );	
        }else {
			MAP_UARTCharPut( UART0_BASE, by );	
		}
	}
}/*}}}*/
#endif

void uart_writeP (const char *buffer,...) {
	va_list ap;
	va_start (ap, buffer);	
	uart_vwriteP(buffer, &ap);
	va_end(ap);
}
void __error__(char * filename, uint32_t line) { 
    uart_writeP("ERROR:%s:%d\n", filename, line);
}
#pragma GCC diagnostic push  // require GCC 4.6
#pragma GCC diagnostic ignored "-Wcast-qual"
void assert_called( const char * const filename,uint32_t line ) { __error__((char*)filename,line); }
#pragma GCC diagnostic pop
#endif
/*}}}*/
