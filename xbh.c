/**
 *
 * @file
 * XBH server firmware for connected TIVA-C w/ LWIP
 * (C) 2014, John Pham, George Mason University <jpham4@gmu.edu>
 * Based on XBH server firmware
 * (C) 2012, XBX Project <xbx@xbx.das-labor.org>
 * Based on Webserver uvm 
 * (C) 2007 Radi Ulrich <mail@ulrichradig.de>
`*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <inttypes.h>
#include <sys/types.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOSConfig.h"
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>


#include "hal/hal.h"
#include "hal/lwip_eth.h"
#include "hal/measure.h"
#include "xbh.h"
#include "xbh_config.h"
#include "xbh_prot.h"
#include "xbh_xbdcomm.h"
#include "xbd_multipacket.h"

#include "util.h"

#define XBH_REV_DIGITS REVISIZE //could be all less than 40 digits, but just return all


/**
 * XBH Command String Constants
 */
const char *XBH_CMD[] = {
     FOREACH_XBH_CMD(XBH_CMD_DEF)
};

/**
 * XBD Command String Constants */
const char *XBD_CMD[] = {
     FOREACH_XBD_CMD(XBD_CMD_DEF)
};

static alignas(sizeof(uint32_t)) uint8_t XBDCommandBuf[XBD_PACKET_SIZE_MAX];
static alignas(sizeof(uint32_t)) uint8_t XBDResponseBuf[XBD_ANSWERLENG_MAX];



/**
 * Retrieves git revision information and mac address
 * @param p_answer Buffer to write information to. Needs to be
 * XBH_REV_DIGITS+1+6*2+1 long. Format is gitrev,mac address (no colons)
 * @return length of data written to p_answer
 */
size_t XBH_HandleRevisionRequest(uint8_t* p_answer){/*{{{*/
    uint8_t i;


    // Report Git Rev in 7 digits length

    for(i=0;i < XBH_REV_DIGITS;i++) {
        p_answer[i]=XBH_REVISION[i];
    }

    p_answer[XBH_REV_DIGITS]=',';

    for(i=0;i<6;++i) {
        p_answer[XBH_REV_DIGITS+1+2*i]=ntoa(mac_addr[i]>>4);
        p_answer[XBH_REV_DIGITS+1+2*i+1]=ntoa(mac_addr[i]&0xf);
    }
    // Add Null terminator
    p_answer[XBH_REV_DIGITS+1+2*6]=0;
    return XBH_REV_DIGITS+1+2*6+1;
}/*}}}*/

/**
 * Asks XBD to execute
 * @return 0 if success, 1 if fail.
 */
static int XBH_HandleEXecutionRequest(void) {/*{{{*/
    int retval;
    // 4 bytes pkt ID + sizeof pwr_sample in ascii hex. Subtract 2 to remove padding
//    uint8_t pkt_buf[4+2*(sizeof(struct pwr_sample)-2)]; 
    
//    memcpy(pkt_buf, "PWRD", 4);


    //Kick off power measurement
    //measure_start();
    //exec_timer_start();

    // Send execution request to XBD
    XBH_DEBUG("Sending 'ex'ecution  'r'equest to XBD\n");
    exec_timer_start();
    memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_exr], XBD_COMMAND_LEN);
    xbdSend(XBDCommandBuf, XBD_COMMAND_LEN);


#if 0
    struct pwr_sample sample; 
    //TODO Use message passing instead of socket
    // Run power measurement/*{{{*/
    while(measure_isrunning()){
        // Dequeue data packets, waiting for a max of 2 ticks
        if(pdTRUE == xQueueReceive(pwr_sample_q_handle, &sample, 2)){
            // Convert to ascii hex to be consistent with rest of xbh
            // Subtract 2 since we don't want to send padding- the struct is 8-byte
            // aligned and has 2 bytes padding
            for(size_t i = 0; i < sizeof(struct pwr_sample)-2; i++){
                pkt_buf[i*2+4] = ntoa(((uint8_t *)&sample)[i] >> 4);
                pkt_buf[i*2+1+4] = ntoa(((uint8_t *)&sample)[i] & 0xF);
            }
            retval = send(sock, pkt_buf, sizeof(pkt_buf), 0);
            // If socket disconnected, fail
            if(retval < 0){
                uart_printf("Failed to send pwr data to PC\n");
                return 1;
            }
        }
    }/*}}}*/
#endif

