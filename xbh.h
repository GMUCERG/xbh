#ifndef _XBH_H
#define _XBH_H


#include "xbh_config.h"
#include "hal/hal.h"

#define XBH_DEBUG DEBUG_OUT

#define CYCLES_PER_US (g_syshz/1000000)

size_t XBH_handle(int sock, const uint8_t *input, size_t input_len, uint8_t *reply);


#define ADDRSIZE sizeof(uint32_t) //bytes used to store address 
#define LENGSIZE sizeof(uint32_t) //bytes used to store length
#define SEQNSIZE sizeof(uint32_t) //bytes used to store sequence counter
#define TYPESIZE sizeof(uint32_t)
#define TIMESIZE sizeof(uint32_t)
#define NUMBSIZE sizeof(uint32_t)
#define REVISIZE 40	//GIT revisions are 40 digit hex numbers

#define CRC16SIZE 2


#endif
