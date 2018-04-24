/**
 * Hardware-specific startup code for XBH running on Connected Tiva-C
 * (C) 2014, John Pham, George Mason University <jpham4@gmu.edu>
 *
 * Based on startup_gcc.c from TI project examples from Tivaware
 */
//*****************************************************************************
//
// startup_gcc.c - Startup code for use with GNU tools.
//
// Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions
//   are met:
// 
//   Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the  
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// This is part of revision 2.1.0.12573 of the Tiva Firmware Development Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdalign.h>
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"

//*****************************************************************************
//
// Forward declaration of the default fault handlers.
//
//*****************************************************************************
void ResetISR(void);
static void NmiSR(void);
static void FaultISR(void);
static void IntDefaultHandler(void);

//*****************************************************************************
//
// The entry point for the application.
//
//*****************************************************************************
extern int main(void);

//*****************************************************************************
//
// Reserve space for the system stack.
//
//*****************************************************************************
static alignas(sizeof(uint64_t)) uint32_t pui32Stack[128];


/*
 * Interrupt handlers from FreeRTOS
 */
extern void xPortPendSVHandler(void);
extern void vPortSVCHandler(void);
extern void xPortSysTickHandler(void);

/*
 * Interrupt handlers from hal
 */
extern void lwIP_eth_isr(void);
extern void exec_timer_cap_isr(void);
extern void exec_timer_wrap_isr(void);
//extern void pwr_sample_timer_isr(void);
extern void watchdog_isr(void);