// COMMENTS FOR JOHN
// make send and receive two independent functions
// add function to querry timer flag
// call receive only when timer flag high
// all functions need xbd id (0-3)
// verify watchdog timer setting now that execution is non blocking


    // Receive status from XBD
    retval = xbdReceive(XBDResponseBuf, XBD_COMMAND_LEN);
    
    if(0 == memcmp(XBDResponseBuf,XBD_CMD[XBD_CMD_exo], XBD_COMMAND_LEN) && 0 == retval) {  
        XBH_DEBUG("Received 'ex'ecution 'o'kay from XBD\n");
/*      XBH_DEBUG("Took %d s, %d IRQs, %d clocks\r\n",
                        risingTimeStamp-fallingTimeStamp,
                        risingIntCtr-fallingIntCtr,
                        risingTime-fallingTime);
*/      return 0;
    } else {
        XBH_DEBUG("Did not receive 'ex'ecution 'o'kay from XBD\n");
        return 1;
    }
}/*}}}*/

/**
 * Program XBD
 * @param input_buf Code buffer containing code in ascii hex format, prefixed by
 * 4 byte address
 * @param len Length of buffer in bytes (in hex format)
 * @return 0 if success, 1 if fails during program flash request to XBD, 2 if
 * fails during flash download request to XBD
 */
static int XBH_HandleCodeDownloadRequest(const uint8_t *input_buf, uint32_t len) {/*{{{*/
    const uint32_t addr = ntohl(*(uint32_t*)input_buf);
    struct xbd_multipkt_state state;
    size_t size;
    int retval;


    size = XBD_genInitialMultiPacket(&state, input_buf+ADDRSIZE, len-ADDRSIZE, XBDCommandBuf,
            XBD_CMD[XBD_CMD_pfr], addr, NO_MP_TYPE);

    XBH_DEBUG( "Sending 'p'rogram 'f'lash 'r'equest to XBD\n");
    xbdSend(XBDCommandBuf, size);
    retval = xbdReceive(XBDResponseBuf, XBD_COMMAND_LEN);
    
    if(0 != memcmp(XBD_CMD[XBD_CMD_pfo],XBDResponseBuf,XBH_COMMAND_LEN) || 0 !=retval) {
        XBH_DEBUG("Error: Did not receive 'p'rogram 'f'lash 'o'kay from XBD!\n");
        return 1;
    }else{
        XBH_DEBUG("Received 'p'rogram 'f'lash 'o'kay from XBD\n");
    }

    while(state.dataleft != 0) {
        size = XBD_genSucessiveMultiPacket(&state, XBDCommandBuf, XBD_PKT_PAYLOAD_MAX,  XBD_CMD[XBD_CMD_fdr]);
        XBH_DEBUG("Sending 'f'lash 'd'ownload 'r'equest to XBD\n");
        xbdSend(XBDCommandBuf, size);
        retval = xbdReceive(XBDResponseBuf, XBD_COMMAND_LEN);
                
        if(0 == memcmp(XBD_CMD[XBD_CMD_pfo],XBDResponseBuf,XBH_COMMAND_LEN) && 0 == retval) {
            XBH_DEBUG("Received 'p'rogram 'f'lash 'o'kay from XBD\n");
        } else {
            XBH_DEBUG("Error: Did not receive 'p'rogram 'f'lash 'o'kay from XBD\n");
            return 2;
        }   
    }
    return 0;

}/*}}}*/

/**
 * Returns number of cycles busy loop has actually waited inside the XBD in
 * p_answer. Appears to be unused
 * @param p_answer Buffer to write XBDtco command plus 8 hex digits (XBD_COMMAND_LEN+8)
 * @return 0 if success, else fail
 */
static int XBH_HandleTimingCalibrationRequest(uint8_t* p_answer) {/*{{{*/
    int retval;

    memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_tcr], XBD_COMMAND_LEN);
    memset(XBDResponseBuf,'-',XBD_COMMAND_LEN+NUMBSIZE);

    exec_timer_start();
    xbdSend(XBDCommandBuf, XBD_COMMAND_LEN);
    retval = xbdReceive(XBDResponseBuf, XBD_COMMAND_LEN+NUMBSIZE);

    XBH_DEBUG("tcr\n");

//  XBH_DEBUG("Answer: [%s]",((uint8_t*)XBDResponseBuf));

    if(0 == memcmp(XBDResponseBuf,XBD_CMD[XBD_CMD_tco],XBD_COMMAND_LEN) && 0 == retval ) {  
        memcpy(p_answer, XBDResponseBuf+XBD_COMMAND_LEN, TIMESIZE);
        return 0;
    } else {
        XBH_ERROR("Response wrong [%s\r\n",XBDResponseBuf);
        return 1;
    }

}/*}}}*/

/**
 * Returns git revision of XBD code in p_answer
 * @param p_answer Buffer to return XBD response command and git revision
 * @returnr 0 if success, 1 if failure
 */
