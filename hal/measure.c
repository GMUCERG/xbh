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

#define PWR_SAMP_STACK 256
#define SAMPLE_PERIOD_US 200



/** Sampling task handle */
static TaskHandle_t pwr_sample_task_handle;
QueueHandle_t pwr_sample_q_handle;

// Timer capture variables/*{{{*/
static volatile uint32_t wrap_cnt;
static volatile uint32_t wrap_cap_cnt;
volatile uint64_t t_start;
volatile uint64_t t_stop;
static volatile uint32_t cap_cnt;
/*}}}*/


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
static void exec_timer_start(void){/*{{{*/
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


    if(0 == cap_cnt){
        t_start = (wrap_cnt_snap << 16) | cap_time;
    }else if (1 == cap_cnt ){
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

/** Refer to page 25 in ina219.pdf */
#define INA219_REG_CONF 0x0
#define INA219_REG_VOLT 0x2
#define INA219_REG_POWR 0x3
#define INA219_REG_CURR 0x4
#define INA219_REG_CAL  0x5
#define INA219_CONF (1<<15|1<<13|2<<11|1<<7|1<<2|7)
#define INA219_CAL 0x1000 //10uA increments, w/ 1Ohm resistor
#define INA219_CNVR 0x2
#define INA219_OVF 0x1
#define INA219_I2C_ADDR 0x46

#ifdef USE_SEPARATE_I2C
#define I2C_WRITE(addr, data, len) i2c_write(I2C2_BASE, addr, data, len)
#define I2C_READ(addr, data, len) i2c_read(I2C2_BASE, addr, data, len)
#else
#define I2C_WRITE(addr, data, len) i2c_comm_write(addr, data, len)
#define I2C_READ(addr, data, len) i2c_comm_read(addr, data, len)
#endif



static void pwr_sample_setup(){/*{{{*/
    uint8_t cmd_buf[3];
    // Configure interrupt priority for periodic sampling timer
    MAP_IntPrioritySet(INT_TIMER1A, PWR_SAMPLE_ISR_PRIO);

    // Enable timer1 as periodic sampling timer
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    MAP_SysCtlPeripheralReset(SYSCTL_PERIPH_TIMER1);
    
    // Set as periodic and load period
    MAP_TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    MAP_TimerLoadSet(TIMER1_BASE, TIMER_A, SAMPLE_PERIOD);

    MAP_IntEnable(INT_TIMER1A);
    MAP_TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

#ifdef USE_SEPARATE_I2C
#define INA219_I2C_BASE I2C2_BASE

    // Configure I2C pins
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C7);
    MAP_SysCtlPeripheralReset(SYSCTL_PERIPH_I2C7);
    MAP_GPIOPinConfigure(GPIO_PD0_I2C7SCL);
    MAP_GPIOPinConfigure(GPIO_PD1_I2C7SDA);
    MAP_GPIOPinTypeI2CSCL(GPIO_PORTD_BASE, GPIO_PIN_0);
    MAP_GPIOPinTypeI2C(GPIO_PORTD_BASE, GPIO_PIN_1);

    // Configure I2C master and fifos, and flush fifos
    i2c_setup(I2C2_BASE, true);
#endif


    // Send configuration to INA219 config register
    cmd_buf[0] = INA219_REG_CONF;
    cmd_buf[1] = INA219_CONF >> 8; 
    cmd_buf[2] = INA219_CONF & 0xFF;
    I2C_WRITE(INA219_I2C_ADDR, cmd_buf, sizeof(cmd_buf));

    // Write calibration
    cmd_buf[0] = INA219_REG_CAL; 
    cmd_buf[1] = INA219_CAL >> 8; 
    cmd_buf[2] = INA219_CAL & 0xFF;
    I2C_WRITE(INA219_I2C_ADDR, cmd_buf, sizeof(cmd_buf));
}/*}}}*/

/**
 * Timer interrupt notifies sample task to wake and perform one sample
 */
void pwr_sample_timer_isr(void){/*{{{*/
    BaseType_t wake = pdFALSE;

    MAP_TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    vTaskNotifyGiveFromISR(pwr_sample_task_handle, &wake);

    // Should always be true, since pwr_sample_task is supposed to be higher
    // priority than any other task
    if(pdTRUE == wake){
        portYIELD_FROM_ISR(wake);
    }
}/*}}}*/

static void pwr_sample_task(void *args){/*{{{*/
    uint32_t wrap_cnt_snap;
    uint32_t wrap_time_snap; 

    struct pwr_sample sample; 
    
    // Run while no stop time set on capture timer
    while(0 == t_stop){
        bool valid;
        
        do{
            uint8_t reg; 

            //Read current
            reg = INA219_REG_CURR;
            I2C_WRITE(INA219_I2C_ADDR, &reg, 1);
            I2C_READ(INA219_I2C_ADDR, &sample.current, 2);

            //Read power 
            reg = INA219_REG_POWR;
            I2C_WRITE(INA219_I2C_ADDR, &reg, 1);
            I2C_READ(INA219_I2C_ADDR, &sample.power, 2);

            //Read voltage. 
            reg = INA219_REG_VOLT;
            I2C_WRITE(INA219_I2C_ADDR, &reg, 1);
            I2C_READ(INA219_I2C_ADDR, &sample.voltage, 2);

            //TI documentation is contradictory on whether power or voltage read
            //triggers conversion. TODO: Verify what happens
            sample.voltage = ntohs(sample.voltage);
            valid = (sample.voltage & INA219_CNVR) && !(sample.voltage & INA219_OVF);
            sample.voltage = htons(sample.voltage >> 3);
        }while(!valid);

        //Pick up timestamp where valid value is available
        //Can't use taskENTER_CRITICAL since wrap counter is higher priority
        IntMasterDisable();{
            wrap_cnt_snap = wrap_cnt;
            wrap_time_snap = MAP_TimerValueGet(TIMER0_BASE, TIMER_A); 
        }IntMasterEnable();
        
        sample.timestamp = wrap_cnt_snap << 16 | wrap_time_snap;

        COND_ERRMSG(
                pdTRUE != xQueueSendToBack(pwr_sample_q_handle, &sample, 0), 
                "Power Sample queue full!\n" );

        // Wait forever until notified by ISR, clear notification upon exit
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
#if DEBUG_STACK
        DEBUG_OUT("Stack Usage: %s: %d\n", __PRETTY_FUNCTION__, uxTaskGetStackHighWaterMark(NULL));
#endif
    }
    // Delete task when done
    MAP_TimerDisable(TIMER1_BASE, TIMER_A);
    vTaskDelete(NULL);
}/*}}}*/

static void pwr_sample_start(){/*{{{*/
    BaseType_t retval;

    MAP_TimerEnable(TIMER1_BASE, TIMER_A);

    i2c_flush(I2C2_BASE);

    retval = xTaskCreate( pwr_sample_task,
            "pwr_samp",
            PWR_SAMP_STACK,
            NULL,
            PWR_SAMP_PRIO,
            &pwr_sample_task_handle);
    pwr_sample_q_handle = xQueueCreate(32, sizeof(struct pwr_sample));
    COND_ERRMSG(retval != pdPASS, "Could not create power sampling task\n");
}/*}}}*/

/*}}}*/


/**
 * Sets up measurement hardware. Should be called from HAL init.
 */
void measure_setup(void){
    exec_timer_setup();
    pwr_sample_setup();
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
 * @return Stop time
 */
uint64_t measure_get_stop(void){
    uint64_t time;
    MAP_IntMasterDisable();{
        time= t_stop;
    }
    return time;
}

/**
 * Gets start time
 * @return Start time
 */
uint64_t measure_get_start(void){
    uint64_t time;
    MAP_IntMasterDisable();{
        time= t_start;
    }
    return time;
}

/**
 * Kicks off only timer and not i2c power measurements
 */
void timer_cal_start(void){
    exec_timer_start();
}
