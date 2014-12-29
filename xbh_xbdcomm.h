#ifndef _XBH_XBDCOMM_H
#define _XBH_XBDCOMM_H

enum XBD_COMM_PROT{COMMI2C, COMM_UART, COMM_UART_OVERDRIVE, COMM_ETHERNET};

#define CE_IDLE			0
#define CE_ACK_WAIT		1
#define CE_FAILURE		2
#define CE_ANSWER_WAIT	3
#define CE_ANSWER_RECD	4

#define CE_MAGIC_SEQNUM	0

#define CE_TIMEOUT_SECS 2
#define CE_MAX_TIMEOUTS	10

extern volatile unsigned long time;

void xbdCommInit(uint8_t commMode);
void xbdCommExit();
void xbdSend(uint8_t length, uint8_t *buf);
void xbdReceive(uint8_t length, uint8_t *buf);


#endif
