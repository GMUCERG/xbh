#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include <driverlib/rom.h>
#include <driverlib/rom_map.h>

#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <driverlib/gpio.h>
#include <driverlib/interrupt.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/timer.h>
#include <driverlib/adc.h>

#include "FreeRTOSConfig.h"
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "hal/hal.h"
#include "hal/measure.h"
#include "hal/isr_prio.h"
#include "hal/i2c_comm.h"

#include "util.h"

#define PWR_SAMP_STACK 256
#define SAMPLE_PERIOD_US 200




/** Sampling task handle */
static TaskHandle_t pwr_sample_task_handle;
QueueHandle_t pwr_sample_q_handle;

// Timer capture variables/*{{{*/
static volatile uint32_t wrap_cnt;
volatile uint64_t t_start;
volatile uint64_t t_stop;
static volatile uint32_t cap_cnt;
/*}}}*/

// holds ADC sample
uint32_t pui32ADC0Value[1];
//holds voltage values
float voltage=0;
//flag to check if interrupt occurred
static volatile bool g_bIntFlag = false;


// Execution timer stuff/*{{{*/

/**
 * Sets up execution timer
 */
static void exec_timer_setup(void){/*{{{*/
    // Enable and reset timer 0
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    MAP_SysCtlPeripheralReset(SYSCTL_PERIPH_TIMER0);

    // PL4 is attached to timer0
    MAP_GPIOPinConfigure(GPIO_PL4_T0CCP0);
    MAP_GPIOPinTypeTimer(GPIO_PORTL_BASE, GPIO_PIN_4);
    
    // Split needed for timer capture
    // We use timer A for capture and timer B to count wraparounds of timer A
    MAP_TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR|TIMER_CFG_A_CAP_TIME_UP|TIMER_CFG_B_PERIODIC_UP);

    // Detect both rising and falling edges
    MAP_TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_BOTH_EDGES);

    // Set priorities for the timers
    MAP_IntPrioritySet(INT_TIMER0A, TIMER_CAP_ISR_PRIO);
    MAP_IntPrioritySet(INT_TIMER0B, TIMER_WRAP_ISR_PRIO);

    MAP_IntEnable(INT_TIMER0A);
    MAP_IntEnable(INT_TIMER0B);

    MAP_TimerIntEnable(TIMER0_BASE, TIMER_CAPA_EVENT);
    MAP_TimerIntEnable(TIMER0_BASE, TIMER_TIMB_TIMEOUT);
}/*}}}*/


/**
 * Starts execution timer, enabling interrupts
 */
void exec_timer_start(void){/*{{{*/
    wrap_cnt = 0;
    t_start= 0;
    t_stop= 0;
    cap_cnt = 0;

    MAP_TimerSynchronize(TIMER0_BASE, TIMER_0A_SYNC|TIMER_0B_SYNC);
    MAP_TimerEnable(TIMER0_BASE, TIMER_BOTH);

}/*}}}*/

/**
 * Records time that execution started and stopped, triggered by rising/falling
 * edge on timer capture pin
 * Not atomic, since can be interrupted by wrap_isr
 */
void exec_timer_cap_isr(void){/*{{{*/
    uint32_t cap_time;
    uint32_t after_cap_time;
    uint32_t wrap_cap_cnt;

    DEBUG_OUT("cap_cnt: %u\n", cap_cnt);


    //----------Prev. code, more efficient but doesn't work (overflow???)------------
    cap_time = MAP_TimerValueGet(TIMER0_BASE, TIMER_A);
    wrap_cap_cnt = wrap_cnt;
    after_cap_time = MAP_TimerValueGet(TIMER0_BASE, TIMER_B);

    if(cap_time > after_cap_time){
        //rollover
        wrap_cap_cnt = wrap_cnt-1;

    }


    if(0 == cap_cnt){
        t_start = (wrap_cap_cnt << 16) | cap_time;
        cap_cnt = cap_cnt^1;
    }else if (1 == cap_cnt ){
        t_stop = (wrap_cap_cnt << 16) | cap_time;
        MAP_TimerDisable(TIMER0_BASE, TIMER_BOTH);
        cap_cnt = cap_cnt^1;

        DEBUG_OUT("wrap_cnt: %u\n", wrap_cap_cnt);
        DEBUG_OUT("cap_time: %u\n", cap_time);
        DEBUG_OUT("t_start: %lu\n", t_start);
        DEBUG_OUT("t_stop: %llu\n", t_stop);
        DEBUG_OUT("t_elapsed: %lld\n", (int32_t)t_stop-t_start);
    }


   

    //Clear at the end of the interrupt, so that wrap_isr knows if it has
    //preempted this interrupt.
    MAP_TimerIntClear(TIMER0_BASE, TIMER_CAPA_EVENT);
}/*}}}*/

