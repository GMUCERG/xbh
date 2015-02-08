#ifndef _I2C_COMM_H
#define _I2C_COMM_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <inc/hw_memmap.h>

#include "hal/watchdog.h"

#define I2C_COMM_BASE I2C0_BASE


// Do not use these 4 functions outside HAL, since device-specific base
// parameter required 
///*{{{*/
void i2c_flush(uint32_t base);
void i2c_setup(uint32_t base, bool highspeed);
int i2c_write(uint32_t base, uint8_t addr, const void *data, size_t len);
int i2c_read(uint32_t base, uint8_t addr, void *data, size_t len);
/*}}}*/

void i2c_comm_setup(void);
void i2c_comm_abort(void);
int i2c_comm_write(uint8_t addr, const void *data, size_t len);
int i2c_comm_read(uint8_t addr, void *data, size_t len);
#endif
