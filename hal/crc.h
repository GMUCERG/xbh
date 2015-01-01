#ifndef _CRC_H_
#define _CRC_H_
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include <driverlib/rom.h>
#include <driverlib/rom_map.h>

#include <driverlib/sw_crc.h>

inline uint16_t crc16_create(void *buf, size_t len){
    return MAP_Crc16(0, buf, len);
}

#endif
