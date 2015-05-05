#include <stdint.h>
#include <stdbool.h>
#include <inc/hw_ints.h>

#include <driverlib/rom.h>
#include <driverlib/rom_map.h>

#include <driverlib/sysctl.h>
#include <driverlib/watchdog.h>
#include <driverlib/interrupt.h>

#include "hal/watchdog.h"
#include "hal/isr_prio.h"
#include "hal/hal.h"
#include "hal/i2c_comm.h"
#include "util.h"

// Set watchdog to reset after 180 secnods
#define WATCHDOG_INTERVAL 180000

void watchdog_setup(void){
#ifndef DISABLE_WATCHDOG
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);
    watchdog_setinterval(WATCHDOG_INTERVAL);
    MAP_WatchdogStallEnable(WATCHDOG0_BASE);
    MAP_IntPrioritySet(INT_WATCHDOG, WATCHDOG_ISR_PRIO);
    MAP_IntEnable(INT_WATCHDOG);
    MAP_WatchdogResetEnable(WATCHDOG0_BASE);
    MAP_WatchdogEnable(WATCHDOG0_BASE);
#endif
}

void watchdog_setinterval(uint32_t ms){
    uint32_t t_wd = g_syshz/1000*ms;
    MAP_WatchdogReloadSet(WATCHDOG0_BASE, t_wd);
}

void watchdog_isr(void){
    static uint8_t wd_count = 0; 
    //If watchdog triggered twice while in i2c, trigger reset immediately.
    //If ISR is somehow stuck, reset will also occur.

    if(g_inI2C){
        wd_count++;
    }else{
        wd_count = 0;
    }
    if(wd_count < 2){
        MAP_WatchdogIntClear(WATCHDOG0_BASE);
        return;
    }else{
        //Trigger reset immediately 
        MAP_SysCtlReset();
    }
}


