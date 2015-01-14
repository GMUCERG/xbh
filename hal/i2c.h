#ifndef _I2C_H
#define _I2C_H

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <inc/hw_memmap.h>

#define I2C_COMM_BASE I2C9_BASE

// Do not use these 4 functions outside HAL, since device-specific base
// parameter required 
///*{{{*/
void i2c_flush(uint32_t base);
void i2c_setup(uint32_t base, bool highspeed);
uint32_t i2c_write(uint32_t base, uint8_t addr, void *data, size_t len);
uint32_t i2c_read(uint32_t base, uint8_t addr, void *data, size_t len);
/*}}}*/

/** Sets up i2c tranceiver for XBD comm */
void i2c_comm_setup(void);

/**
 * Reads array over I2C from address given
 * @param addr Address of I2C device to read from
 * @param data Data to read
 * @param len Length of data
 * @return 0 if success, 1 if error
 */
inline int i2c_comm_write(uint8_t addr, void *data, size_t len){
    return i2c_write(I2C_COMM_BASE, addr, data, len);
}
/**
 * Writes array over I2C to address given
 * @param addr Address of I2C device to write to
 * @param data Data to write
 * @param len Length of data
 * @return 0 if success, 1 if error
 */
inline int i2c_comm_read(uint8_t addr, void *data, size_t len){
    return i2c_read(I2C_COMM_BASE, addr, data, len);
}


#endif
