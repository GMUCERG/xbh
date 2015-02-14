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

#define MAX_FIFO_BURST 8

/**
 * Global indicating XBH busy in I2C function. Since system most likely to hang
 * up in I2C, if watchdog interrupt triggered here, do not service watchdog
 */
bool g_inI2C = false;

/**
 * Flushes I2C fifos
 * @param base Base address of I2C transceiver
 */
void i2c_flush(uint32_t base){
    MAP_I2CTxFIFOFlush(base);
    MAP_I2CRxFIFOFlush(base);
}

/**
 * Sets up fifos and clock for I2C. Does not configure pins
 * @param base Base address of I2C transceiver
 * @param highspeed True if 400kbit, else 100kbit
 */
void i2c_setup(uint32_t base, bool highspeed){
    MAP_I2CMasterInitExpClk(base, g_syshz, highspeed);
    MAP_I2CTxFIFOConfigSet(base, I2C_FIFO_CFG_TX_MASTER);
    MAP_I2CRxFIFOConfigSet(base, I2C_FIFO_CFG_RX_MASTER);
    i2c_flush(base);
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
    size_t offset = 0;
    size_t len_mod = len % MAX_FIFO_BURST;
    size_t final_burst = (len_mod !=0 )? len_mod : MAX_FIFO_BURST;
    uint32_t err; 
    g_inI2C = true;
    
    MAP_I2CMasterSlaveAddrSet(base, addr, false);
    
    MAP_I2CMasterBurstLengthSet(base, MAX_FIFO_BURST);
    for(offset = 0; offset < (len-1)/MAX_FIFO_BURST; offset++){
        for(size_t i = 0; i < MAX_FIFO_BURST; ++i){
            I2CFIFODataPut(base, ((uint8_t *)data)[offset*MAX_FIFO_BURST+i]);
        }
        if( 0 == offset){
            I2CMasterControl(base, I2C_MASTER_CMD_FIFO_BURST_SEND_START);
        }else{
            I2CMasterControl(base, I2C_MASTER_CMD_FIFO_BURST_SEND_CONT);
        }
        while(I2CMasterBusy(base));
        err = I2CMasterErr(base);
        if(err != I2C_MASTER_ERR_NONE){
            goto error;
        }
    }
    MAP_I2CMasterBurstLengthSet(base, final_burst);
    for(size_t i = 0; i < final_burst; ++i){
        I2CFIFODataPut(base, ((uint8_t *)data)[offset*MAX_FIFO_BURST+i]);
    }
    if(len <= MAX_FIFO_BURST){
        I2CMasterControl(base, I2C_MASTER_CMD_FIFO_SINGLE_SEND);
    }else{
        I2CMasterControl(base, I2C_MASTER_CMD_FIFO_BURST_SEND_FINISH);
    }
    while(MAP_I2CMasterBusy(base));
    err = MAP_I2CMasterErr(base);
    if(err != I2C_MASTER_ERR_NONE){
        goto error;
    }
    while(!(MAP_I2CFIFOStatus(base) & I2C_FIFO_TX_EMPTY));
    g_inI2C = false;
    return 0;
error:
    MAP_I2CTxFIFOFlush(base);
    MAP_I2CMasterControl(base, I2C_MASTER_CMD_FIFO_BURST_SEND_ERROR_STOP);
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
    size_t offset = 0;
    size_t len_mod = len % MAX_FIFO_BURST;
    size_t final_burst = (len_mod !=0 )? len_mod : MAX_FIFO_BURST;
    uint32_t err; 
    g_inI2C = true;

    // Send configuration to INA219 config register
    MAP_I2CMasterSlaveAddrSet(base, addr, true);
    MAP_I2CMasterBurstLengthSet(base, MAX_FIFO_BURST);

    for(offset = 0; offset < (len-1)/MAX_FIFO_BURST; offset++){
        if(0 == offset){
            I2CMasterControl(base, I2C_MASTER_CMD_FIFO_BURST_RECEIVE_START);
        }else{
            I2CMasterControl(base, I2C_MASTER_CMD_FIFO_BURST_RECEIVE_CONT);
        }
        for(size_t i = 0; i < MAX_FIFO_BURST; ++i){
            ((uint8_t *)data)[offset*MAX_FIFO_BURST+i] = I2CFIFODataGet(base);
        }
        err = I2CMasterErr(base);
        if(err != I2C_MASTER_ERR_NONE){
            goto error;
        }
    }
    MAP_I2CMasterBurstLengthSet(base, final_burst);
    if(len <= MAX_FIFO_BURST){
        I2CMasterControl(base, I2C_MASTER_CMD_FIFO_SINGLE_RECEIVE);
    }else{
        I2CMasterControl(base, I2C_MASTER_CMD_FIFO_BURST_RECEIVE_FINISH);
    }
    for(size_t i = 0; i < final_burst; ++i){
        ((uint8_t *)data)[offset*MAX_FIFO_BURST+i] = I2CFIFODataGet(base);
    }
    err = MAP_I2CMasterErr(base);
    if(err != I2C_MASTER_ERR_NONE){
        goto error;
    }
    g_inI2C = false;
    return 0;
error:
    MAP_I2CRxFIFOFlush(base);
    MAP_I2CMasterControl(base, I2C_MASTER_CMD_FIFO_BURST_RECEIVE_ERROR_STOP);
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