//*****************************************************************************
//
// The vector table.  Note that the proper constructs must be placed on this to
// ensure that it ends up at physical address 0x0000.0000.
// 
// Comments pulled from inc/hw_ints.h
//
//*****************************************************************************
__attribute__ ((section(".isr_vector"),used ))
void (* const g_pfnVectors[])(void) =
{
    (void (*)(void))((uint32_t)pui32Stack + sizeof(pui32Stack)), // The initial stack pointer // O
    ResetISR,                               // The reset handler        // 1
    NmiSR,                                  // The NMI handler          // 2
    FaultISR,                               // The hard fault handler   // 3
    IntDefaultHandler,                      // The MPU fault handler    // 4
    IntDefaultHandler,                      // The bus fault handler    // 5
    IntDefaultHandler,                      // The usage fault handler  // 6
    0,                                      // Reserved                 // 7
    0,                                      // Reserved                 // 8
    0,                                      // Reserved                 // 9
    0,                                      // Reserved                 // 10
    vPortSVCHandler,                        // SVCall handler           // 11  // Handles system calls
    IntDefaultHandler,                      // Debug monitor handler    // 12
    0,                                      // Reserved                 // 13
    xPortPendSVHandler,                     // The PendSV handler       // 14
    xPortSysTickHandler,                    // The SysTick handler      // 15
    IntDefaultHandler,                      // 16  // GPIO Port A
    IntDefaultHandler,                      // 17  // GPIO Port B
    IntDefaultHandler,                      // 18  // GPIO Port C
    IntDefaultHandler,                      // 19  // GPIO Port D
    IntDefaultHandler,                      // 20  // GPIO Port E
    IntDefaultHandler,                      // 21  // UART0
    IntDefaultHandler,                      // 22  // UART1
    IntDefaultHandler,                      // 23  // SSI0
    IntDefaultHandler,                      // 24  // I2C0
    IntDefaultHandler,                      // 25  // PWM Fault
    IntDefaultHandler,                      // 26  // PWM Generator 0
    IntDefaultHandler,                      // 27  // PWM Generator 1
    IntDefaultHandler,                      // 28  // PWM Generator 2
    IntDefaultHandler,                      // 29  // QEI0
    IntDefaultHandler,                      // 30  // ADC0 Sequence 0
    IntDefaultHandler,                      // 31  // ADC0 Sequence 1
    IntDefaultHandler,                      // 32  // ADC0 Sequence 2
    IntDefaultHandler,                      // 33  // ADC0 Sequence 3
    watchdog_isr,                           // 34  // Watchdog Timers 0 and 1
    exec_timer_cap_isr,                     // 35  // 16/32-Bit Timer 0A
    exec_timer_wrap_isr,                    // 36  // 16/32-Bit Timer 0B
//    pwr_sample_timer_isr,                   // 37  // 16/32-Bit Timer 1A
    IntDefaultHandler,                   // 37  // 16/32-Bit Timer 1A
    IntDefaultHandler,                      // 38  // 16/32-Bit Timer 1B
    IntDefaultHandler,                      // 39  // 16/32-Bit Timer 2A
    IntDefaultHandler,                      // 40  // 16/32-Bit Timer 2B
    IntDefaultHandler,                      // 41  // Analog Comparator 0
    IntDefaultHandler,                      // 42  // Analog Comparator 1
    IntDefaultHandler,                      // 43  // Analog Comparator 2
    IntDefaultHandler,                      // 44  // System Control
    IntDefaultHandler,                      // 45  // Flash Memory Control
    IntDefaultHandler,                      // 46  // GPIO Port F
    IntDefaultHandler,                      // 47  // GPIO Port G
    IntDefaultHandler,                      // 48  // GPIO Port H
    IntDefaultHandler,                      // 49  // UART2
    IntDefaultHandler,                      // 50  // SSI1
    IntDefaultHandler,                      // 51  // 16/32-Bit Timer 3A
    IntDefaultHandler,                      // 52  // 16/32-Bit Timer 3B
    IntDefaultHandler,                      // 53  // I2C1
    IntDefaultHandler,                      // 54  // CAN 0
    IntDefaultHandler,                      // 55  // CAN1
    lwIP_eth_isr,                           // 56  // Ethernet MAC
    IntDefaultHandler,                      // 57  // HIB
    IntDefaultHandler,                      // 58  // USB MAC
    IntDefaultHandler,                      // 59  // PWM Generator 3
    IntDefaultHandler,                      // 60  // uDMA 0 Software
    IntDefaultHandler,                      // 61  // uDMA 0 Error
    IntDefaultHandler,                      // 62  // ADC1 Sequence 0
    IntDefaultHandler,                      // 63  // ADC1 Sequence 1
    IntDefaultHandler,                      // 64  // ADC1 Sequence 2
    IntDefaultHandler,                      // 65  // ADC1 Sequence 3
    IntDefaultHandler,                      // 66  // EPI 0
    IntDefaultHandler,                      // 67  // GPIO Port J
    IntDefaultHandler,                      // 68  // GPIO Port K
    IntDefaultHandler,                      // 69  // GPIO Port L
    IntDefaultHandler,                      // 70  // SSI 2
    IntDefaultHandler,                      // 71  // SSI 3
    IntDefaultHandler,                      // 72  // UART 3
    IntDefaultHandler,                      // 73  // UART 4
    IntDefaultHandler,                      // 74  // UART 5
    IntDefaultHandler,                      // 75  // UART 6
    IntDefaultHandler,                      // 76  // UART 7
    IntDefaultHandler,                      // 77  // I2C 2
    IntDefaultHandler,                      // 78  // I2C 3
    IntDefaultHandler,                      // 79  // Timer 4A
    IntDefaultHandler,                      // 80  // Timer 4B
    IntDefaultHandler,                      // 81  // Timer 5A
    IntDefaultHandler,                      // 82  // Timer 5B
    IntDefaultHandler,                      // 83  // Floating-Point Exception (imprecise)
    0,                                      // 84  // Reserved
    IntDefaultHandler,                      // 85  // Reserved
    IntDefaultHandler,                      // 86  // I2C 4
    IntDefaultHandler,                      // 87  // I2C 5
    IntDefaultHandler,                      // 88  // GPIO Port M
    IntDefaultHandler,                      // 89  // GPIO Port N
    0,                                      // 90  // Reserved
    IntDefaultHandler,                      // 91  // Tamper
    IntDefaultHandler,                      // 92  // GPIO Port P (Summary or P0)
    IntDefaultHandler,                      // 93  // GPIO Port P1
    IntDefaultHandler,                      // 94  // GPIO Port P2
    IntDefaultHandler,                      // 95  // GPIO Port P3
    IntDefaultHandler,                      // 96  // GPIO Port P4
    IntDefaultHandler,                      // 97  // GPIO Port P5
    IntDefaultHandler,                      // 98  // GPIO Port P6
    IntDefaultHandler,                      // 99  // GPIO Port P7
    IntDefaultHandler,                      // 100 // GPIO Port Q (Summary or Q0)
    IntDefaultHandler,                      // 101 // GPIO Port Q1
    IntDefaultHandler,                      // 102 // GPIO Port Q2
    IntDefaultHandler,                      // 103 // GPIO Port Q3
    IntDefaultHandler,                      // 104 // GPIO Port Q4
    IntDefaultHandler,                      // 105 // GPIO Port Q5
    IntDefaultHandler,                      // 106 // GPIO Port Q6
    IntDefaultHandler,                      // 107 // GPIO Port Q7
    IntDefaultHandler,                      // 108 // GPIO Port R
    IntDefaultHandler,                      // 109 // GPIO Port S
    IntDefaultHandler,                      // 110 // SHA/MD5
    IntDefaultHandler,                      // 111 // AES
    IntDefaultHandler,                      // 112 // DES
    IntDefaultHandler,                      // 113 // LCD
    IntDefaultHandler,                      // 114 // 16/32-Bit Timer 6A
    IntDefaultHandler,                      // 115 // 16/32-Bit Timer 6B
    IntDefaultHandler,                      // 116 // 16/32-Bit Timer 7A
    IntDefaultHandler,                      // 117 // 16/32-Bit Timer 7B
    IntDefaultHandler,                      // 118 // I2C 6
    IntDefaultHandler,                      // 119 // I2C 7
    0,                                      // 120 // Reserved
    IntDefaultHandler,                      // 121 // 1-Wire
    0,                                      // 122 // Reserved
    0,                                      // 123 // Reserved
    0,                                      // 124 // Reserved
    IntDefaultHandler,                      // 125 // I2C 8
    IntDefaultHandler,                      // 126 // I2C 9
    IntDefaultHandler                       // 127 // GPIO T
};

