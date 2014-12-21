#ifndef _XBH_H
#define _XBH_H


#include "xbh_config.h"
#include "hal/hal.h"

#define CYCLES_PER_US (g_syshz/1000000)

size_t XBH_handle(const uint8_t *input, size_t input_len, uint8_t *output);

#define XBH_DEBUG DEBUG_OUT

#endif
