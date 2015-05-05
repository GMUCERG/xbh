#include <inttypes.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "FreeRTOSConfig.h"
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <lwip/sockets.h> 

#include "hal/hal.h"
#include "hal/measure.h"
#include "xbh.h"
#include "xbh_prot.h"
#include "util.h"
#include "xbh_config.h"
#include "xbd_multipacket.h"


#define XBH_SRV_STACK 512
#define XBH_HNDLR_STACK 512

typedef enum {XBHSRV_ACCEPT, XBHSRV_CMD, XBHSRV_WAIT} xbhsrv_state;
    
/**
 * Task handle for XBH server process
 */
static TaskHandle_t xbh_srv_task_handle;

/**
 * Task handle for XBH handler process (handling work w/ XBD)
 */
static TaskHandle_t xbh_hndlr_task_handle;
/**
 * Queue for messages passed to xbh server
 */
QueueHandle_t xbh_hndlr_to_srv_q_handle;
/**
 * Queue for messages passed to xbh handler 
 */
static QueueHandle_t xbh_srv_to_hndlr_q_handle;

static void xbh_srv_task(void *arg);
static void xbh_hndlr_task(void *arg);

// Define these outside XBH thread so they sit outside stack space
uint8_t xbh_cmd[XBH_PACKET_SIZE_MAX];
uint8_t reply_buf[XBH_ANSWERLENG_MAX];

//Buffer to hold commands received while waiting
uint8_t wait_buf[XBH_WAITBUF_MAX];


/**
 * Starts XBHServer task
 */
void start_xbhserver(void){/*{{{*/
    int retval;

    xbh_hndlr_to_srv_q_handle = xQueueCreate(15, sizeof(struct xbh_hndlr_to_srv_msg));
    xbh_srv_to_hndlr_q_handle = xQueueCreate(1, sizeof(struct xbh_srv_to_hndlr_msg));

    DEBUG_OUT("Starting XBH task\n");
    retval = xTaskCreate( xbh_srv_task,
            "xbh_srv",
            XBH_SRV_STACK,
            NULL,
            XBH_SRV_PRIO,
            &xbh_srv_task_handle);
    LOOP_ERRMSG(retval != pdPASS, "Could not create xbh server task\n");
    retval = xTaskCreate( xbh_hndlr_task,
            "xbh_hndlr",
            XBH_HNDLR_STACK,
            NULL,
            XBH_SRV_PRIO,
            &xbh_hndlr_task_handle);
    LOOP_ERRMSG(retval != pdPASS, "Could not create xbh handler task\n");
}/*}}}*/


/**
 * XBH Handler thread
 */
static void xbh_hndlr_task(void *arg){/*{{{*/
    struct xbh_srv_to_hndlr_msg rx_msg;
    struct xbh_hndlr_to_srv_msg tx_msg;
    while(true){
        xQueueReceive(xbh_srv_to_hndlr_q_handle, &rx_msg, portMAX_DELAY);
        // Reserve CMDLEN_SZ since prepending length
        // later
        tx_msg.len = XBH_handle(rx_msg.cmd_buf,rx_msg.len,reply_buf+CMDLEN_SZ);
        tx_msg.reply_buf = reply_buf;
        tx_msg.type = XBH_HNDLR_DONE;

        xQueueSend(xbh_hndlr_to_srv_q_handle, &tx_msg, portMAX_DELAY);
    }
}/*}}}*/




static ssize_t recv_cmd(int sock, void *buf, size_t maxlen){
    size_t len;
    uint8_t *buffer = (uint8_t *)buf;

    ssize_t retval = recv_waitall(sock, buffer, CMDLEN_SZ,0);
    if(retval <= 0){ return -1; }

    len = hex2len(buffer);

    // If data length greater than XBD_PACKET_SIZE_MAX, then
    // overlow would happen, so error out, close connection, and
    // return to listening 
    if(len > maxlen){
        return -1;
    }

    retval = recv_waitall(sock, buffer, len,0);
    if(retval <= 0){ return -1; }

    //Validate if command, otherwise abort
    if(0 != memcmp(buffer,"XBH",3)){
        return -1;
    }else{
#ifdef DEBUG_XBHNET
        char cmd[XBH_COMMAND_LEN+1];
        cmd[XBH_COMMAND_LEN]='\0';
        memcpy(cmd, buffer, XBH_COMMAND_LEN);
        DEBUG_OUT("Command: %s\n", cmd);
        DEBUG_OUT("Command Length: %d\n", len);
#endif
    }
    return len;
}


/**
 * XBH server thread
 */
