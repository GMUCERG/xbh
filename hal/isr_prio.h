#ifndef _ISR_PRIO_H
#define _ISR_PRIO_H

/* Interrupt priorities */
/* 3 bits, shift upwards by 5, lower number = higher priority (for Cortex M4) */

// FreeRTOS priorities/*{{{*/
/* Interrupt nesting behaviour configuration. */
/* Be ENORMOUSLY careful if you want to modify these two values and make sure
 * you read http://www.freertos.org/a00110.html#kernel_priority first!
 */
#define FREERTOS_KERNEL_PRIO ( 7 << 5 ) /* Priority 7, or 0xE0 as only the top three bits are implemented.  This is the lowest priority. */

#define FREERTOS_SYSCALL_PRIO ( 2 << 5 )  /* Priority 2, or 0xA0 as only the top three bits are implemented. */
/*}}}*/

#define ETH_ISR_PRIO FREERTOS_KERNEL_PRIO
#define TIMER_CAP_ISR_PRIO (1 << 5)
#define TIMER_WRAP_ISR_PRIO (0 << 5)


#endif
