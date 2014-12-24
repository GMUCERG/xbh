#ifndef _TASK_PRIO_H
#define _TASK_PRIO_H

/* More critical priorities are higher */

#define XBH_SRV_PRIO tskIDLE_PRIORITY+1

#define ETH_PRIO tskIDLE_PRIORITY+2 //Ethernet receive priority
#define LWIP_PRIO tskIDLE_PRIORITY+1 //LWIP Priority

#endif
