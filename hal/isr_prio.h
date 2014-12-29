#ifndef _ISR_PRIO_H
#define _ISR_PRIO_H

// Macros for converting ISR priorities to format expected by ARM/*{{{*/
// Max supported priority 
#define ISR_MAX_PRIO_RAW 7 
// Pre-encoding value of FREERTOS_SYSCALL_PRIO
#define MAX_SYSCALL_PRIO_RAW (ISR_MAX_PRIO_RAW-2)

// Modifies priority into appropriate format for processor
// This macro inverts ARM priority values to be consistent w/ FreeRTOS (i.e
// higher number == higher priority)
// On ARM Cortex M4:
/* 3 bits, shift upwards by 5, lower number = higher priority (for Cortex M4) */

#define ISR_PRIO_ENC(x) ((ISR_MAX_PRIO_RAW-x) << 5)
/*}}}*/

/* Interrupt priorities */
// FreeRTOS priorities/*{{{*/
/* Interrupt nesting behaviour configuration. */
/* Be ENORMOUSLY careful if you want to modify these two values and make sure
 * you read http://www.freertos.org/a00110.html#kernel_priority first!
 */
#define FREERTOS_KERNEL_PRIO ISR_PRIO_ENC(0) /* Priority 7, or 0xE0 as only the top three bits are implemented.  This is the lowest priority. */

#define FREERTOS_SYSCALL_PRIO ISR_PRIO_ENC(MAX_SYSCALL_PRIO_RAW)  /* Priority 2, or 0xA0 as only the top three bits are implemented. */

/*}}}*/

#define ETH_ISR_PRIO        FREERTOS_KERNEL_PRIO
#define TIMER_CAP_ISR_PRIO  ISR_PRIO_ENC(MAX_SYSCALL_PRIO_RAW-1)
#define TIMER_WRAP_ISR_PRIO ISR_PRIO_ENC(MAX_SYSCALL_PRIO_RAW+1)
#define PWR_SAMPLE_ISR_PRIO ISR_PRIO_ENC(MAX_SYSCALL_PRIO_RAW-2)

#endif
