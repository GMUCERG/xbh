#include <inttypes.h>
#include <stdbool.h>

#include <driverlib/rom.h>

#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <driverlib/gpio.h>
#include <driverlib/interrupt.h>
#include <driverlib/pin_map.h>
#include <driverlib/rom_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/timer.h>

#include "hal/timer.h"
#include "isr_prio.h"

static uint32_t wrap_cnt;
static uint64_t t_start;
static bool exec_started;

uint64_t exec_time_elapsed;

/**
 * Sets up execution timer
 */
void exec_timer_setup(void){/*{{{*/
    MAP_IntPrioritySet(INT_TIMER2A, TIMER_CAP_ISR_PRIO);
    MAP_IntPrioritySet(INT_TIMER2B, TIMER_WRAP_ISR_PRIO);

    // PM0 and PM1 are attached to timer2
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
    MAP_SysCtlPeripheralReset(SYSCTL_PERIPH_TIMER2);
    MAP_GPIOPinConfigure(GPIO_PM0_T2CCP0);
    
    // Split needed for timer capture
    // We use timer A for capture and timer B to count wraparounds of timer A
    MAP_TimerConfigure(TIMER2_BASE, TIMER_CFG_SPLIT_PAIR|TIMER_CFG_A_CAP_TIME_UP|TIMER_CFG_B_PERIODIC_UP);

    // Synchronize both counters
    MAP_TimerSynchronize(TIMER0_BASE, TIMER_2A_SYNC|TIMER_2B_SYNC);

    // Detect both rising and falling edges
    MAP_TimerControlEvent(TIMER2_BASE, TIMER_A, TIMER_EVENT_BOTH_EDGES);
}/*}}}*/

/**
 * Starts execution timer, enabling interrupts
 */
void exec_timer_startcap(void){/*{{{*/
    wrap_cnt = 0;
    t_fall = 0;
    t_rise = 0;
    exec_started = false;
    MAP_TimerIntEnable(TIMER2_BASE, TIMER_TIMA_EVENT);
    MAP_TimerIntEnable(TIMER2_BASE, TIMER_TIMB_MATCH);
    MAP_TimerEnable(TIMER2_BASE, TIMER_BOTH);
}/*}}}*/

/**
 * Records time that execution started and stopped, triggered by rising/falling
 * edge on timer capture pin
 */
void exec_timer_cap_isr(void){/*{{{*/
    if(!exec_started){
        t_start = wrap_cnt << 16 + MAP_TimerValueGet(TIMER0_BASE, TIMER_A);
        exec_started = true;
    }else{
        exec_time_elapsed = (wrap_cnt << 16 + MAP_TimerValueGet(TIMER0_BASE, TIMER_A)) - t_start;
        MAP_TimerDisable(TIMER2_BASE, TIMER_BOTH);
        MAP_TimerIntDisable(TIMER2_BASE, TIMER_TIMA_EVENT);
        MAP_TimerIntDisable(TIMER2_BASE, TIMER_TIMB_MATCH);
    }
}/*}}}*/

/**
 * Counts how many times capture timer wraps around
 */
void exec_timer_wrap_isr(void){/*{{{*/
    wrap_cnt++;
}/*}}}*/

