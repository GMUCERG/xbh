#ifndef _XBH_XBDCOMM_H
#define _XBH_XBDCOMM_H

enum XBD_COMM_PROT{COMM_I2C, COMM_UART, COMM_UART_OVERDRIVE, COMM_ETHERNET};

#define ADDRSIZE sizeof(uint32_t) //bytes used to store address 
#define LENGSIZE sizeof(uint32_t) //bytes used to store length
#define SEQNSIZE sizeof(uint32_t) //bytes used to store sequence counter
#define TYPESIZE sizeof(uint32_t)
#define TIMESIZE sizeof(uint64_t)
#define NUMBSIZE sizeof(uint32_t)
#define REVISIZE 40	//GIT revisions are 40 digit hex numbers

#define XBD_ANSWERLENG_MAX 32
#define XBD_PACKET_SIZE_MAX 255

#define CRC16SIZE 2

#define XBD_RESULTLEN_EBASH (XBD_COMMAND_LEN+1+2+CRC16SIZE)

void xbdCommInit(uint8_t commMode);
void xbdCommExit(void);
void xbdSend(const void *buf, size_t len);
void xbdReceive(void *buf, size_t len);


#endif
