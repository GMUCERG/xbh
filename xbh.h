#ifndef _XBH_H
#define _XBH_H


#include "xbhconfig.h"
#include "hal/hal.h"

#define TCP_PRIO 2
#define ETH_PRIO 3

#define CYCLES_PER_US (g_syshz/1000000)

uint32_t XBH_handle(const uint8_t *input, uint16_t input_len, uint8_t* output);

#endif
