#ifndef __XBH_COMM_H__
#define __XBH_COMM_H__
#include "xbh_prot.h"

#define XBH_COMM_DEBUG LWIP_DBG_OFF
#define XBH_COMM_PORT 22505
#define XBH_COMM_POLL_INTERVAL 4
#define XBH_COMM_MAX_RETRIES   4

#define XBH_COMM_ADDR IP_ADDR_ANY
#define XBH_COMM_TCP_PRIO TCP_PRIO_MIN

void xbh_comm_init();

#endif /* __LWIPOPTS_H__ */