int XBH_HandleTargetRevisionRequest(uint8_t* p_answer) {/*{{{*/
    int retval;
    memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_trr], XBD_COMMAND_LEN);
    memset(XBDResponseBuf,' ',XBD_COMMAND_LEN+REVISIZE);
    xbdSend(XBDCommandBuf, XBD_COMMAND_LEN);
    retval = xbdReceive(XBDResponseBuf, XBD_COMMAND_LEN+REVISIZE);

//  XBH_DEBUG("Answer: [%s]",((uint8_t*)XBDResponseBuf));

    if(0 == memcmp(XBDResponseBuf,XBD_CMD[XBD_CMD_tro],XBD_COMMAND_LEN) && 0 == retval) {   
        memcpy(p_answer, XBDResponseBuf+XBD_COMMAND_LEN, REVISIZE);
        return 0;
    } else {
        return 1;
    }

}/*}}}*/


/**
 * Reports Power
 * @param p_answer Array to be filled w/ w/ hex in big endian of format 
 * pppppppp where p is power.
 */
void XBH_HandlePowerRequest(uint8_t* p_answer) {
     uint32_t avgPwr = measure_get_avg();
     uint32_t maxPwr = measure_get_max();
     uint32_t  gain = measure_get_gain();
     uint32_t  cntover = measure_get_cntover();
     
     avgPwr = avgPwr >> 20;
     DEBUG_OUT("Average amplified shunt voltage: %d\n", avgPwr);
     DEBUG_OUT("Maximum amplified shunt voltage: %d\n", maxPwr);
     DEBUG_OUT("Gain set on XBP: %d\n", gain);
     if(cntover) {
         DEBUG_OUT("Counter for Average computation had a overflow\n");
     }
     avgPwr = htonl(avgPwr);
     maxPwr = htonl(maxPwr);
     cntover = htonl(cntover);
     
     memcpy(p_answer, &avgPwr, TIMESIZE);
     memcpy(p_answer+TIMESIZE, &maxPwr, TIMESIZE);
     memcpy(p_answer+2*TIMESIZE, &cntover, TIMESIZE);
}

/**
 * Set XBP Gain
 * @param input_buf Buffer containing type code in ascii hex format 
 * @param len Length of buffer in bytes (in hex format)
 */
static void XBH_HandlePowerGainRequest(const uint8_t *input_buf, uint32_t len) {/*{{{*/
    const uint32_t gain = ntohl(*(uint32_t*)input_buf);
    
     DEBUG_OUT("Setting Gain on XBP of: %d\n", gain);

    // add code to set IOs
}

/**
 * Reports time taken of last operation
 * @param p_answer Array to be filled w/ w/ hex in big endian of format 
 */
void XBH_HandleRePorttimestampRequest(uint8_t* p_answer)    {/*{{{*/
    uint64_t start = measure_get_start();
    uint64_t stop = measure_get_stop();
    uint64_t time = stop - start;
    uint32_t quot = time / g_syshz;
    uint32_t rem = time % g_syshz;
    uint32_t syshz = htonl(g_syshz);

    quot = htonl(quot);
    rem = htonl(rem);
    //lldiv_t result = lldiv(time, g_syshz);

    //Format is seconds , timestamp % xbh_clk, xbh_clk

    memcpy(p_answer, &quot, TIMESIZE);
    memcpy(p_answer+TIMESIZE, &rem, TIMESIZE);
    memcpy(p_answer+2*TIMESIZE, &syshz, TIMESIZE);

}/*}}}*/

/**
 * Set XBD communication protocol
 * @param requestedComm Requested communication protocol. 'U' for UART, 'O' for
 * overdriven UART, 'I' for I2C, 'E' for ethernet.
 * @return 0 if successful, 2 if invalid
 */
static int XBH_HandleSetCommunicationRequest(const uint8_t requestedComm) {/*{{{*/
    switch(requestedComm) {
        case 'U':
            xbdCommExit();
            xbdCommInit(COMM_UART);
            break;
        case 'O':
            xbdCommExit();
            xbdCommInit(COMM_UART_OVERDRIVE);
            break;
        case 'I':
            xbdCommExit();
            xbdCommInit(COMM_I2C);
            break;
        case 'E':
            xbdCommExit();
            xbdCommInit(COMM_ETHERNET);
            break;
        default:
            XBH_DEBUG("Invalid set communication request %c\r\n",requestedComm);
            return 2;
            break;
    }
    return(0);
}/*}}}*/

