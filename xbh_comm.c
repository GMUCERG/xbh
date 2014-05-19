/**
 * XBH communications service
 * Copyright (c) 2014 John Pham
 *
 * Loosely based off of httpd.c from lwip which has the following copyright notice:
 *
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */
#include "lwip/opt.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#define XBH_COMM_PORT 22505
#define XBH_COMM_TCP_PRIO TCP_PRIO_MIN
#define XBH_COMM_ADDR IP_ADDR_ANY

#define XBH_COMM_DEBUG LWIP_DBG_OFF

#define XBH_COMM_POLL_INTERVAL 4
#define XBH_COMM_MAX_RETRIES   4

struct xbh_comm_state{
    uint32_t retries;
};


/*
 * LWIP Callbacks
 */
static err_t xbh_comm_accept(void *state, struct tcp_pcb *pcb, err_t err);
static err_t xbh_comm_recv(void *state, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
static err_t xbh_comm_poll(void *state, struct tcp_pcb *pcb);
static err_t xbh_comm_sent(void *state, struct tcp_pcb *pcb, uint16_t sent);

/*
 * LWIP helper functions
 */
static err_t xbh_comm_close_conn(struct xbh_comm_state *state, struct tcp_pcb *pcb);
static void xbh_comm_err(void *arg, err_t err);
static struct xbh_comm_state* xbh_comm_alloc(void);
static void  xbh_comm_free (struct xbh_comm_state* state);

/*
 * XBH functions
 */

static struct xbh_comm_state* xbh_comm_alloc(void){
    struct xbh_comm_state *ret;
    ret = (struct xbh_comm_state *)mem_malloc(sizeof(struct xbh_comm_state));
    return ret;
}

static void  xbh_comm_free (struct xbh_comm_state* state){
    mem_free(state);
}

void xbh_comm_init(){
    struct tcp_pcb *pcb;
    err_t err;

    pcb = tcp_new();

    LWIP_ASSERT("xbh_comm_init_addr: tcp_new failed", pcb != NULL);
    tcp_setprio(pcb, XBH_COMM_TCP_PRIO);

    err = tcp_bind(pcb, IP_ADDR_ANY, XBH_COMM_PORT);
    LWIP_ASSERT("xbh_comm_init_addr: tcp_bind failed", err == ERR_OK);
    pcb = tcp_listen(pcb);
    LWIP_ASSERT("xbh_comm_init_addr: tcp_listen failed", pcb != NULL);
    /* initialize callback arg and accept callback */
    tcp_arg(pcb, pcb);
    tcp_accept(pcb, xbh_comm_accept);
}


static err_t xbh_comm_accept(void *state, struct tcp_pcb *pcb, err_t err){
    struct tcp_pcb *lpcb = (struct tcp_pcb*)state;
    struct xbh_comm_state *xcs = (struct xbh_comm_state*)state;

    LWIP_UNUSED_ARG(err);
    /* Decrease the listen backlog counter */
    tcp_accepted(lpcb);
    /* Set priority */
    tcp_setprio(pcb, XBH_COMM_TCP_PRIO);

    state = xbh_comm_alloc();
    if (state == NULL) {
        LWIP_DEBUGF(XBH_COMM_DEBUG, ("xbh_comm_accept: Out of memory, RST\n"));
        return ERR_MEM;
    }

    /* Tell TCP that this is the structure we wish to be passed for our
       callbacks. */
    tcp_arg(pcb, xcs);

    /* Set up the various callback functions */
    tcp_recv(pcb, xbh_comm_recv);
    tcp_err(pcb, xbh_comm_err);
    tcp_poll(pcb, xbh_comm_poll, XBH_COMM_POLL_INTERVAL);
    tcp_sent(pcb, xbh_comm_sent);

    return ERR_OK;
}


static err_t xbh_comm_recv(void *state, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    struct xbh_comm_state *xcs = (struct xbh_comm_state*)state;

    if ((err != ERR_OK) || (p == NULL) || (xcs == NULL)) {
        /* error or closed by other side? */
        if (p != NULL) {
            /* Inform TCP that we have taken the data. */
            tcp_recved(pcb, p->tot_len);
            pbuf_free(p);
        }
        if (xcs == NULL) {
            /* this should not happen, only to be robust */
            LWIP_DEBUGF(XBH_COMM_DEBUG, ("Error, xbh_comm_recv: xcs is NULL, close\n"));
        }
        xbh_comm_close_conn(xcs, pcb);
        return ERR_OK;
    }
    return ERR_OK;
}

static err_t xbh_comm_close_conn(struct xbh_comm_state *state, struct tcp_pcb *pcb){
    err_t err;
    LWIP_DEBUGF(XBH_COMM_DEBUG, ("Closing connection %p\n", (void*)pcb));

    tcp_arg(pcb, NULL);
    tcp_recv(pcb, NULL);
    tcp_err(pcb, NULL);
    tcp_poll(pcb, NULL, 0);
    tcp_sent(pcb, NULL);
    if(state != NULL) {
        xbh_comm_free(state);
    }

    err = tcp_close(pcb);
    if (err != ERR_OK) {
        LWIP_DEBUGF(XBH_COMM_DEBUG, ("Error %d closing %p\n", err, (void*)pcb));
        /* error closing, try again later in poll */
        tcp_poll(pcb, xbh_comm_poll, XBH_COMM_POLL_INTERVAL);
    }
    return err;
}

static err_t xbh_comm_poll(void *state, struct tcp_pcb *pcb) {
    struct xbh_comm_state *xcs = (struct xbh_comm_state*)state;
    LWIP_DEBUGF(XBH_COMM_DEBUG | LWIP_DBG_TRACE, ("xbh_comm_poll: pcb=%p hs=%p pcb_state=%s\n",
                (void*)pcb, (void*)xcs, tcp_debug_state_str(pcb->state)));

    if (xcs== NULL) {
        err_t closed;
        /* arg is null, close. */
        LWIP_DEBUGF(XBH_COMM_DEBUG, ("xbh_comm_poll: arg is NULL, close\n"));
        closed = xbh_comm_close_conn(xcs, pcb);
        LWIP_UNUSED_ARG(closed);
        if (closed == ERR_MEM) {
            LWIP_DEBUGF(XBH_COMM_DEBUG, ("xbh_comm_poll: mem error, aborted\n"));
            tcp_abort(pcb);
            return ERR_ABRT;
        }
        return ERR_OK;
    } else {
        xcs->retries++;
        if (xcs->retries == XBH_COMM_MAX_RETRIES) {
            LWIP_DEBUGF(XBH_COMM_DEBUG, ("xbh_comm_poll: too many retries, close\n"));
            xbh_comm_close_conn(xcs, pcb);
            return ERR_OK;
        }

        ///* If this connection has a file open, try to send some more data. If
        // * it has not yet received a GET request, don't do this since it will
        // * cause the connection to close immediately. */
        //if(xcs && (xcs->handle)) {
        //    LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("http_poll: try to send more data\n"));
        //    if(http_send_data(pcb, xcs)) {
        //        /* If we wrote anything to be sent, go ahead and send it now. */
        //        LWIP_DEBUGF(HTTPD_DEBUG | LWIP_DBG_TRACE, ("tcp_output\n"));
        //        tcp_output(pcb);
        //    }
        //}
    }

    return ERR_OK;
}

static err_t xbh_comm_sent(void *state, struct tcp_pcb *pcb, uint16_t sent) {
    //send more stuff
    return -1;
}

static void xbh_comm_err(void *state, err_t err) {
    struct xbh_comm_state *xcs = (struct xbh_comm_state *)state;
    LWIP_UNUSED_ARG(err);

    LWIP_DEBUGF(XBH_COMMD_DEBUG, ("xbh_comm_err: %s", lwip_strerr(err)));

    if (xcs != NULL) {
        xbh_comm_free(xcs);
    }
}
