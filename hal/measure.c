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


#define WRAP_TIME_MASK 0xFFFF0000

static volatile uint64_t t_start;
static volatile uint64_t t_stop;
static volatile uint32_t wrap_cnt;
static volatile uint32_t wrap_time;

static volatile bool exec_started;


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
void exec_timer_start(void){/*{{{*/
    wrap_cnt = 0;
    t_fall = 0;
    t_rise = 0;
    exec_started = false;
    MAP_TimerIntEnable(TIMER2_BASE, TIMER_TIMA_EVENT);
    MAP_TimerIntEnable(TIMER2_BASE, TIMER_TIMB_TIMEOUT);
    MAP_TimerEnable(TIMER2_BASE, TIMER_BOTH);
}/*}}}*/

/**
 * Records time that execution started and stopped, triggered by rising/falling
 * edge on timer capture pin
 * Not atomic, since can be interrupted by wrap_isr
 */
void exec_timer_cap_isr(void){/*{{{*/
    uint32_t wrap_cnt_snap;
    uint32_t wrap_time_snap;
    uint32_t cap_time;

    TimerIntClear(TIMER2_BASE, TIMER_TIMA_EVENT);
    //Disable TimerB interrupt so snap and time read are atomic
    TimerIntDisable(TIMER2_BASE, TIMER_TIMB_TIMEOUT);{
        wrap_cnt_snap = wrap_cnt;
        wrap_time_snap = wrap_time;
        cap_time = MAP_TimerValueGet(TIMER0_BASE, TIMER_A);
    } TimerIntEnable(TIMER2_BASE, TIMER_TIMB_TIMEOUT);
    

    // If timer A value when wrap_cnt was incremented was equal to cap timer, then
    // wrap must have been triggered during or after cap timer
    if(wrap_cnt_time == cap_time){
        // If capture time was shortly before wraparound point, let's say halfway
        // through counter (0xFFFF/2), than wraparound probably happened after
        // and thus decrement to compensate
        if(cap_time > (0xFFFF >> 1)){
            --wrap_cnt;
        }  
    }


    if(!exec_started){
        t_start = wrap_cnt << 16 + cap_time;
        exec_started = true;
    }else{
        t_stop = wrap_cnt << 16 + cap_time;
        exec_time_elapsed = (wrap_cnt << 16 + MAP_TimerValueGet(TIMER0_BASE, TIMER_A)) - t_start;
        
        MAP_TimerDisable(TIMER2_BASE, TIMER_BOTH);
    }
}/*}}}*/

/**
 * Counts how many times capture timer wraps around
 * This ISR should be of higher priority than exec_timer_cap_isr 
 */
void exec_timer_wrap_isr(void){/*{{{*/
    TimerIntClear(TIMER2_BASE, TIMER_TIMB_TIMEOUT);
    
    ++wrap_cnt; 
    wrap_time = MAP_TimerValueGet(TIMER0_BASE, TIMER_A); 
}/*}}}*/


void pwr_sample_isr(void *args){
    uint32_t wrap_cnt_snap;
    uint32_t wrap_time_snap; 
    
    //Disable interrupts so we get both values together atomically
    //Really only need to disable exec_timer_cap and exec_timer_wrap, but we
    //kill all just to be sure. Plus cpsid/cpsie is faster than masking
    //individual interrupts
    IntMasterDisable();
    wrap_cnt_snap = wrap_cnt;
    wrap_cnt_snap = wrap_time;
    IntMasterEnable();

}

