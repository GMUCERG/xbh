#ifndef _XBD_MULTIPACKET_H
#define _XBD_MULTIPACKET_H

#include "xbh.h"
#include "xbh_prot.h"
#include <inttypes.h>


#define XBD_ANSWERLENG_MAX 32
#define XBD_PACKET_SIZE_MAX 255


#define XBD_RESULTLEN_EBASH (XBD_COMMAND_LEN+1+2+CRC16SIZE)

uint32_t XBD_genSucessiveMultiPacket(const uint8_t* srcdata, uint8_t* dstbuf, uint32_t dstlenmax, const uint8_t *code);

uint32_t XBD_genInitialMultiPacket(const uint8_t* srcdata, uint32_t srclen, uint8_t* dstbuf,const uint8_t *code, uint32_t type, uint32_t addr);

uint8_t XBD_recSucessiveMultiPacket(const uint8_t* recdata, uint32_t reclen, uint8_t* dstbuf, uint32_t dstlenmax, const uint8_t *code);

uint8_t XBD_recInitialMultiPacket(const uint8_t* recdata, uint32_t reclen, const uint8_t *code, uint8_t hastype, uint8_t hasaddr);

#endif /* _XBD_MULTIPACKET_H */