/**
 * Counts how many times capture timer wraps around
 * This ISR should be of higher priority than exec_timer_cap_isr 
 * If we have interrupted the timer capture ISR, do not increme
 */
void exec_timer_wrap_isr(void){/*{{{*/
    ++wrap_cnt;
    MAP_TimerIntClear(TIMER0_BASE, TIMER_TIMB_TIMEOUT);
}/*}}}*/
/*}}}*/


// Power measurement stuff /*{{{*/
#define SAMPLE_PERIOD ((g_syshz*SAMPLE_PERIOD_US)/1000000)

static float pwr_sample_setup(){/*{{{*/
    
    // The ADC0 peripheral must be enabled for use.
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    
    // ADC0 is used with AIN0 on port PE3
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    
    // Select the analog ADC function for these pins.
    MAP_GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);
    
    // Enable sample sequence 3 with a processor signal trigger.Sequence 3
    // will do a single sample when the processor sends a signal to start the
    // conversion.
    MAP_ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_TIMER, 0);
    
    // Configure step 0 on sequence 3.  Sample channel 0 (ADC_CTL_CH0) in
    // single-ended mode (default) and configure the interrupt flag
    // (ADC_CTL_IE) to be set when the sample is done.  Tell the ADC logic
    // that this is the last conversion on sequence 3 (ADC_CTL_END).
    MAP_ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE |
                             ADC_CTL_END);
    
    // Since sample sequence 3 is now configured, it must be enabled.
    MAP_ADCSequenceEnable(ADC0_BASE, 3);
    
    // Clear the interrupt status flag.  This is done to make sure the
    // interrupt flag is cleared before we sample.
    MAP_ADCIntClear(ADC0_BASE, 3);
    
    // The Timer0 peripheral must be enabled for use.
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    
    // Configure Timer0B as a 32-bit periodic timer.
    MAP_TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);

    // Set the Timer0A load value to 1ms.
    MAP_TimerLoadSet(TIMER1_BASE, TIMER_A, SAMPLE_PERIOD);

    // Enable triggering
    MAP_TimerControlTrigger(TIMER1_BASE, TIMER_A, true);

    // Enable processor interrupts.
    MAP_IntMasterEnable();

    //Enable ADC interrupt
    MAP_ADCIntEnable(ADC0_BASE, 3);

    //enable the ADC0 Sequencer 3 Interrupt in the NVIC
    MAP_IntEnable(INT_ADC0SS3);

    // Enable Timer0A.
    MAP_TimerEnable(TIMER1_BASE, TIMER_A);
    
    while(1)
    {
        while(!g_bIntFlag)
        {

        }
        
        voltage = (3.3/4096)*pui32ADC0Value[0];
    }
    

}/*}}}*/

void ADC0IntHandler(void) {

    // Clear the ADC interrupt flag.
    MAP_ADCIntClear(ADC0_BASE, 3);

    // Read ADC Value.
    MAP_ADCSequenceDataGet(ADC0_BASE, 3, pui32ADC0Value);
    g_bIntFlag = true;

}

/*static void pwr_sample_start(){{{{
    BaseType_t retval;

    MAP_TimerEnable(TIMER1_BASE, TIMER_A);

    retval = xTaskCreate( pwr_sample_task,
            "pwr_samp",
            PWR_SAMP_STACK,
            NULL,
            PWR_SAMP_PRIO,
            &pwr_sample_task_handle);
    pwr_sample_q_handle = xQueueCreate(32, sizeof(struct pwr_sample));
    COND_ERRMSG(retval != pdPASS, "Could not create power sampling task\n");
}}}}*/

/*}}}*/


/**
 * Sets up measurement hardware. Should be called from HAL init.
 */
void measure_setup(void){
    exec_timer_setup();
//    pwr_sample_setup();
}


/**
 * Kicks off measurement start, starting timers, etc
 */
void measure_start(void){
    exec_timer_start();
    pwr_sample_start();
    MAP_TimerSynchronize(TIMER0_BASE, TIMER_0A_SYNC|TIMER_0B_SYNC|TIMER_1A_SYNC);
}


/**
 * Returns false if exection is complete
 * Basically checks if t_stop is nonzero
 */
bool measure_isrunning(void){
    return (0 == t_stop);
}

/**
 * Gets stop time
 * Locking not required since this should only be run after timer is disabled
 * We make this a function so it can be added later if needed
 * @return Stop time
 */
uint64_t measure_get_stop(void){
    uint64_t time;
    time = t_stop;
    return time;
}

/**
 * Gets start time
 * Locking not required since this should only be run after timer is disabled
 * We make this a function so it can be added later if needed
 * @return Start time
 */
uint64_t measure_get_start(void){
    uint64_t time;
    time = t_start;
    return time;
}

/**
 * Gets power reading
 * @return power reading
 */
float measure_get_power(void){
    float power;
    power = voltage;
    return power;
}
