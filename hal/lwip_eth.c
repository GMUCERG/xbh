#include <inttypes.h>
#include <stdbool.h>

#include <driverlib/rom.h>

#include <inc/hw_emac.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <driverlib/debug.h>
#include <driverlib/emac.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/rom_map.h>
#include <driverlib/sysctl.h>

#include "FreeRTOSConfig.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>
#include <portable.h>

#include "lwip_eth.h"
#include <lwip/opt.h>
#include <lwip/tcpip.h>
#include <lwip/dhcp.h>
#include <lwip/netif.h>
#include <netif/tivaif.h>

#include "hal/hal.h"
#include "hal/lwip_eth.h"
#include "xbh.h"
#include "util.h"


#define LINK_POLL_MSEC 2000
#define ETH_INT_STACK 256


// Global Variables /*{{{*/
/** Mac address */
uint8_t mac_addr[6];
/*}}}*/

// Static Variables /*{{{*/

/** Handle for ethernet interrupt task */
static TaskHandle_t eth_int_task_handle;

/** Queue handle for status register passed between ISR and task */
static QueueHandle_t eth_int_q_handle;

/** NetIF struct for lwIP */
static struct netif lwip_netif;
/*}}}*/

// Forward Declarations *//*{{{*/
void init_ethernet(void);
void lwIP_eth_isr(void);
static void eth_int_task(void *arg);
static void tcpip_init_cb(void *args);
static void link_status(struct netif *arg);
/*}}}*/


/**
 * Creates task to feed data from interrupt to lwIP, 
 * initializes EMAC0, then initializes lwIP
 */
void init_ethernet(void){/*{{{*/

    { // Enable Ethernet hardware/*{{{*/
        uint32_t user0, user1;
        /**
         * Enable ethernet
         * See page 160 of spmu298a
         */
        MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_EMAC0);
        MAP_SysCtlPeripheralReset(SYSCTL_PERIPH_EMAC0);

        //Enable internal PHY
        MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_EPHY0);
        MAP_SysCtlPeripheralReset(SYSCTL_PERIPH_EPHY0);

        // Set Ethernet LED pinouts
        // See Page 269 of spmu298a.pdf
        MAP_GPIOPinConfigure(GPIO_PF0_EN0LED0);
        MAP_GPIOPinConfigure(GPIO_PF4_EN0LED1);
        GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4);


        // Busy wait until MAC ready
        while(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_EMAC0) == 0);

        //Using builtin PHY, so don't need to call GPIOPinTypeEthernetMII()
        MAP_EMACPHYConfigSet(EMAC0_BASE, EMAC_PHY_TYPE_INTERNAL|EMAC_PHY_INT_MDIX_EN|EMAC_PHY_AN_100B_T_FULL_DUPLEX);


        // Initialize MAC (see spmu363a.pdf)
        // Maybe should optimize burst size
        MAP_EMACInit(EMAC0_BASE, g_syshz, EMAC_BCONFIG_MIXED_BURST|EMAC_BCONFIG_PRIORITY_FIXED, 4, 4, 0);

        // Set options
        //  Tune parameters
        MAP_EMACConfigSet(EMAC0_BASE, (EMAC_CONFIG_FULL_DUPLEX |
                    EMAC_CONFIG_CHECKSUM_OFFLOAD |
                    EMAC_CONFIG_7BYTE_PREAMBLE |
                    EMAC_CONFIG_IF_GAP_96BITS |
                    EMAC_CONFIG_USE_MACADDR0 |
                    EMAC_CONFIG_SA_FROM_DESCRIPTOR |
                    EMAC_CONFIG_100MBPS|
                    EMAC_CONFIG_BO_LIMIT_1024),
                (EMAC_MODE_RX_STORE_FORWARD |
                 EMAC_MODE_TX_STORE_FORWARD |
                 EMAC_MODE_TX_THRESHOLD_64_BYTES |
                 EMAC_MODE_RX_THRESHOLD_64_BYTES), 0);

        //Mac Address saved in user0 and user1 by default
        MAP_FlashUserGet(&user0, &user1);
        mac_addr[0] = user0       & 0xFF; 
        mac_addr[1] = user0 >> 8  & 0xFF; 
        mac_addr[2] = user0 >> 16 & 0xFF; 
        mac_addr[3] = user1       & 0xFF; 
        mac_addr[4] = user1 >> 8  & 0xFF; 
        mac_addr[5] = user1 >> 16 & 0xFF; 

        MAP_EMACAddrSet(EMAC0_BASE, 0, mac_addr );

        //Explicitly Disable PTP
        EMACTimestampDisable(EMAC0_BASE); 
    }/*}}}*/

    // Lower priority of ISR so *FromISR functions can be safely called
    MAP_IntPrioritySet(INT_EMAC0, ETH_ISR_PRIO);

    tcpip_init(tcpip_init_cb, NULL);
}/*}}}*/

/**
 * Callback for tcpip_init()
 */
