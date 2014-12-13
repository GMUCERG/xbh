#include "FreeRTOSConfig.h"
#include <FreeRTOS.h>
#include <task.h>
#include <stdbool.h>

#include <lwip/sockets.h> 

#define XBH_TASK_STACK 512
/**
 * Task handle for XBH process
 */
static TaskHandle_t xbh_task_handle;

void xbh_task(void *arg);

void start_xbh_task(void){
    DEBUG_OUT("Starting XBH task");
    retval = xTaskCreate( eth_int_task,
            "xbh",
            XBH_STACK,
            NULL,
            tskIDLE_PRIORITY+1,
            &xbh_task_handle);
    LOOP_ERR(retval != pdPASS);
}




void xbh_task(void *arg){
    struct sockaddr_in listen_addr;
    int sock_fd = socket(PF_INET,SOCK_STREAM,0);
    

    while(true){
        
    }
}

getaddrinfo
socket_read
recv