//*****************************************************************************
//
// The following are constructs created by the linker, indicating where the
// the "data" and "bss" segments reside in memory.  The initializers for the
// for the "data" segment resides immediately following the "text" segment.
//
//*****************************************************************************
extern uint32_t _etext;
extern uint32_t _ldata;
extern uint32_t _data;
extern uint32_t _edata;
extern uint32_t _bss;
extern uint32_t _ebss;

//*****************************************************************************
//
// This is the code that gets called when the processor first starts execution
// following a reset event.  Only the absolutely necessary set is performed,
// after which the application supplied entry() routine is called.  Any fancy
// actions (such as making decisions based on the reset cause register, and
// resetting the bits in that register) are left solely in the hands of the
// application.
//
//*****************************************************************************
void
ResetISR(void)
{
    uint32_t *pui32Src, *pui32Dest;

    //
    // Copy the data segment initializers from flash to SRAM.
    //
    pui32Src = &_ldata;
    for(pui32Dest = &_data; pui32Dest < &_edata; )
    {
        *pui32Dest++ = *pui32Src++;
    }

    //
    // Zero fill the bss segment.
    //
    __asm("    ldr     r0, =_bss\n"
          "    ldr     r1, =_ebss\n"
          "    mov     r2, #0\n"
          "    .thumb_func\n"
          "zero_loop:\n"
          "        cmp     r0, r1\n"
          "        it      lt\n"
          "        strlt   r2, [r0], #4\n"
          "        blt     zero_loop");

    //
    // Enable the floating-point unit.  This must be done here to handle the
    // case where main() uses floating-point and the function prologue saves
    // floating-point registers (which will fault if floating-point is not
    // enabled).  Any configuration of the floating-point unit using DriverLib
    // APIs must be done here prior to the floating-point unit being enabled.
    //
    // Note that this does not use DriverLib since it might not be included in
    // this project.
    //
    HWREG(NVIC_CPAC) = ((HWREG(NVIC_CPAC) &
                         ~(NVIC_CPAC_CP10_M | NVIC_CPAC_CP11_M)) |
                        NVIC_CPAC_CP10_FULL | NVIC_CPAC_CP11_FULL);

    //
    // Call the application's entry point.
    //
    main();
}

//*****************************************************************************
//
// This is the code that gets called when the processor receives a NMI.  This
// simply enters an infinite loop, preserving the system state for examination
// by a debugger.
//
//*****************************************************************************
static void
NmiSR(void)
{
    //
    // Enter an infinite loop.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
// This is the code that gets called when the processor receives a fault
// interrupt.  This simply enters an infinite loop, preserving the system state
// for examination by a debugger.
//
//*****************************************************************************
static void
FaultISR(void)
{
    //
    // Enter an infinite loop.
    //
    while(1)
    {
    }
}

//*****************************************************************************
//
// This is the code that gets called when the processor receives an unexpected
// interrupt.  This simply enters an infinite loop, preserving the system state
// for examination by a debugger.
//
//*****************************************************************************
static void
IntDefaultHandler(void)
{
    //
    // Go into an infinite loop.
    //
    while(1)
    {
    }
}
