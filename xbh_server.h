#ifndef _XBHSERVER_H
#define _XBHSERVER_H

#include "hal/measure.h"

// ANSWERLENG MAX must fit entire length of buffers in XBD at once
#define XBH_ANSWERLENG_MAX 3000
#define XBH_PACKET_SIZE_MAX 1500
#define XBH_WAITBUF_MAX 32

#define XBH_HNDLR_DONE 0
#define XBH_HNDLR_PWR_MSG 1

#define CMDLEN_SZ 4


struct xbh_hndlr_to_srv_msg{
    uint8_t *reply_buf;
    size_t len;
//    struct pwr_sample sample;
    uint8_t type: 2;
};
struct xbh_srv_to_hndlr_msg{
    uint8_t *cmd_buf;
    size_t len;
};


void start_xbhserver();

#endif
