#ifndef _XBH_XBDCOMM_H
#define _XBH_XBDCOMM_H

#include "avrlibtypes.h"


#define COMM_I2C 0
#define COMM_UART 1
#define COMM_UART_OVERDRIVE 2
#define COMM_ETHERNET 3

#define CE_IDLE					0
#define CE_ACK_WAIT			1
#define CE_FAILURE			2
#define CE_ANSWER_WAIT	3
#define CE_ANSWER_RECD	4

#define CE_MAGIC_SEQNUM	0

#define CE_TIMEOUT_SECS 2
#define CE_MAX_TIMEOUTS	10

extern volatile unsigned long time;

void xbdCommInit(u08 commMode);
void xbdCommExit();
void xbdSend(u08 length, u08 *buf);
void xbdReceive(u08 length, u08 *buf);


#endif
