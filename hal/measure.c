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

#include "FreeRTOSConfig.h"
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include "hal/hal.h"
#include "hal/measure.h"
#include "hal/isr_prio.h"
#include "hal/i2c_comm.h"

#include "util.h"
#include "driverlib/adc.h"
#define PWR_SAMP_STACK 256
#define SAMPLE_PERIOD_US 200



/** Sampling task handle */
//static TaskHandle_t pwr_sample_task_handle;
//QueueHandle_t pwr_sample_q_handle;

// Timer capture variables/*{{{*/
static volatile uint32_t wrap_cnt;
// Used to capture wrap value if wrap interrupts capture
static volatile uint32_t wrap_cap_cnt;
volatile uint64_t t_start;
volatile uint64_t t_stop;
static volatile uint32_t cap_cnt;
/*}}}*/

//Power measurement variables 
static volatile uint32_t avgPwr;  // upper 3 nibbles get ADC values
static volatile uint32_t maxPwr;  // direct ADC values
static volatile uint32_t gain;    // 0 -> 25, 1 -> 50, 2 -> 100, 3 -> 200 gain
static volatile uint32_t cntover; //set to 1 when avgcnt reaches 0x000FFFFF
static volatile uint32_t avgcnt;
uint32_t ADC0Value[1];


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

    MAP_TimerEnable(TIMER0_BASE, TIMER_BOTH);
}/*}}}*/

/**
 * Records time that execution started and stopped, triggered by rising/falling
 * edge on timer capture pin
 * Not atomic, since can be interrupted by wrap_isr
 */
void exec_timer_cap_isr(void){/*{{{*/
    uint32_t cap_time;

    //Can't disable interrupts to make operations atomic, since otherwise missed
    //wrap
    //dint();{
    cap_time = MAP_TimerValueGet(TIMER0_BASE, TIMER_A);
    //} eint();

    if(0 == cap_cnt){
        t_start = (wrap_cnt << 16) | cap_time;
    }else if (1 == cap_cnt ){
        t_stop = (wrap_cnt << 16) | cap_time;
        MAP_TimerDisable(TIMER0_BASE, TIMER_BOTH);
        DEBUG_OUT("wrap_cnt: %u\n", wrap_cnt);
        DEBUG_OUT("cap_time: %u\n", cap_time);
        DEBUG_OUT("t_start: %lu\n", t_start);
        DEBUG_OUT("t_stop: %llu\n", t_stop);
        DEBUG_OUT("t_elapsed: %lld\n", (int32_t)t_stop-t_start);
    }
    ++cap_cnt;
    
    // If timer wrap ISR has run, update wrap_cnt
    if(wrap_cap_cnt > wrap_cnt){
        wrap_cnt = wrap_cap_cnt;
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
    MAP_TimerIntClear(TIMER0_BASE, TIMER_TIMB_TIMEOUT);
    
    if(MAP_TimerIntStatus(TIMER0_BASE, true)&TIMER_CAPA_EVENT){
        wrap_cap_cnt = wrap_cnt;
        ++wrap_cap_cnt;
    }else{
        if(wrap_cap_cnt > wrap_cnt){
            wrap_cnt = wrap_cap_cnt;
        }
        ++wrap_cnt; 
    }
}/*}}}*/
/*}}}*/


// Power measurement stuff /*{{{*/
#define SAMPLE_PERIOD ((g_syshz*SAMPLE_PERIOD_US)/1000000)

 void power_measure_setup(void){
    
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    //
    // For this example ADC0 is used with AIN0 on port E7.
    // The actual port and pins used may be different on your part, consult
    // the data sheet for more information.  GPIO port E needs to be enabled
    // so these pins can be used.
    // TODO: change this to whichever GPIO port you are using.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    //
    // Select the analog ADC function for these pins.
    // Consult the data sheet to see which functions are allocated per pin.
    // TODO: change this to select the port/pin you are using.
    //
    MAP_GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0);
    // Enable sample sequence 3 with a processor signal trigger.  Sequence 3
    // will do a single sample when the processor sends a signal to start the
    // conversion.  Each ADC module has 4 programmable sequences, sequence 0
    // to sequence 3.  This example is arbitrarily using sequence 3.
    //
    MAP_ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_TIMER , 0);
    //
    // Configure step 0 on sequence 3.  Sample channel 0 (ADC_CTL_CH0) in
    // single-ended mode (default) and configure the interrupt flag
    // (ADC_CTL_IE) to be set when the sample is done.  Tell the ADC logic
    // that this is the last conversion on sequence 3 (ADC_CTL_END).  Sequence
    // 3 has only one programmable step.  Sequence 1 and 2 have 4 steps, and
    // sequence 0 has 8 programmable steps.  Since we are only doing a single
    // conversion using sequence 3 we will only configure step 0.  For more
    // information on the ADC sequences and steps, reference the datasheet.
    //
    MAP_ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);
    //
    // Since sample sequence 3 is now configured, it must be enabled.
    //
    MAP_ADCSequenceEnable(ADC0_BASE, 3);
    //
    // Clear the interrupt status flag.  This is done to make sure the
    // interrupt flag is cleared before we sample.
    //
    MAP_ADCIntClear(ADC0_BASE, 3);
    /*
     * Timer
     */
    // The Timer1 peripheral must be enabled for use.
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    // Configure Timer1A as a 32-bit periodic timer.
    MAP_TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    // Set the Timer1A load value to 1ms.
    //TimerLoadSet(TIMER1_BASE, TIMER_A, SAMPLE_PERIOD);
    #define F_SAMPLE    1000
    MAP_TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet()/F_SAMPLE );
    // Enable triggering
    // TimerControlTrigger(TIMER1_BASE, TIMER_A, true);
    MAP_TimerControlTrigger(TIMER1_BASE, TIMER_A, true );
    //MAP_IntMasterEnable();
    MAP_IntPrioritySet(INT_TIMER0B, PWR_SAMPLE_ISR_PRIO);
    //Enable ADC interrupt
    MAP_ADCIntEnable(ADC0_BASE, 3);
    //enable the ADC0 Sequencer 3 Interrupt in the NVIC
    MAP_IntEnable(INT_ADC0SS3);
}
    

