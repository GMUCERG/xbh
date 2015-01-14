#ifndef _XBD_MULTIPACKET_H
#define _XBD_MULTIPACKET_H

#include <stdbool.h>
#include <inttypes.h>
#include "xbh.h"
#include "xbh_prot.h"
#include "xbh_xbdcomm.h"


struct xbd_multipkt_state{
#if 0
uint32_t xbd_genmp_datanext;
uint32_t xbd_genmp_dataleft;
uint32_t xbd_genmp_seqn;
#endif

uint32_t recmp_addr;
uint32_t recmp_datanext;
uint32_t recmp_dataleft;
uint32_t recmp_seqn;
uint32_t recmp_type;
};


#if 0
uint32_t XBD_genSucessiveMultiPacket(struct xbd_multipkt_state *state, const uint8_t* srcdata, uint8_t* dstbuf, uint32_t dstlenmax, const uint8_t *code);

uint32_t XBD_genInitialMultiPacket(struct xbd_multipkt_state *state, const uint8_t* srcdata, uint32_t srclen, uint8_t* dstbuf,const uint8_t *code, uint32_t type, uint32_t addr);
#endif

uint8_t XBD_recSucessiveMultiPacket(struct xbd_multipkt_state *state, const uint8_t* recdata, uint32_t reclen, uint8_t* dstbuf, uint32_t dstlenmax, const char *code);

uint8_t XBD_recInitialMultiPacket(struct xbd_multipkt_state *state, const uint8_t* recdata, uint32_t reclen, const char *code, bool hastype, bool hasaddr);

#endif /* _XBD_MULTIPACKET_H */
