#ifndef _XBD_MULTIPACKET_H
#define _XBD_MULTIPACKET_H

#include <stdbool.h>
#include <inttypes.h>
#include "xbh.h"
#include "xbh_prot.h"
#include "xbh_xbdcomm.h"

#define NO_MP_TYPE 0xFFFFFFFF
#define NO_MP_ADDR 0xFFFFFFFF


struct xbd_multipkt_state{
uint32_t addr;
uint32_t datanext;
uint32_t dataleft;
uint32_t seqn;
uint32_t type;
const void *tgt_data;
};

size_t XBD_genInitialMultiPacket(struct xbd_multipkt_state *state, const void *srcdata, size_t srclen, void *dstbuf, const char *cmd_code, uint32_t addr, uint32_t type);

size_t XBD_genSucessiveMultiPacket(struct xbd_multipkt_state *state, void *dstbuf, uint32_t dstlenmax, const char *cmd_code);

int XBD_recInitialMultiPacket(struct xbd_multipkt_state *state, const void *recdata, uint32_t reclen, const char *cmd_code, bool hastype, bool hasaddr);

int XBD_recSucessiveMultiPacket(struct xbd_multipkt_state *state, const void *recdata, uint32_t reclen, void *dstbuf, uint32_t dstlenmax, const char *code);


#endif /* _XBD_MULTIPACKET_H */
