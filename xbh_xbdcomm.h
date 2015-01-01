#ifndef _XBH_XBDCOMM_H
#define _XBH_XBDCOMM_H

enum XBD_COMM_PROT{COMM_I2C, COMM_UART, COMM_UART_OVERDRIVE, COMM_ETHERNET};

#define CE_IDLE			0
#define CE_ACK_WAIT		1
#define CE_FAILURE		2
#define CE_ANSWER_WAIT	3
#define CE_ANSWER_RECD	4

#define CE_MAGIC_SEQNUM	0

#define CE_TIMEOUT_SECS 2
#define CE_MAX_TIMEOUTS	10

void xbdCommInit(uint8_t commMode);
void xbdCommExit();
void xbdSend(void *buf, size_t len);
void xbdReceive(void *buf, size_t len);


#endif
