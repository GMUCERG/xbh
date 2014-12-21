#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "FreeRTOSConfig.h"
#include <FreeRTOS.h>
#include <task.h>

#include <lwip/sockets.h> 

#include "util.h"
#include "xbhconfig.h"
#include "xbd_multipacket.h"


#define XBH_STACK 512
#define CMDLEN_SZ 4

typedef enum {XBHSRV_ACCEPT, XBHSRV_CMD } xbhsrv_state;
    
/**
 * Task handle for XBH process
 */
static TaskHandle_t xbhsrv_task_handle;

static void xbhsrv_task(void *arg);

/**
 * Starts XBHServer task
 */
void start_xbhserver(void){/*{{{*/
    uint32_t retval;

    DEBUG_OUT("Starting XBH task\n");
    retval = xTaskCreate( xbhsrv_task,
            "xbh",
            XBH_STACK,
            NULL,
            tskIDLE_PRIORITY+1,
            &xbhsrv_task_handle);
    LOOP_ERRMSG(retval != pdPASS, "Could not create xbh server task\n");
}/*}}}*/


// Define these outside XBH thread so they sit outside stack space
uint8_t xbd_cmd[XBD_PACKET_SIZE_MAX];
uint8_t reply_buf[XBD_ANSWERLENG_MAX];
/**
 * XBH thread
 */
static void xbhsrv_task(void *arg){/*{{{*/
    struct sockaddr_in listen_addr;
    struct sockaddr_in clnt_addr;

    int32_t srv_sock, clnt_sock;

    socklen_t clnt_addr_sz;
    int32_t retval;

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
                fcntl(clnt_sock, F_SETFL, 0); // Set blocking
                state = XBHSRV_CMD;
                break;
            case XBHSRV_CMD: 
                {
                    size_t len = 0;
                    retval = recv_bytes(clnt_sock, xbd_cmd, CMDLEN_SZ,0);
                    if(retval <= 0){ goto cmd_err; }

                    // Get length of command message in ascii hex format
                    for(size_t i = 0; i < CMDLEN_SZ; i++){
                        len += htoi(xbd_cmd[i]) << 4*(CMDLEN_SZ-1-i);
                    }

                    //+1 to account for colon delimiter
                    retval = recv_bytes(clnt_sock, xbd_cmd+CMDLEN_SZ, len+1,0);
                    if(retval <= 0){ goto cmd_err; }

                    //Validate if command, otherwise abort
                    if(0 != memcmp(xbd_cmd+CMDLEN_SZ,":XBH",4)){
                        goto cmd_err;
                    }else{
#ifdef DEBUG_XBHNET
                    char cmd[XBH_COMMAND_LEN+1];
                    cmd[XBH_COMMAND_LEN]='\0';
                    memcpy(cmd, xbd_cmd+CMDLEN_SZ+1, XBH_COMMAND_LEN);
                    DEBUG_OUT("Command: %s\n", cmd);
                    DEBUG_OUT("Command Length: %d\n", len);
#endif
                    }
                    //XBH_handle(xbd_cmd, clnt_sock);
cmd_err:
                    close(clnt_sock);
                    state = XBHSRV_ACCEPT;
                }
                break;
        }

#if DEBUG_STACK
    DEBUG_OUT("Stack Usage: %s: %d\n", __PRETTY_FUNCTION__, uxTaskGetStackHighWaterMark(NULL));
#endif
    }
}/*}}}*/
