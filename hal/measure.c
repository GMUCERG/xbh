#include <inttypes.h>
#include <stdbool.h>

#include <driverlib/rom.h>
#include <driverlib/rom_map.h>

#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <driverlib/gpio.h>
#include <driverlib/interrupt.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/timer.h>

#include "hal/hal.h"
#include "hal/measure.h"
#include "hal/isr_prio.h"

#include "FreeRTOSConfig.h"
#include <FreeRTOS.h>
#include <task.h>

#define SAMPLE_PERIOD_US 500



/** Sampling task handle */
TaskHandle_t pwr_sample_task_handle;

// Timer capture variables/*{{{*/
static volatile uint32_t wrap_cnt;
static volatile uint32_t wrap_cap_cnt;
static volatile uint64_t t_start;
static volatile uint64_t t_stop;
static volatile uint32_t cap_cnt;
/*}}}*/



// Execution timer stuff/*{{{*/

/**
 * Sets up execution timer
 */
void exec_timer_setup(void){/*{{{*/
    MAP_IntPrioritySet(INT_TIMER0A, TIMER_CAP_ISR_PRIO);
    MAP_IntPrioritySet(INT_TIMER0B, TIMER_WRAP_ISR_PRIO);

    // PA0 and PA1 are attached to timer0
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    MAP_SysCtlPeripheralReset(SYSCTL_PERIPH_TIMER0);
    MAP_GPIOPinConfigure(GPIO_PA0_T0CCP0);
    
    // Split needed for timer capture
    // We use timer A for capture and timer B to count wraparounds of timer A
    MAP_TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR|TIMER_CFG_A_CAP_TIME_UP|TIMER_CFG_B_PERIODIC_UP);

    // Synchronize both counters
    MAP_TimerSynchronize(TIMER0_BASE, TIMER_0A_SYNC|TIMER_0B_SYNC);

    // Detect both rising and falling edges
    MAP_TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_BOTH_EDGES);

}/*}}}*/

/**
 * Starts execution timer, enabling interrupts
 */
void exec_timer_start(void){/*{{{*/
    wrap_cnt = 0;
    t_start= 0;
    t_stop= 0;
    cap_cnt = 0;
    MAP_TimerIntEnable(TIMER0_BASE, TIMER_CAPA_EVENT);
    MAP_TimerIntEnable(TIMER0_BASE, TIMER_TIMB_TIMEOUT);
    MAP_TimerEnable(TIMER0_BASE, TIMER_BOTH);
}/*}}}*/

/**
 * Records time that execution started and stopped, triggered by rising/falling
 * edge on timer capture pin
 * Not atomic, since can be interrupted by wrap_isr
 */
void exec_timer_cap_isr(void){/*{{{*/
    uint32_t wrap_cnt_snap;
    uint32_t wrap_cap_cnt_snap;
    uint32_t cap_time;

    MAP_TimerIntClear(TIMER0_BASE, TIMER_CAPA_EVENT);
    //Disable interrupts so snap and time read are atomic
    MAP_IntMasterDisable();{
        wrap_cnt_snap = wrap_cnt;
        wrap_cap_cnt_snap = wrap_cap_cnt;
        cap_time = MAP_TimerValueGet(TIMER0_BASE, TIMER_A);
    } MAP_IntMasterEnable();
    

    // If cap count when wrap happens equal to timer A value when wrap_cnt was incremented was equal to cap timer, then
    // wrap must have been triggered during or after cap timer
    if(wrap_cap_cnt_snap == cap_cnt){
        // If capture time was shortly before wraparound point, let's say halfway
        // through counter (0xFFFF/2), than wraparound probably happened after
        // and thus decrement to compensate
        if(cap_time > (0xFFFF >> 1)){
            --wrap_cnt_snap;
        }  
    }


    if(cap_cnt == 0){
        t_start = (wrap_cnt_snap << 16) | cap_time;
    }else if (cap_cnt == 1){
        t_stop = (wrap_cnt_snap << 16) | cap_time;
        MAP_TimerDisable(TIMER0_BASE, TIMER_BOTH);
    }

    MAP_IntMasterDisable();{
        ++cap_cnt;
    } MAP_IntMasterEnable();
}/*}}}*/

/**
 * Counts how many times capture timer wraps around
 * This ISR should be of higher priority than exec_timer_cap_isr 
 */
void exec_timer_wrap_isr(void){/*{{{*/
    TimerIntClear(TIMER0_BASE, TIMER_TIMB_TIMEOUT);
    
    ++wrap_cnt; 
    wrap_cap_cnt = cap_cnt;
}/*}}}*/
/*}}}*/


// Power measurement stuff /*{{{*/
#define SAMPLE_PERIOD ((g_syshz*SAMPLE_PERIOD_US)/1000000)

void pwr_sample_setup(){
    MAP_IntPrioritySet(INT_TIMER1A, PWR_SAMPLE_ISR_PRIO);

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    MAP_SysCtlPeripheralReset(SYSCTL_PERIPH_TIMER1);
    
    MAP_TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    MAP_TimerLoadSet(TIMER1_BASE, TIMER_A, SAMPLE_PERIOD);

}

void pwr_sample_start(){
    MAP_TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    MAP_TimerEnable(TIMER0_BASE, TIMER_A);
}


/**
 * Timer interrupt notifies sample task to wake and perform one sample
 */
void pwr_sample_timer_isr(void){
    BaseType_t wake = pdFALSE;

    MAP_TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    vTaskNotifyGiveFromISR(pwr_sample_task_handle, &wake);

    // Should always be true, since pwr_sample_task is supposed to be higher
    // priority than any other task
    if(wake == pdTRUE){
        portYIELD_FROM_ISR(wake);
    }
}

void pwr_sample_task(void *args){
    uint32_t wrap_cnt_snap;
    uint32_t wrap_time_snap; 
    
    while(1){
        // Wait forever until notified by ISR, clear notification upon exit
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        taskENTER_CRITICAL();{
            wrap_cnt_snap = wrap_cnt;
            wrap_time_snap = MAP_TimerValueGet(TIMER0_BASE, TIMER_A); 
        }taskEXIT_CRITICAL();
        // Do measurement here 
    }
}
/*}}}*/