/**
 * Download parameters to XBD
 * @param input_buf Buffer containing type code in ascii hex format 
 * @param len Length of buffer in bytes (in hex format)
 * @return 0 if success, 1 if fails during program parameter request to XBD, 2 if
 * fails during program download request to XBD
 */
static int XBH_HandleDownloadParametersRequest(const uint8_t *input_buf, uint32_t len) {/*{{{*/
    const uint32_t addr = ntohl(*(uint32_t*)input_buf);
    const uint32_t type = ntohl(*(uint32_t*)(input_buf+ADDRSIZE));
    struct xbd_multipkt_state state;
    size_t size;
    int retval;

    size = XBD_genInitialMultiPacket(&state, input_buf+ADDRSIZE+TYPESIZE,
            len-ADDRSIZE-TYPESIZE, XBDCommandBuf,
            XBD_CMD[XBD_CMD_ppr], addr, type);

    XBH_DEBUG("Sending 'p'rogram 'p'arameters 'r'equest to XBD\n");
    xbdSend(XBDCommandBuf, size);
    retval = xbdReceive(XBDResponseBuf, XBD_COMMAND_LEN);

    if(0 != memcmp(XBD_CMD[XBD_CMD_ppo],XBDResponseBuf,XBH_COMMAND_LEN) || retval != 0) {
        XBH_DEBUG("Did not receive 'p'rogram 'p'arameters 'o'kay from XBD\n");
        return 1;
    }
    XBH_DEBUG("Received 'p'rogram 'p'arameters 'o'kay from XBD\n");


    while(state.dataleft != 0) {
        size = XBD_genSucessiveMultiPacket(&state, XBDCommandBuf, XBD_PKT_PAYLOAD_MAX,  XBD_CMD[XBD_CMD_pdr]);

        XBH_DEBUG("Sending 'p'rogram 'd'ownload 'r'equest to XBD\n");
        xbdSend(XBDCommandBuf, size);
        retval = xbdReceive(XBDResponseBuf, XBD_COMMAND_LEN);

        if(0 == memcmp(XBD_CMD[XBD_CMD_pdo],XBDResponseBuf,XBH_COMMAND_LEN) && 0 == retval) {
            XBH_DEBUG("Received 'p'rogram 'd'ownload 'o'kay from XBD\n");
        } else {
            XBH_DEBUG("Did not recieve 'p'rogram 'd'ownload 'o'kay from XBD\n");
            return 2;
        }   
    }
    return 0;
}/*}}}*/

/**
 * Retrieve value of computation
 * @param p_answer Buffer to write answer to 
 * @return length of data in p_answer without XBD_COMMAND_LEN. Negative if
 * failure
 */
static ssize_t XBH_HandleUploadResultsRequest(uint8_t* p_answer) {/*{{{*/
    int ret;
    int retval;
    struct xbd_multipkt_state state;
    XBH_DEBUG("Sending 'u'pload 'r'esults 'r'equest to XBD\n");

    memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_urr], XBD_COMMAND_LEN);

    xbdSend(XBDCommandBuf, XBD_COMMAND_LEN);


    retval = xbdReceive(XBDResponseBuf, XBD_ANSWERLENG_MAX-CRC16SIZE);
    
    if(0 == retval && 0 == XBD_recInitialMultiPacket(&state, XBDResponseBuf,XBD_ANSWERLENG_MAX-CRC16SIZE, XBD_CMD[XBD_CMD_uro], true /*hastype*/, false /*hasaddr*/)){
        XBH_DEBUG("Received 'u'pload 'r'esults 'o'kay from XBD\n");
    }else{
        XBH_DEBUG("Error with 'u'pload 'r'esults 'o'kay from XBD\n");
        return (retval); // was -
    }
        


    memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_rdr], XBD_COMMAND_LEN);
    do {
        XBH_DEBUG("Sending 'r'esult 'd'ata 'r'equest to XBD\n");
        xbdSend(XBDCommandBuf, XBD_COMMAND_LEN);
        retval = xbdReceive(XBDResponseBuf, XBD_ANSWERLENG_MAX-CRC16SIZE);
        ret=XBD_recSucessiveMultiPacket(&state, XBDResponseBuf, XBD_ANSWERLENG_MAX-CRC16SIZE, p_answer, XBH_ANSWERLENG_MAX-XBH_COMMAND_LEN, XBD_CMD[XBD_CMD_rdo]);
    } while(state.dataleft != 0 && 0 == ret && 0 == retval); 


    if( 0 == retval && ret==0 && 0 == memcmp(XBDResponseBuf,XBD_CMD[XBD_CMD_rdo],XBD_COMMAND_LEN)) {    
        return state.datanext;
    } else {
        XBH_DEBUG("'r'esult 'd'ata 'r'equest to XBD failed\n");
        return (retval); // was -
    }
}/*}}}*/


