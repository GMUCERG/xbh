#ifndef _HAL_WATCHDOG_H_
#define _HAL_WATCHDOG_H_
#include <driverlib/rom_map.h>
#include <driverlib/watchdog.h>

#include "hal/watchdog.h"
#include "hal/hal.h"
#include "hal/i2c_comm.h"

void watchdog_setup(void);
void watchdog_refresh(uint32_t ms);
void watchdog_start(uint32_t ms);
void watchdog_stop(void);
void watchdog_isr(void);


#endif