static void xbh_srv_task(void *arg){/*{{{*/
    struct sockaddr_in listen_addr;
    struct sockaddr_in clnt_addr;

    int srv_sock = -1, clnt_sock = -1;

    socklen_t clnt_addr_sz;
    int retval;

    xbhsrv_state state = XBHSRV_ACCEPT;

    //Setup socket and listen/*{{{*/
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(TCP_XBH_PORT);
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    srv_sock = socket(PF_INET,SOCK_STREAM,0);
    LOOP_ERRMSG(srv_sock < 0, "Could not create socket\n");
    retval = bind(srv_sock, (struct sockaddr *)&listen_addr, (socklen_t)sizeof(listen_addr));
    LOOP_ERRMSG(retval != 0, "Could not bind socket to address\n");
    retval = listen(srv_sock, 0);
    LOOP_ERRMSG(retval != 0, "Could not listen on socket\n");
    /*}}}*/
    
    while(true){
        //If socket closed, go back to accepting another connection
        if(send(clnt_sock, NULL, 0, 0) < 0){
            state = XBHSRV_ACCEPT;
        }
        switch(state){
            case XBHSRV_ACCEPT:
                clnt_addr_sz = sizeof(clnt_addr);
                clnt_sock = accept(srv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_sz);
                LOOP_ERRMSG(clnt_sock < 0, "Failed to accept connection\n");
                uart_printf("New connection from %hd.%hd.%hd.%hd, port = %d\n",
                        ((uint8_t *)&clnt_addr.sin_addr.s_addr)[0],
                        ((uint8_t *)&clnt_addr.sin_addr.s_addr)[1],
                        ((uint8_t *)&clnt_addr.sin_addr.s_addr)[2],
                        ((uint8_t *)&clnt_addr.sin_addr.s_addr)[3],
                        clnt_addr.sin_port);
                state = XBHSRV_CMD;
                break;
            case XBHSRV_CMD: 
                {

                    ssize_t len = recv_cmd(clnt_sock, xbh_cmd, XBH_PACKET_SIZE_MAX);
                    if (len < 0){
                        goto cmd_err;
                    }
#if 0
                    // Reset if reset cmd received
                    if(!memcmp(xbh_cmd, XBH_CMD[XBH_CMD_rsr], len)){
                        xbh_reset();
                    }
#endif

                    struct xbh_srv_to_hndlr_msg tx_msg;
                    tx_msg.cmd_buf = xbh_cmd;
                    tx_msg.len = len;
                    xQueueSend(xbh_srv_to_hndlr_q_handle, &tx_msg, portMAX_DELAY);
                    portYIELD();
                    state = XBHSRV_WAIT;
                    break;
cmd_err:
                    close(clnt_sock);
                    state = XBHSRV_ACCEPT;
                    break;
                }

            case XBHSRV_WAIT: 
                {
                    static TickType_t last_sent = 0; 
                    // Check for xbh handle completion and issue reply to PC
                    struct xbh_hndlr_to_srv_msg rx_msg;
                    if(xQueueReceive(xbh_hndlr_to_srv_q_handle, &rx_msg, 1) == pdTRUE){
                        if(rx_msg.type == XBH_HNDLR_DONE){
                            size_t len = rx_msg.len;

                            // Prepend length to buffer
                            len2hex(len, reply_buf);
                            len += CMDLEN_SZ;

                            sendall(clnt_sock, reply_buf, len, 0);
                            state = XBHSRV_CMD;
                        }
                        last_sent = xTaskGetTickCount();
                    //If last keepalive greater than 1k ticks ago, send
                    //keepalive
                    }else if (xTaskGetTickCount() - last_sent > 1000){
                        memcpy(wait_buf+CMDLEN_SZ, XBH_CMD[XBH_CMD_kao], XBH_COMMAND_LEN);
                        len2hex(XBH_COMMAND_LEN, wait_buf);
                        int retval = sendall(clnt_sock, wait_buf, XBH_COMMAND_LEN+CMDLEN_SZ, 0);
                        if (retval < 0) xbh_reset();
                        last_sent = xTaskGetTickCount();
                    }

// Unneeded, just reset if connection terminated. Ugly, but should work
// If enabling, add GEN(rsr) to XBH_CMD
#if 0
                    //Check for RST from pc
                    struct timeval tv;
                    fd_set read_fds;
                    int nready = 0;

                    memset(&tv, 0, sizeof(tv));
                    FD_SET(clnt_sock, &read_fds);

                    nready = select(clnt_sock+1, &read_fds, NULL, NULL, &tv);
                    if(nready < 0){
                        goto wait_err;
                    }
                    if(nready == 1){
                        ssize_t len = recv_cmd(clnt_sock, wait_buf, XBH_WAITBUF_MAX);
                        if(len < 0){
                            goto wait_err;
                        }else if(!memcmp(wait_buf, XBH_CMD[XBH_CMD_rsr], len)){
                            xbh_reset();
                        }
                    }
                    break;
wait_err:
                    close(clnt_sock);
                    state = XBHSRV_ACCEPT;
                    break;
#endif
                }
        }

#if DEBUG_STACK
    DEBUG_OUT("Stack Usage: %s: %d\n", __PRETTY_FUNCTION__, uxTaskGetStackHighWaterMark(NULL));
#endif
    }
}/*}}}*/