/**
 * Asks XBD to calculate checksum
 * @return 0 if success, 1 if fail.
 */
static int XBH_HandleChecksumCalcRequest() {/*{{{*/
    int retval;
    memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_ccr], XBD_COMMAND_LEN);
    XBH_DEBUG("Sending 'c'hecksum 'c'alc 'r'equest to XBD\n");
    exec_timer_start();
    xbdSend(XBDCommandBuf, XBD_COMMAND_LEN);
    retval = xbdReceive(XBDResponseBuf, XBD_COMMAND_LEN);
    

    if(0 == memcmp(XBDResponseBuf,XBD_CMD[XBD_CMD_cco],XBD_COMMAND_LEN) && 0 == retval) {   
        XBH_DEBUG("Received 'c'hecksum 'c'alc 'o'kay from XBD\n");
/*      XBH_DEBUG("Took %d s, %d IRQs, %d clocks\r\n",
                risingTimeStamp-fallingTimeStamp,
                risingIntCtr-fallingIntCtr,
                risingTime-fallingTime);
*/      return 0;
    } else {
        XBH_DEBUG("Did not receive 'c'hecksum 'c'alc 'o'kay from XBD\n");
        return 1;
    }
}/*}}}*/


/**
 * Switches from bootloader to application mode
 * @return 0 if success
 */
static uint8_t XBH_HandleStartApplicationRequest() {/*{{{*/
    uint16_t trys;
    int retval;

    trys=0;
    while(trys<3) {
        XBH_DEBUG("Sending 's'tart 'a'pplication 'r'equest to XBD\n");
        memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_sar], XBD_COMMAND_LEN);
        xbdSend(XBDCommandBuf, XBD_COMMAND_LEN);
        vTaskDelay(500);

        XBH_DEBUG("Sending 'v'ersion 'i'nformation 'r'equest to XBD\n");
        memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_vir], XBD_COMMAND_LEN);
        xbdSend(XBDCommandBuf, XBD_COMMAND_LEN);
        retval = xbdReceive(XBDResponseBuf, XBD_COMMAND_LEN);
    

        if(0 == memcmp(XBDResponseBuf,XBD_CMD[XBD_CMD_AFo],XBD_COMMAND_LEN) && 0 == retval) {   
            XBH_DEBUG("Recieved 'A'pplication 'F' version 'o'kay from XBD\n");
            return 0;
        }   else {
            XBH_DEBUG("Try %d: [%s]",trys,((uint8_t*)XBDResponseBuf));
            ++trys;
            xbd_reset(true);
            vTaskDelay(100);
            xbd_reset(false);
            vTaskDelay(1000);
        }
    }

    XBH_DEBUG("AF 'v'ersion 'i'nformation 'r'equest to XBD failed\n");

    return 1;

}/*}}}*/

/**
 * Switches from application to bootloader mode
 * @return 0 if success
 */
static uint8_t XBH_HandleStartBootloaderRequest() {/*{{{*/
    uint16_t trys;
    int retval;

    trys=0;
    while(trys<3) {

        memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_sbr], XBD_COMMAND_LEN);
        xbdSend(XBDCommandBuf, XBD_COMMAND_LEN);

        vTaskDelay(500);

        memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_vir], XBD_COMMAND_LEN);
        XBH_DEBUG("Sending 'v'ersion 'i'nformation 'r'equest to XBD\n");
        xbdSend(XBDCommandBuf, XBD_COMMAND_LEN);
        retval = xbdReceive(XBDResponseBuf, XBD_COMMAND_LEN);
        if(0 == memcmp(XBDResponseBuf,XBD_CMD[XBD_CMD_BLo],XBD_COMMAND_LEN) && 0 == retval) {   
            XBH_DEBUG("Recieved 'B'oot'L'oader version 'o'kay from XBD\n");
            return 0;
        } else {
            XBH_DEBUG("Try %d: [%s]",trys,((uint8_t*)XBDResponseBuf));
            ++trys;
            xbd_reset(true);
            vTaskDelay(100);
            xbd_reset(false);
            vTaskDelay(1000);
        }
    }
    XBH_DEBUG("BL 'v'ersion 'i'nformation 'r'equest to XBD failed\n");
    return 1;
    
}/*}}}*/

/**
 * Returns bootloader/application status in p_answer
 */
static void XBH_HandleSTatusRequest(uint8_t* p_answer) {/*{{{*/
    memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_vir],XBD_COMMAND_LEN);
    xbdSend(XBDCommandBuf, XBD_COMMAND_LEN);
    //TODO Check if valid 
    xbdReceive(p_answer, XBD_COMMAND_LEN);
}/*}}}*/



