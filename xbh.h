#ifndef _XBH_H
#define _XBH_H


#include <stddef.h>
#include <inttypes.h>
#include "xbh_config.h"
#include "hal/hal.h"

#define XBH_DEBUG DEBUG_OUT
#define XBH_ERROR uart_printf

#define CYCLES_PER_US (g_syshz/1000000)

#define XBH_ANSWERLENG_MAX 256
#define XBH_PACKET_SIZE_MAX 1500
size_t XBH_handle(int sock, const uint8_t *input, size_t input_len, uint8_t *reply);




#endif
