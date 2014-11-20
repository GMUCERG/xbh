#ifndef _XBH_H
#define _XBH_H
#define LWIP_XBH_DEBUG LWIP_DBG_ON
#define CYCLES_PER_US ((F_CPU+500000)/1000000)


uint32_t XBH_handle(const uint8_t *input, uint16_t input_len, uint8_t* output);

#endif