/**
 * Starts execution timer, enabling interrupts
 */
void  power_measure_start(void){/*{{{*/
    // Enable Timer1A.
    MAP_TimerEnable(TIMER1_BASE, TIMER_A);
}

void ADC0SS3_HANDLER(void)
{
    MAP_ADCIntClear(ADC0_BASE, 3);
    // Read ADC Value.
    MAP_ADCSequenceDataGet(ADC0_BASE, 3, ADC0Value);

    avgPwr = ADC0Value[0];

    //leftshift=left<<20;


   // current = (0.0008/4096)*pui32ADC0Value[0];
   // current = 0.0008*pui32ADC0Value[0];
   // right=  (0.3/4096)*leftshift;
    // right=  current;
   // rightshift=right>>20;
   //right = current;
    //right = current>>20;
   // unit32_t int num = 0;

/*
    float valTemp = current;
    num_c = (int) valTemp;
    f_c = valTemp - (float)num_c;
    f_c = f_c * 10000;

    power = current * voltage;

    float valTemp1 = power;
        num_p = (int) valTemp1;
        f_p = valTemp1 - (float)num_p;
        f_p = f_p * 10000;

    i++;


     avgPwr = avgPwr * (i-1)/i + (power/i);
     float valTemp2 =  avgPwr;
             num_ap = (int) valTemp2;
             f_ap = valTemp1 - (float)num_ap;
             f_ap = f_ap * 10000;
 DEBUG_OUT("Power Mwasurenment");

            if(power > maxPwr) maxPwr = power;
            float valTemp3 =  maxPwr;
                        num_mp = (int) valTemp3;
                        f_mp = valTemp3 - (float)num_mp;
                        f_mp = f_mp * 10000;
                        */
                       // DEBUG_OUT("\nADC input value= %d \nADC Current measured=%4d.%4d\nPower=%4d.%4d\nAverage Power=%4d.%4d \nMaximum Power=%4d.%4d",left,(int)num_c,(int)f_c, (int)num_p,(int)f_p, (int)num_ap,(int)f_ap, (int)num_mp,(int)f_mp);
                        
    DEBUG_OUT("\nADC input value= %d", avgPwr);
                        
                        
            //UARTprintf("ADC Current measured = %4d.%4d\r\r\r\r\r\n  Power = %4d\r \n  Average Power = %4d\r \n Maximum Power = %4d\r",current, power, avgPwr, maxPwr);
           // UARTprintf("\nADC input value= %d \nADC Current measured=%4d.%4d\nPower=%4d.%4d\nAverage Power=%4d.%4d \nMaximum Power=%4d.%4d",left,(int)num_c,(int)f_c, (int)num_p,(int)f_p, (int)num_ap,(int)f_ap, (int)num_mp,(int)f_mp);
  
                         
    MAP_TimerDisable(TIMER1_BASE, TIMER_A);

}

/*}}}*/


/**
 * Sets up measurement hardware. Should be called from HAL init.
 */
void measure_setup(void){
    exec_timer_setup();
    power_measure_setup();
}


/**
 * Kicks off measurement start, starting timers, etc
 */
//void measure_start(void){
//    exec_timer_start();
//    power_measure_start();
//    MAP_TimerSynchronize(TIMER0_BASE, TIMER_0A_SYNC|TIMER_0B_SYNC|TIMER_1A_SYNC);
//}


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
 * Gets avg power reading
 * @return avg power reading
 */
uint32_t measure_get_avg(void){
    uint32_t l_avgPwr;
    l_avgPwr = avgPwr;
    return l_avgPwr;
}

/**
 * Gets max reading
 * @return max reading
 */
uint32_t measure_get_max(void){
    return maxPwr;
}


/**
 * Gets gain of the XBP
 * @return gain of XBP
 */
uint32_t measure_get_gain(void){
    return gain;
}

/**
 * Gets average counter overflow
 * @return counter overflow
 */
uint32_t measure_get_cntover(void){
    return cntover;
}
