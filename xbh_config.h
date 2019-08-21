#ifndef _XBHCONFIG_H
#define _XBHCONFIG_H

#ifndef XBH_HOSTNAME
#define XBH_HOSTNAME "xbh"
#endif


#ifndef TCP_XBH_PORT
#define TCP_XBH_PORT 22595
#endif

//#define DEBUG_XBHNET

#define XBH_IP4_STATIC 1
//#define XBH_IP4_ADDR 0xC0A80063  //192.168.0.99
#define XBH_IP4_ADDR 0xC0A80A60    // 192.168.10.96
//#define XBH_IP4_ADDR 0x0A2A000C    // 10.42.0.12
#define XBH_IP4_NETMASK 0xFFFFFF00
//#define XBH_IP4_NETMASK 0xFF000000
#define XBH_IP4_GW 0
#endif
