#ifndef _HAL_CRC_H_
#define _HAL_CRC_H_
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

//#define USE_DRIVERLIB_CRC

#ifdef USE_DRIVERLIB_CRC
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/sw_crc.h>

inline uint16_t crc16_create(const void *buf, size_t len){
    return MAP_Crc16(0, buf, len);
}
#else
uint16_t crc16_create(const void *buf, size_t len);
#endif


#endif
