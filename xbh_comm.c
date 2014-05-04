/**
 * XBH communications service
 *
 * Loosely based off of httpd.c from lwip
 */
#include "lwip/opt.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"

#include <string.h>
#include <stdlib.h>

#define XBH_COMM_PORT 22505
#define XBH_COMM_TCP_PRIO TCP_PRIO_MIN
#define XBH_COMM_ADDR IP_ADDR_ANY

#define XBH_COMM_DEBUG LWIP_DBG_OFF

struct xbh_comm_state{
}

static struct xbh_comm_state* xbh_comm_state_alloc(void){
    struct xbh_comm_state *ret;
    ret = (struct http_state *)mem_malloc(sizeof(struct xbh_comm_state));
    return ret;
}

void xbh_comm_init(){
    struct tcp_pcb *pcb;
    err_t err;

    pcb = tcp_new();
    LWIP_ASSERT("xbh_comm_init_addr: tcp_new failed", pcb != NULL);
    tcp_setprio(pcb, XBH_COMM_TCP_PRIO);

    err = tcp_bind(pcb, XBH_COMM_ADDR, XBH_COMM_PORT);
    LWIP_ASSERT("xbh_comm_init_addr: tcp_bind failed", err == ERR_OK);
    pcb = tcp_listen(pcb);
    LWIP_ASSERT("xbh_comm_init_addr: tcp_listen failed", pcb != NULL);
    /* initialize callback arg and accept callback */
    tcp_arg(pcb, pcb);
    tcp_accept(pcb, xbh_comm_accept);
}


static err_t xbh_comm_accept(void *arg, struct tcp_pcb *pcb, err_t err){
    struct tcp_pcb_listen *lpcb = (struct tcp_pcb_listen*)arg;
    struct xbh_comm_state *state;
    LWIP_UNUSED_ARG(err);
    /* Decrease the listen backlog counter */
    tcp_accepted(lpcb);
    /* Set priority */
    tcp_setprio(pcb, XBH_COMM_TCP_PRIO);

    state = xbh_comm_state_alloc();
    if (state == NULL) {
        LWIP_DEBUGF(XBH_COMM_DEBUG, ("xbh_comm_accept: Out of memory, RST\n"));
        return ERR_MEM;
    }

  /* Tell TCP that this is the structure we wish to be passed for our
     callbacks. */
  tcp_arg(pcb, state);

  /* Set up the various callback functions */
  tcp_recv(pcb, http_recv);
  tcp_err(pcb, http_err);
  tcp_poll(pcb, http_poll, HTTPD_POLL_INTERVAL);
  tcp_sent(pcb, http_sent);
}


static err_t xbh_comm_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
}

static err_t xbh_comm_poll(void *arg, struct tcp_pcb *pcb) {
}

static void xbh_comm_err(void *arg, err_t err) {
}