/**
 * Retrieves stack usage information from XBD
 */
int XBH_HandleStackUsageRequest(uint8_t* p_answer) {/*{{{*/
    int retval;
    XBH_DEBUG("Sending 's'tack 'r'equest to XBD\n");

    memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_sur], XBH_COMMAND_LEN);
//  XBH_DEBUG("sur\n");

    memset(XBDResponseBuf,'-',XBD_COMMAND_LEN+NUMBSIZE);


    xbdSend(XBDCommandBuf, XBD_COMMAND_LEN);
    retval = xbdReceive(XBDResponseBuf, XBD_COMMAND_LEN+NUMBSIZE);

//  XBH_DEBUG("Answer: [%s]",((uint8_t*)XBDResponseBuf));

    if(0 == memcmp(XBDResponseBuf,XBD_CMD[XBD_CMD_suo],XBD_COMMAND_LEN) && 0 == retval) {   
        XBH_DEBUG("'s'tack 'u'sage 'o'kay received from XBD\n");
        //Copy to Stack Usage information for transmission to the XBS
        memcpy(p_answer, XBDResponseBuf+XBD_COMMAND_LEN, NUMBSIZE);

        return 0;
    } else {
        XBH_DEBUG("'s'tack 'u'sage 'o'kay not received from XBD\n");
        return 1;
    }
    return -1;
}/*}}}*/





/**
 * Decode commands
 * @param input buffer containing commands 
 * @param input_len length of buffer
 * @param reply Buffer containing output to return (MUST be greater or equal to XBH_COMMAND_LEN
 * @return Length of reply containing return data
 */