static void tcpip_init_cb(void *args){/*{{{*/
    ip_addr_t ip_addr;
    ip_addr_t netmask;
    ip_addr_t gw_addr;
    BaseType_t retval;
    eth_int_q_handle = xQueueCreate(1, sizeof(uint32_t));

    retval = xTaskCreate( eth_int_task,
            "eth_int",
            ETH_INT_STACK,
            NULL,
            tskIDLE_PRIORITY+ETH_PRIO,
            &eth_int_task_handle);
    LOOP_ERR(retval != pdPASS);



    ip_addr.addr = 0;
    netmask.addr = 0;
    gw_addr.addr = 0;

    netif_add(&lwip_netif, &ip_addr, &netmask, &gw_addr, NULL, tivaif_init,
            tcpip_input);

    //netif_add sets everything to defaults, so run after netif_add
    netif_set_hostname(&lwip_netif, XBH_HOSTNAME);
    netif_set_status_callback(&lwip_netif, link_status);
    netif_set_default(&lwip_netif);

#if LWIP_IPV6
    //IPv6, enable linklocal addresses and SLAAC
    netif_create_ip6_linklocal_address(&lwip_netif, mac_addr);
    lwip_netif.autoconfig_enabled = 1;
#endif

    dhcp_start(&lwip_netif);

#if DEBUG_STACK
    DEBUG_OUT("Stack Usage: %s: %d\n", __PRETTY_FUNCTION__, uxTaskGetStackHighWaterMark(NULL));
#endif
}/*}}}*/

/**
 * Outputs link status on serial console
 */
static void link_status(struct netif *arg){/*{{{*/
  uart_printf("IP address of interface %c%c set to %hd.%hd.%hd.%hd\n",
    arg->name[0], arg->name[1],
    ip4_addr1_16(&arg->ip_addr),
    ip4_addr2_16(&arg->ip_addr),
    ip4_addr3_16(&arg->ip_addr),
    ip4_addr4_16(&arg->ip_addr));
  uart_printf("Netmask of interface %c%c set to %hd.%hd.%hd.%hd\n",
    arg->name[0], arg->name[1],
    ip4_addr1_16(&arg->netmask),
    ip4_addr2_16(&arg->netmask),
    ip4_addr3_16(&arg->netmask),
    ip4_addr4_16(&arg->netmask));
  uart_printf("Gateway of interface %c%c set to %hd.%hd.%hd.%hd\n",
    arg->name[0], arg->name[1],
    ip4_addr1_16(&arg->gw),
    ip4_addr2_16(&arg->gw),
    ip4_addr3_16(&arg->gw),
    ip4_addr4_16(&arg->gw));
  uart_printf("Hostname of interface %c%c set to %s\n",
    arg->name[0], arg->name[1], arg->hostname);
}/*}}}*/



/**
 * ISR for handling interrupts from Ethernet MAC (ISR 56)
 */
void lwIP_eth_isr(void){/*{{{*/
    uint32_t status;
    BaseType_t wake;

    status = MAP_EMACIntStatus(EMAC0_BASE, true);

    // If PTP implemented like lwiplib:
    // Check if from timer
    //      clear timer interrupt
    // However, we have already run EMACTimestampDisable() so we don't need to
    // worry about MAC timer interrupts
    
    //Clear interrupt
    if(status) {
        MAP_EMACIntClear(EMAC0_BASE, status);
    }

    // Disable ethernet interrupts here, reenable in task, as we don't want
    // ethernet interrupts while already processing ethernet interrupts See
    // tivaif_hwinit() for list of interrupts
    MAP_EMACIntDisable(EMAC0_BASE, 
            (EMAC_INT_RECEIVE | EMAC_INT_TRANSMIT |
             EMAC_INT_TX_STOPPED | EMAC_INT_RX_NO_BUFFER |
             EMAC_INT_RX_STOPPED | EMAC_INT_PHY));


    // Enqueue status to eth_int_task
    // If sending to queue unblocks task, wake should equal true, 
    // This freeRTOS function copies status by value, so status going out of
    // scope after yielding is not a problem. 
    // We could use semaphore instead of queue by making status a global var
    xQueueSendToBackFromISR(eth_int_q_handle, &status,  &wake);

    //Yield to eth_int_task
    portYIELD_FROM_ISR(wake == pdTRUE);
}/*}}}*/

/**
 * Task for feeding packets from ISR to lwIP
 * Loops forever blocking on queue waiting for next status from ISR
 * @param arg Unused
 */
static void eth_int_task(void* arg){/*{{{*/
    uint32_t status;
    while(1){

        //Loop waiting max time between loops until queue item received
        while(xQueueReceive(eth_int_q_handle, &status, portMAX_DELAY)!=pdPASS);

        tivaif_interrupt(&lwip_netif, status);
        // Reenable interrupts disabled in lwIP_eth_isr()
        MAP_EMACIntEnable(EMAC0_BASE, (EMAC_INT_PHY|
                    EMAC_INT_RECEIVE|
                    EMAC_INT_RX_NO_BUFFER|
                    EMAC_INT_RX_STOPPED|
                    EMAC_INT_TRANSMIT|
                    EMAC_INT_TX_STOPPED));

#if DEBUG_STACK
        DEBUG_OUT("Stack Usage: %s: %d\n", __PRETTY_FUNCTION__, uxTaskGetStackHighWaterMark(NULL));
#endif
    }
}/*}}}*/
