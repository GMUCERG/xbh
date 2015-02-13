#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include <driverlib/rom.h>
#include <driverlib/rom_map.h>

#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/i2c.h>

#include "hal/hal.h"
#include "hal/i2c_comm.h"

/**
 * Global indicating XBH busy in I2C function. Since system most likely to hang
 * up in I2C, if watchdog interrupt triggered here, do not service watchdog
 */
bool g_inI2C = false;

/**
 * Sets up fifos and clock for I2C. Does not configure pins
 * @param base Base address of I2C transceiver
 * @param highspeed True if 400kbit, else 100kbit
 */
void i2c_setup(uint32_t base, bool highspeed){
    MAP_I2CMasterInitExpClk(base, g_syshz, highspeed);
}

/**
 * Writes array over I2C to address given
 * @param base Base address of I2C transceiver
 * @param addr Address of I2C device to write to
 * @param data Data to write
 * @param len Length of data
 * @return 0 if success, else value of I2CMasterErr
 */
int i2c_write(uint32_t base, uint8_t addr, const void *data, size_t len){
    uint32_t err; 
    g_inI2C = true;


    
    MAP_I2CMasterSlaveAddrSet(base, addr, false);
    while(MAP_I2CMasterBusy(base));
    for(size_t i = 0; i < len; ++i){
        MAP_I2CMasterDataPut(base, ((uint8_t *)data)[i]);
        //while(MAP_I2CMasterBusy(base));
        if(0 == i){
            if(len == 1){
                MAP_I2CMasterControl(base, I2C_MASTER_CMD_SINGLE_SEND);
            }else{
                MAP_I2CMasterControl(base, I2C_MASTER_CMD_BURST_SEND_START);
            }
        }else if(len-1 == i){
            MAP_I2CMasterControl(base, I2C_MASTER_CMD_BURST_SEND_FINISH);
        }else{
            MAP_I2CMasterControl(base, I2C_MASTER_CMD_BURST_SEND_CONT);
        }
        while(MAP_I2CMasterBusy(base));
    }
    err = MAP_I2CMasterErr(base);
    if(err != I2C_MASTER_ERR_NONE){
        goto error;
    }
    g_inI2C = false;
    return 0;
error:
    MAP_I2CMasterControl(base, I2C_MASTER_CMD_BURST_SEND_ERROR_STOP);
    g_inI2C = false;
    return err;
}

/**
 * Reads array over I2C from address given
 * @param base Base address of I2C transceiver
 * @param addr Address of I2C device to read from
 * @param data Data to read
 * @param len Length of data
 * @return 0 if success, else value of I2CMasterErr
 */
int i2c_read(uint32_t base, uint8_t addr, void *data, size_t len){
    uint32_t err; 
    g_inI2C = true;
    
    //while(MAP_I2CMasterBusy(base));
    MAP_I2CMasterSlaveAddrSet(base, addr, true);
    //while(MAP_I2CMasterBusy(base));
    for(size_t i = 0; i < len; ++i){
        if(0 == i){
            if(len == 1){
                MAP_I2CMasterControl(base, I2C_MASTER_CMD_SINGLE_RECEIVE);
            }else{
                MAP_I2CMasterControl(base, I2C_MASTER_CMD_BURST_RECEIVE_START);
            }
        }else if(len-1 == i){
            MAP_I2CMasterControl(base, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
        }else{
            MAP_I2CMasterControl(base, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
        }
        while(MAP_I2CMasterBusy(base));
        ((uint8_t *)data)[i] = MAP_I2CMasterDataGet(base);
        //while(MAP_I2CMasterBusy(base));
    }
    err = MAP_I2CMasterErr(base);
    if(err != I2C_MASTER_ERR_NONE){
        goto error;
    }
    g_inI2C = false;
    return 0;
error:
    MAP_I2CMasterControl(base, I2C_MASTER_CMD_BURST_RECEIVE_ERROR_STOP);
    g_inI2C = false;
    return err;
}


void i2c_comm_setup(void){

#if 0
    //Debug stuff
    MAP_GPIOPinTypeGPIOOutputOD(GPIO_PORTB_BASE, GPIO_PIN_3);
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_2);
    while(1){
        MAP_GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_2);
        MAP_GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_3);
    }
#else
    // Configure I2C pins

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
    MAP_SysCtlPeripheralReset(SYSCTL_PERIPH_I2C0);
    MAP_GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    MAP_GPIOPinConfigure(GPIO_PB3_I2C0SDA);
    MAP_GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);
    MAP_GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);

    // Configure I2C master and fifos, and flush fifos
    i2c_setup(I2C0_BASE, true);
#endif
}

// For linking
extern inline int i2c_comm_write(uint8_t addr, const void *data, size_t len);
extern inline int i2c_comm_read(uint8_t addr, void *data, size_t len);

