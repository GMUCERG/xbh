#ifndef _XBH_H
#define _XBH_H
#define XBH_DEBUG LWIP_DBG_OFF
#define HTONS32(x) ((x & 0xFF000000)>>24)+((x & 0x00FF0000)>>8)+((x & 0x0000FF00)<<8)+((x & 0x000000FF)<<24)
#define CYCLES_PER_US ((F_CPU+500000)/1000000)
#endif
