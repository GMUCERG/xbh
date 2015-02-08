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

#define TIME_MAX 0xFFFFFFFF
static bool watchdog_en = false;

void watchdog_setup(void){
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);
    if(MAP_WatchdogLockState(WATCHDOG0_BASE) == true) {
        MAP_WatchdogUnlock(WATCHDOG0_BASE);
    }
    MAP_WatchdogResetDisable(WATCHDOG0_BASE);
    MAP_WatchdogReloadSet(WATCHDOG0_BASE, TIME_MAX);
    MAP_IntPrioritySet(INT_WATCHDOG, WATCHDOG_ISR_PRIO);
//    MAP_WatchdogStallEnable(WATCHDOG0_BASE);
    MAP_WatchdogIntEnable(WATCHDOG0_BASE);
}

void watchdog_refresh(uint32_t ms){
    uint32_t t_wd = g_syshz/1000*ms;
    MAP_WatchdogReloadSet(WATCHDOG0_BASE, t_wd);
}

void watchdog_start(uint32_t ms){
    watchdog_refresh(ms);
//    MAP_WatchdogResetEnable(WATCHDOG0_BASE);
    watchdog_en = true;
}
void watchdog_stop(void){
    MAP_WatchdogResetDisable(WATCHDOG0_BASE);
    MAP_WatchdogReloadSet(WATCHDOG0_BASE, TIME_MAX);
    watchdog_en = false;
}
void watchdog_isr(void){
    MAP_WatchdogIntClear(WATCHDOG0_BASE);
    if(watchdog_en){
        i2c_comm_abort();
    }
}


