#ifndef _XBH_XBDCOMM_H
#define _XBH_XBDCOMM_H

#include "xbh.h"
#include "xbh_prot.h"

enum XBD_COMM_PROT{COMM_I2C, COMM_UART, COMM_UART_OVERDRIVE, COMM_ETHERNET};

#define ADDRSIZE sizeof(uint32_t) //bytes used to store address 
#define LENGSIZE sizeof(uint32_t) //bytes used to store length
#define SEQNSIZE sizeof(uint32_t) //bytes used to store sequence counter
#define TYPESIZE sizeof(uint32_t)
#define TIMESIZE sizeof(uint32_t)
#define NUMBSIZE sizeof(uint32_t)
#define REVISIZE 7	//GIT revisions are 40 digit hex numbers, only use first 7 
#define CRC16SIZE sizeof(uint16_t)

//TODO: XXX //Make sure this matches XBD 
#define XBD_ANSWERLENG_MAX 48
// Matches max i2c packet size set in atmega ports of XBD
#define XBD_PACKET_SIZE_MAX 160
#define XBD_PKT_PAYLOAD_MAX (XBD_PACKET_SIZE_MAX-XBD_COMMAND_LEN-CRC16SIZE)


//#define XBD_RESULTLEN_EBASH (XBD_COMMAND_LEN+1+2+CRC16SIZE)

void xbdCommInit(uint8_t commMode);
void xbdCommExit(void);
void xbdSend(const void *buf, size_t len);
int xbdReceive(void *buf, size_t len);


#endif
