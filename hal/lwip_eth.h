#ifndef _LWIP_ETH_H
#define _LWIP_ETH_H

#include <inttypes.h>

void init_ethernet(void);

extern uint8_t mac_addr[6];

#endif