size_t XBH_handle(const uint8_t *input, size_t input_len, uint8_t *reply) {/*{{{*/
    int ret;
    XBH_DEBUG("XBH_handle()\n");

    if ( (0 == memcmp(XBH_CMD[XBH_CMD_srr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 's'ubversion/git 'r'evision 'r'equest received\n");
        ret = XBH_HandleRevisionRequest(&reply[XBH_COMMAND_LEN]);

        XBH_DEBUG("'s'ubversion/git 'r'evision 'o'kay sent\n");
        memcpy(reply, XBH_CMD[XBH_CMD_sro], XBH_COMMAND_LEN);
        return ret+XBH_COMMAND_LEN;
    }/*}}}*/

    if ( (0 == memcmp(XBH_CMD[XBH_CMD_exr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 'ex'ecution 'r'equest received\n");
        ret=XBH_HandleEXecutionRequest();

        if(0 == ret) {
            XBH_DEBUG("Handle 'ex'ecution 'o'kay sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_exo], XBH_COMMAND_LEN);
            return XBH_COMMAND_LEN;
        } else {
            XBH_DEBUG("Handle 'ex'ecution 'f'ail sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_exf], XBH_COMMAND_LEN);
            return XBH_COMMAND_LEN;
        }
    }/*}}}*/

    if ( (0 == memcmp(XBH_CMD[XBH_CMD_cdr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 'c'ode 'd'ownload 'r'equest received\n");

        ret=XBH_HandleCodeDownloadRequest(&input[XBH_COMMAND_LEN],(input_len-XBH_COMMAND_LEN));

        if(0 == ret)
        {
            XBH_DEBUG("'c'ode 'd'ownload 'o'kay sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_cdo], XBH_COMMAND_LEN);
            return (uint16_t) XBH_COMMAND_LEN;
        } else {
            XBH_DEBUG("'c'ode 'd'ownload 'f'ail sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_cdf], XBH_COMMAND_LEN);
            return (uint16_t) XBH_COMMAND_LEN;
        }   
    }/*}}}*/

    if ( (0 == memcmp(XBH_CMD[XBH_CMD_tcr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 't'iming 'c'alibration 'r'equest received\n");
        ret=XBH_HandleTimingCalibrationRequest(&reply[XBH_COMMAND_LEN]);
        
        if(0 == ret) {
            XBH_DEBUG("'t'iming 'c'alibration 'o'kay sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_tco], XBH_COMMAND_LEN);
            return (uint16_t) XBH_COMMAND_LEN+TIMESIZE;
        } else {
            XBH_DEBUG("'t'iming 'c'alibration 'f'ail sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_tcf], XBH_COMMAND_LEN);
            return (uint16_t) XBH_COMMAND_LEN;
        }
    }/*}}}*/

    if ( (0 == memcmp(XBH_CMD[XBH_CMD_trr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 't'arget 'r'evision 'r'equest received\n");
        //resetTimer=resetTimerBase*2;
        ret = XBH_HandleTargetRevisionRequest(&reply[XBH_COMMAND_LEN]);
        //resetTimer=0;
        if(0 == ret) {
            XBH_DEBUG("'t'arget 'r'evision 'o'kay sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_tro], XBH_COMMAND_LEN);
            return (uint16_t) XBH_COMMAND_LEN+REVISIZE;
        } else {
            XBH_DEBUG("'t'arget 'r'evision 'f'ail sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_trf], XBH_COMMAND_LEN);
            return (uint16_t) XBH_COMMAND_LEN;
        }
    }/*}}}*/

    if ( (0 == memcmp(XBH_CMD[XBH_CMD_rpr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 'r'e'p'ort timestamp 'r'equest received\n");

        XBH_HandleRePorttimestampRequest(&reply[XBH_COMMAND_LEN]);
        
        XBH_DEBUG("'r'e'p'ort timestamp 'o'kay sent\n");
        memcpy(reply, XBH_CMD[XBH_CMD_rpo], XBH_COMMAND_LEN);
        return  XBH_COMMAND_LEN+(3*TIMESIZE);
    }/*}}}*/

   
    if ( (0 == memcmp(XBH_CMD[XBH_CMD_pwr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 'p'o'w'er measurement 'r'equest received\n");

        XBH_HandlePowerRequest(&reply[XBH_COMMAND_LEN]);
        
        XBH_DEBUG("'p'o'w'er measurement 'o'kay sent\n");
        memcpy(reply, XBH_CMD[XBH_CMD_pwo], XBH_COMMAND_LEN);
        return  XBH_COMMAND_LEN+(3*TIMESIZE);
    }/*}}}*/
    

 
     if ( (0 == memcmp(XBH_CMD[XBH_CMD_pgr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 'p'ower 'g'ain set 'r'equest received\n");

        XBH_HandlePowerGainRequest(&input[XBH_COMMAND_LEN],(input_len-XBH_COMMAND_LEN));
        
        XBH_DEBUG("'p'ower 'g'ain set 'o'kay sent\n");
        memcpy(reply, XBH_CMD[XBH_CMD_pwo], XBH_COMMAND_LEN);
        return  XBH_COMMAND_LEN;
    }/*}}}*/
    
    if ( (0 == memcmp(XBH_CMD[XBH_CMD_scr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 's'et 'c'ommunication 'r'equest received\n");

        ret=XBH_HandleSetCommunicationRequest(input[XBH_COMMAND_LEN]);
        
        if(ret==0)  {
            XBH_DEBUG("'s'et 'c'ommunication 'o'kay sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_sco], XBH_COMMAND_LEN);
        }else{
            XBH_DEBUG("'s'et 'c'ommunication 'f'ail sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_scf], XBH_COMMAND_LEN);
        }
        return (uint16_t) XBH_COMMAND_LEN;
    }   /*}}}*/

    if ( (0 == memcmp(XBH_CMD[XBH_CMD_dpr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 'd'ownload 'p'arameters 'r'equest received\n");


        //prepare TWI transmission to XBD here
        //TODO Find parameter requirements 
        ret=XBH_HandleDownloadParametersRequest(&input[XBH_COMMAND_LEN],(input_len-XBH_COMMAND_LEN));


        if(0 == ret) {
            XBH_DEBUG("'d'ownload 'p'arameters 'o'kay sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_dpo], XBH_COMMAND_LEN);
            return (uint16_t) XBH_COMMAND_LEN;
        } else {
            XBH_DEBUG("'d'ownload 'p'arameters 'f'ail sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_dpf], XBH_COMMAND_LEN);
            return (uint16_t) XBH_COMMAND_LEN;
        }
    }/*}}}*/

if ( (0 == memcmp(XBH_CMD[XBH_CMD_urr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 'u'pload 'r'esults 'r'equest received\n");

        
        ret=XBH_HandleUploadResultsRequest(&reply[XBH_COMMAND_LEN]);


        if(0 < ret) {
            XBH_DEBUG("'u'pload 'r'esults 'o'kay sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_uro], XBH_COMMAND_LEN);
            return XBH_COMMAND_LEN+ret;
        } else {
            ret = -ret;
            XBH_DEBUG("'u'pload 'r'esults 'f'ail sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_urf], XBH_COMMAND_LEN);
            // Append return value of XBH_HandleUploadResultsRequest 
            
            //reply[XBH_COMMAND_LEN]=ret;
            //return XBH_COMMAND_LEN+1;
            //Just discard return value, problem shouldn't happen normally
            return XBH_COMMAND_LEN;
        }
    }/*}}}*/

    if ( (0 == memcmp(XBH_CMD[XBH_CMD_ccr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 'c'hecksum 'c'alc 'r'equest received\n");
        ret=XBH_HandleChecksumCalcRequest();

        if(0 == ret) {
            XBH_DEBUG("Handle 'c'hecksum 'c'alc 'o'kay sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_cco], XBH_COMMAND_LEN);
            return (uint16_t) XBH_COMMAND_LEN;
        } else {
            XBH_DEBUG("Handle 'c'hecksum 'c'alc 'f'ail sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_ccf], XBH_COMMAND_LEN);
            return (uint16_t) XBH_COMMAND_LEN;
        }
    }/*}}}*/



    
    if ( (0 == memcmp(XBH_CMD[XBH_CMD_str],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 'st'atus 'r'equest received\n");
        XBH_HandleSTatusRequest(&reply[XBH_COMMAND_LEN]);
        
        memcpy(reply, XBH_CMD[XBH_CMD_sto], XBH_COMMAND_LEN);
        XBH_DEBUG("'st'atus 'o'kay sent\n");
        return (uint16_t) 2*XBH_COMMAND_LEN;
    }/*}}}*/

    if ( (0 == memcmp(XBH_CMD[XBH_CMD_rcr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 'r'eset 'c'ontrol 'r'equest received\n");
        if(input[XBH_COMMAND_LEN] == 'y'){
            xbd_reset(true);
        }else if (input[XBH_COMMAND_LEN] == 'n'){
            xbd_reset(false);
        }else{
            XBH_DEBUG("'r'eset 'c'ontrol 'f'ail sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_rcf], XBH_COMMAND_LEN);
            return (uint16_t) XBH_COMMAND_LEN;
        }

        
        XBH_DEBUG("'r'eset 'c'ontrol 'o'kay sent\n");
        memcpy(reply, XBH_CMD[XBH_CMD_rco], XBH_COMMAND_LEN);
        return (uint16_t) XBH_COMMAND_LEN;
    }/*}}}*/

    if ( (0 == memcmp(XBH_CMD[XBH_CMD_sar],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 's'tart 'a'pplication 'r'equest received\n");
        ret=XBH_HandleStartApplicationRequest();
        
        if(0 == ret)
        {
            XBH_DEBUG("'s'tart 'a'pplication 'o'kay sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_sao], XBH_COMMAND_LEN);
            return (uint16_t) XBH_COMMAND_LEN;
        } else {
            XBH_DEBUG("'s'tart 'a'pplication 'f'ail sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_saf], XBH_COMMAND_LEN);
            return (uint16_t) XBH_COMMAND_LEN;
        }
    }/*}}}*/

    if ( (0 == memcmp(XBH_CMD[XBH_CMD_sur],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 's'tack 'u'sage 'r'equest received\n");
        ret=XBH_HandleStackUsageRequest(&reply[XBH_COMMAND_LEN]);
        
        if(0 == ret) {
            XBH_DEBUG("'s'tack 'u'sage 'o'kay sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_suo], XBH_COMMAND_LEN);
            return (uint16_t) XBH_COMMAND_LEN+NUMBSIZE;
        } else {
            XBH_DEBUG("'s'tack 'u'sage 'f'ail sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_suf], XBH_COMMAND_LEN);
            return (uint16_t) XBH_COMMAND_LEN;
        }
    }/*}}}*/

    if ( (0 == memcmp(XBH_CMD[XBH_CMD_sbr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 's'tart 'b'ootloader 'r'equest received\n");
        ret=XBH_HandleStartBootloaderRequest();
        
        if(0 == ret) {
            XBH_DEBUG("'s'tart 'b'ootloader 'o'kay sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_sbo], XBH_COMMAND_LEN);
            return (uint16_t) XBH_COMMAND_LEN;
        } else {
            XBH_DEBUG("'s'tart 'b'ootloader 'f'ail sent\n");
            memcpy(reply, XBH_CMD[XBH_CMD_sbf], XBH_COMMAND_LEN);
            return (uint16_t) XBH_COMMAND_LEN;
        }
    }/*}}}*/


    memcpy(reply, XBH_CMD[XBH_CMD_unk], XBH_COMMAND_LEN);
    XBH_DEBUG("'un'known command 'r'eceived (len: %d)\n", input_len);
    for(uint8_t u=0;u<input_len;++u) {
        if(u % 32 == 0){ 
            XBH_DEBUG("\n");
        }
        XBH_DEBUG("%x\n",input[u]);
    }
    XBH_DEBUG("\n");
    
    return (uint16_t) XBH_COMMAND_LEN;
}/*}}}*/
