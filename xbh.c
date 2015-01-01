/**
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
#include <stdlib.h>
#include <string.h>

#include "FreeRTOSConfig.h"
#include <FreeRTOS.h>
#include <queue.h>

#include <lwip/sockets.h>

#include "hal/hal.h"
#include "hal/lwip_eth.h"
#include "hal/measure.h"
#include "xbh.h"
#include "xbh_config.h"
#include "xbh_prot.h"
#include "xbh_xbdcomm.h"
#include "xbd_multipacket.h"

#include "util.h"

#define XBH_REV_DIGITS 7


uint16_t resetTimerBase=10;	//10s is the initial base unit for resetTimer settings

/**
 * XBH Command String Constants
 */
const char *XBH_CMD[] = {
     FOREACH_XBH_CMD(XBH_CMD_DEF)
};

/**
 * XBD Command String Constants */
const char *XBD_CMD[] = {
     FOREACH_XBH_CMD(XBD_CMD_DEF)
};

static uint8_t XBDCommandBuf[XBD_PACKET_SIZE_MAX];
static uint8_t XBDResponseBuf[XBD_ANSWERLENG_MAX];


// From xbd_multipacket.h
// TODO Put these into a state struct
extern uint32_t xbd_recmp_datanext;
extern uint32_t xbd_recmp_dataleft;


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
	p_answer[XBH_REV_DIGITS+1+2*6+1]=0;
	return XBH_REV_DIGITS+1+2*6+1;
}/*}}}*/

/**
 * Asks XBD to execute
 * @param socket Sock to stream power measurements to
 * @return 0 if success, 1 if fail.
 */
static uint8_t XBH_HandleEXecutionRequest(int sock) {/*{{{*/
    struct pwr_sample sample; 
    int retval;
    // 4 bytes pkt ID + sizeof pwr_sample in ascii hex. Subtract 2 to remove padding
    char pkt_buf[4+2*(sizeof(struct pwr_sample)-2)]; 
    
    memcpy(pkt_buf, "PWRD", 4);


    //Kick off power measurement
    measure_start();

    // Send execution request to XBD
    XBH_DEBUG("Sending 'ex'ecution  'r'equest to XBD\n");
	memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_exr], XBD_COMMAND_LEN);
	xbdSend(XBDCommandBuf, XBD_COMMAND_LEN);


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


    // Receive status from XBD
	xbdReceive(XBDResponseBuf, XBD_COMMAND_LEN);
	
	if(0 == memcmp(XBDResponseBuf,XBD_CMD[XBD_CMD_exo], XBD_COMMAND_LEN)) {	
        XBH_DEBUG("Received 'ex'ecution 'o'kay from XBD\n");
/*		XBH_DEBUG("Took %d s, %d IRQs, %d clocks\r\n",
						risingTimeStamp-fallingTimeStamp,
						risingIntCtr-fallingIntCtr,
						risingTime-fallingTime);
*/		return 0;
	} else {
        XBH_DEBUG("Did not receive 'ex'ecution 'o'kay from XBD\n");
		return 1;
	}
}/*}}}*/


#if 0 /*{{{*/

//TODO: Implement
uint8_t XBH_HandleTimingCalibrationRequest(uint8_t* p_answer) {/*{{{*/
	uint8_t i;

	constStringToBuffer (XBDCommandBuf, XBDtcr);
	memset(XBDResponseBuf,'-',XBD_COMMAND_LEN+NUMBSIZE);
	xbdSend(XBD_COMMAND_LEN, XBDCommandBuf);
	xbdReceive(XBD_COMMAND_LEN+NUMBSIZE, XBDResponseBuf);

	XBH_DEBUG("tcr\n");

//	XBH_DEBUG("Answer: [%s]",((uint8_t*)XBDResponseBuf));

	if(0 == memcmp_P(XBDResponseBuf,XBDtco,XBD_COMMAND_LEN))
	{	

		for(i=0;i<4;++i)
		{
			p_answer[2*i] = ntoa(XBDResponseBuf[XBD_COMMAND_LEN+i]>>4);
			p_answer[2*i+1] = ntoa(XBDResponseBuf[XBD_COMMAND_LEN+i]&0xf);
		}
		return 0;
	}
	else
	{
		XBH_ERROR("Response wrong [%s\r\n",XBDResponseBuf);
		return 1;
	}
}
/*}}}*/
//TODO: Implement 
uint8_t XBH_HandleTargetRevisionRequest(uint8_t* p_answer) {/*{{{*/
	uint8_t i;

	constStringToBuffer (XBDCommandBuf, XBDtrr);
	memset(XBDResponseBuf,'-',XBD_COMMAND_LEN+REVISIZE);
	xbdSend(XBD_COMMAND_LEN, XBDCommandBuf);
	xbdReceive(XBD_COMMAND_LEN+REVISIZE, XBDResponseBuf);

	XBH_DEBUG("trr\n");

//	XBH_DEBUG("Answer: [%s]",((uint8_t*)XBDResponseBuf));

	if(0 == memcmp_P(XBDResponseBuf,XBDtro,XBD_COMMAND_LEN))
	{	
		for(i=0;i<REVISIZE;++i)
		{
			p_answer[i] = XBDResponseBuf[XBD_COMMAND_LEN+i];
		}
		return 0;
	}
	else
	{
	        XBH_ERROR("trr fai\r\n");
		return 1;
	}

}/*}}}*/


/**
 * Program XBD
 * TODO Clean up code, minimize scope of variables and document them.
 * @param input_buf Code buffer containing code in ascii hex format, prefixed by
 * 4 byte address
 * @param len Length of buffer in bytes (in hex format)
 * @return 0 if success, 1 if fails during program flash request to XBD, 2 if
 * fails during flash download request to XBD
 */
static uint8_t XBH_HandleCodeDownloadRequest(const uint8_t *input_buf, uint32_t len) {/*{{{*/
	uint32_t *cmd_ptr = (uint32_t*)(XBDCommandBuf+XBD_COMMAND_LEN);
	uint32_t seqn=0,addr=0;
    uint32_t byte_len = len/2; //byte length of len which is  ascii hex length in nybbles
	uint32_t len_remaining; //bytes left to send to XBD
    uint32_t fd_idx; // offset of packet to send to XBD
    uint32_t numbytes; // bytes written to XBD

    memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_pfr], XBD_COMMAND_LEN);

	//place and endian-convert code load addr (4 bytes)
	for(uint32_t i=0; i < ADDRSIZE; ++i) {
		addr |= ( (uint32_t)
				 (htoi(input_buf[(i*2)])<<4 |
				  htoi(input_buf[(i*2)+1]))
				) <<((3-i)*8)uint32_t ;
	}

	*cmd_ptr = htons(addr);

	//place LENG (in correct endianess)
	++cmd_ptr; //ADDRSIZE == sizeof(uint32_t)
	*cmd_ptr = htons((uint32_t)(byte_len-ADDRSIZE));

    XBH_DEBUG( "Sending 'p'rogram 'f'lash 'r'equest to XBD\n");
	xbdSend(XBD_COMMAND_LEN+ADDRSIZE+LENGSIZE, XBDCommandBuf);
	xbdReceive(XBD_COMMAND_LEN, XBDResponseBuf);

	
	if(0 != memcmp(XBD_CMD[XBD_CMD_pfo],XBDResponseBuf,XBH_COMMAND_LEN)) {
        XBH_DEBUG("Error: Did not receive 'p'rogram 'f'lash 'o'kay from XBD!\n");
		return 1;
	}else{
        XBH_DEBUG("Received 'p'rogram 'f'lash 'o'kay from XBD\n");
    }


	memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_fdr], XBD_COMMAND_LEN);
	len_remaining=(byte_len-ADDRSIZE);
	fd_idx = ADDRSIZE;
	cmd_ptr = (uint32_t *)&XBDCommandBuf[XBD_COMMAND_LEN];
    // Loop through, sending code in 128 byte packets + code
    // TODO: Find out why 128 bytes
    //
	while(len_remaining != 0) {
        uint32_t i;
		*cmd_ptr = htons(seqn);
		++seqn;
        //TODO Find out why 128 is hardcoded in here. XBD_PACKET_SIZE_MAX/2 ? 
		for(i=0; i < (len_remaining<128?len_remaining:128) ; ++i) {
			XBDCommandBuf[XBD_COMMAND_LEN+SEQNSIZE+i]=
				htoi(input_buf[((fd_idx+i)*2)])<<4 |
				htoi(input_buf[((fd_idx+i)*2)+1]);
		}
		numbytes=i;
        XBH_DEBUG("Sending 'f'lash 'd'ownload 'r'equest to XBD\n");
		xbdSend(XBD_COMMAND_LEN+SEQNSIZE+numbytes, XBDCommandBuf);
		xbdReceive(XBD_COMMAND_LEN, XBDResponseBuf);
				
		if(0 == memcmp(XBD_CMD[XBD_CMD_pfo],XBDResponseBuf,XBH_COMMAND_LEN)) {
            XBH_DEBUG("Received 'p'rogram 'f'lash 'o'kay from XBD\n");
			len_remaining-=numbytes;
			fd_idx+=numbytes;
		} else {
            XBH_DEBUG("Error: Did not receive 'p'rogram 'f'lash 'o'kay from XBD\n");
			return 2;
		}	
	}
	return 0;

}/*}}}*/

/**
 * Set XBD communication protocol
 * @param requestedComm Requested communication protocol. 'U' for UART, 'O' for
 * overdriven UART, 'I' for I2C, 'E' for ethernet.
 * @return 0 if successful, 2 if invalid
 */
static uint8_t XBH_HandleSetCommunicationRequest(const uint8_t requestedComm) {/*{{{*/
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
 * TODO Clean up code, minimize scope of variables and document them.
 * @param input_buf Buffer containing type code in ascii hex format 
 * @param len Length of buffer in bytes (in hex format)
 * @return 0 if success, 1 if fails during program parameter request to XBD, 2 if
 * fails during program download request to XBD
 */
static uint8_t XBH_HandleDownloadParametersRequest(const uint8_t *input_buf, uint32_t len) {/*{{{*/
	uint32_t *cmd_ptr = (uint32_t*)(XBDCommandBuf+XBD_COMMAND_LEN);//+TYPESIZE+ADDRSIZE);
	uint32_t seqn=0,temp=0;
    uint32_t byte_len = len/2; //byte length of len which is  ascii hex length in nybbles
	uint32_t len_remaining; //bytes left to send to XBD
    uint32_t fd_idx; // offset of packet to send to XBD
    uint32_t numbytes; // bytes written to XBD

    memcpy(XBDCommandBuf,XBD_CMD[XBD_CMD_ppr], XBD_COMMAND_LEN);

	//place and endian-convert TYPE (4 bytes)
	for(uint32_t i=0; i < TYPESIZE ; ++i) {
		temp |= ( (uint32_t)
				 (htoi(input_buf[(i*2)])<<4 |
				  htoi(input_buf[(i*2)+1]))
				) <<((3-i)*8);
	}
	*cmd_ptr = htons(temp);
	++cmd_ptr; //TYPESIZE == sizeof(uint32_t)
	temp=0;
	//place and endian-convert ADDR (4 bytes)
	for(uint32_t i=0; i < ADDRSIZE ; ++i) {
		temp |= ( (uint32_t)
				 (htoi(input_buf[(TYPESIZE+i)*2])<<4 |
				  htoi(input_buf[(TYPESIZE+i)*2+1]))
				) <<((3-i)*8);
	}
	*cmd_ptr = htons(temp);
	++cmd_ptr; //ADDRSIZE == sizeof(uint32_t)
	temp=0;

	//place LENG (in correct endianess)
	*cmd_ptr =  htons((uint32_t)(byte_len-ADDRSIZE-TYPESIZE));


    XBH_DEBUG("Sending 'p'rogram 'p'arameters 'r'equest to XBD\n");
	xbdSend(XBD_COMMAND_LEN+TYPESIZE+ADDRSIZE+LENGSIZE, XBDCommandBuf);
	xbdReceive(XBD_COMMAND_LEN, XBDResponseBuf);

	
	if(0 != memcmp(XBD_CMD[XBD_CMD_ppo],XBDResponseBuf,XBH_COMMAND_LEN)) {
        XBH_DEBUG("Did not receive 'p'rogram 'p'arameters 'o'kay from XBD\n");
		return 1;
	}
    XBH_DEBUG("Received 'p'rogram 'p'arameters 'o'kay from XBD\n");


    memcpy(XBDCommandBuf,XBD_CMD[XBD_CMD_pdr], XBD_COMMAND_LEN);
	len_remaining=(byte_len-TYPESIZE-ADDRSIZE);
	fd_idx = ADDRSIZE+TYPESIZE;
	cmd_ptr = (uint32_t *)&XBDCommandBuf[XBD_COMMAND_LEN];
	while(len_remaining != 0) {
		*cmd_ptr =  htons(seqn);
		++seqn;
		for(i=0; i < (len_remaining<128?len_remaining:128) ; ++i)
		{
			XBDCommandBuf[XBD_COMMAND_LEN+SEQNSIZE+i]=
				htoi(input_buf[((fd_idx+i)*2)])<<4 |
				htoi(input_buf[((fd_idx+i)*2)+1]);
		}
		numbytes=i;
        XBH_DEBUG("Sending 'p'rogram 'd'ownload 'r'equest to XBD\n");
		xbdSend(byte_len=XBD_COMMAND_LEN+SEQNSIZE+numbytes, XBDCommandBuf);
		xbdReceive(XBD_COMMAND_LEN, XBDResponseBuf);

		if(0 == memcmp(XBD_CMD[XBD_CMD_pdo],XBDResponseBuf,XBH_COMMAND_LEN)) {
            XBH_DEBUG("Received 'p'rogram 'd'ownload 'o'kay from XBD\n");
			len_remaining-=numbytes;
			fd_idx+=numbytes;
		} else {
            XBH_DEBUG("Did not recieve 'p'rogram 'd'ownload 'o'kay from XBD\n");
			return 2;
		}	
	}
	return 0;
}/*}}}*/


/**
 * Asks XBD to calculate checksum
 * @return 0 if success, 1 if fail.
 */
static uint8_t XBH_HandleChecksumCalcRequest() {/*{{{*/
	memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_ccr], XBD_COMMAND_LEN);
    XBH_DEBUG("Sending 'c'hecksum 'c'alc 'r'equest to XBD\n");
	xbdSend(XBD_COMMAND_LEN, XBDCommandBuf);
	xbdReceive(XBD_COMMAND_LEN, XBDResponseBuf);
	

	if(0 == memcmp_P(XBDResponseBuf,XBD_CMD[XBD_CMD_cco],XBD_COMMAND_LEN)) {	
        XBH_DEBUG("Received 'c'hecksum 'c'alc 'o'kay from XBD\n");
/*		XBH_DEBUG("Took %d s, %d IRQs, %d clocks\r\n",
				risingTimeStamp-fallingTimeStamp,
				risingIntCtr-fallingIntCtr,
				risingTime-fallingTime);
*/		return 0;
	} else {
        XBH_DEBUG("Did not receive 'c'hecksum 'c'alc 'o'kay from XBD\n");
		return 1;
	}
}/*}}}*/


/**
 * Retrieve value of computation
 * @param p_answer Buffer to write answer to 
 * @return 0 if success
 */
static uint8_t XBH_HandleUploadResultsRequest(uint8_t* p_answer) {/*{{{*/
	uint8_t ret;

    XBH_DEBUG("Sending 'u'pload 'r'esults 'r'equest to XBD\n");
	memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_urr], XBD_COMMAND_LEN);

/*	for(uint8_t u=0;u<XBD_COMMAND_LEN;++u)
	{
			XBH_DEBUG("%x ",XBDCommandBuf[u]);
	}
	XBH_DEBUG("\r\n");
*/
	xbdSend(XBD_COMMAND_LEN, XBD_CMD[XBD_CMD_urr]);
//	_delay_ms(100);


	xbdReceive(XBD_ANSWERLENG_MAX-CRC16SIZE, XBDResponseBuf);
	
	if(0 == XBD_recInitialMultiPacket(XBDResponseBuf,XBD_ANSWERLENG_MAX-CRC16SIZE, XBD_CMD[XBD_CMD_uro], 1 /*hastype*/, 0/*hasaddr*/)){
        XBH_DEBUG("Received 'u'pload 'r'esults 'o'kay from XBD\n");
    }else{
        XBH_DEBUG("Error with 'u'pload 'r'esults 'o'kay from XBD\n");
    }
        


/*
	uint8_t XBDResponseBuf[XBD_ANSWERLENG_MAX];
	uint8_t XBHMultiPacketDecodeBuf[XBD_ANSWERLENG_MAX];

	for(uint8_t u=0;u<XBD_RESULTLEN_EBASH;++u)
	{
			XBH_DEBUG("%x ",XBDResponseBuf[u]);
	}
	XBH_DEBUG("\r\n");
*/

	memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_rdr], XBD_COMMAND_LEN);
	do {
        XBH_DEBUG("Sending 'r'esult 'd'ata 'r'equest to XBD\n");
		xbdSend(XBD_COMMAND_LEN, XBD_CMD[XBD_CMD_rdr]);
//		_delay_ms(100);

	//	XBH_DEBUG("1.xbd_recmp_dataleft: %\r\n",xbd_recmp_dataleft);
		xbdReceive(XBD_ANSWERLENG_MAX-CRC16SIZE, XBDResponseBuf);
		ret=XBD_recSucessiveMultiPacket(XBDResponseBuf, XBD_ANSWERLENG_MAX-CRC16SIZE, p_answer, MTU_SIZE/2, XBDrdo);
	//	XBH_DEBUG("2.xbd_recmp_dataleft: %\r\n",xbd_recmp_dataleft);
	//	XBH_DEBUG("ret: %\r\n",ret);
	} while(xbd_recmp_dataleft != 0 && 0 == ret); 


	if(ret==0 && 0 == memcmp(XBDResponseBuf,XBD_CMD[XBD_CMD_rdo],XBD_COMMAND_LEN)) {	
		for(int32_t i=xbd_recmp_datanext-1;i>=0; --i)
		{
			p_answer[2*i] = ntoa(p_answer[i]>>4);
			p_answer[2*i+1] = ntoa(p_answer[i]&0xf);
		}
		return 0;
	} else {
        XBH_DEBUG("'r'esult 'd'ata 'r'equest to XBD failed\n");
		return 0x80+ret;
	}
}/*}}}*/

/**
 * Switches from bootloader to application mode
 * @return 0 if success
 */
static uint8_t XBH_HandleStartApplicationRequest() {/*{{{*/
    uint16_t trys;
    //TODO replace w/ C99 bool
	volatile uint8_t yes='y';
	volatile uint8_t no='n';

	trys=0;
	while(trys<3) {
        XBH_DEBUG("Sending 's'tart 'a'pplication 'r'equest to XBD\n");
		memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_sar], XBD_COMMAND_LEN);
		xbdSend(XBD_COMMAND_LEN, XBDCommandBuf);
        //TODO Replace _delay_ms w/ ARM-specific function
		_delay_ms(100);

        XBH_DEBUG("Sending 'v'ersion 'i'nformation 'r'equest to XBD\n");
		memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_vir], XBD_COMMAND_LEN);
		xbdSend(XBD_COMMAND_LEN, XBDCommandBuf);
		xbdReceive(XBD_COMMAND_LEN, XBDResponseBuf);
	

        if(0 == memcmp(XBDResponseBuf,XBD_CMD[XBD_CMD_AFo],XBD_COMMAND_LEN)) {	
            XBH_DEBUG("Recieved 'A'pplication 'F' version 'o'kay from XBD\n");
            return 0;
        }	else {
            XBH_DEBUG("Try %d: [%s]",trys,((uint8_t*)XBDResponseBuf));
            ++trys;
            XBH_HandleResetControlRequest(yes);
            _delay_ms(100);
            XBH_HandleResetControlRequest(no);
            _delay_ms(1000);
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
	volatile uint8_t yes='y';
	volatile uint8_t no='n';

	trys=0;
	while(trys<3) {

		memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_sbr], XBD_COMMAND_LEN);
		xbdSend(XBD_COMMAND_LEN, XBDCommandBuf);

		_delay_ms(100);

		memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_vir], XBD_COMMAND_LEN);
        XBH_DEBUG("Sending 'v'ersion 'i'nformation 'r'equest to XBD\n");
		xbdSend(XBD_COMMAND_LEN, XBDCommandBuf);
		xbdReceive(XBD_COMMAND_LEN, XBDResponseBuf);
		if(0 == memcmp(XBDResponseBuf,XBD_CMD[XBD_CMD_BLo],XBD_COMMAND_LEN)) {	
            XBH_DEBUG("Recieved 'B'oot'L'oader version 'o'kay from XBD\n");
			return 0;
		} else {
            XBH_DEBUG("Try %d: [%s]",trys,((uint8_t*)XBDResponseBuf));
			++trys;
			XBH_HandleResetControlRequest(yes);
			_delay_ms(100);
			XBH_HandleResetControlRequest(no);
			_delay_ms(1000);
		}
	}
    XBH_DEBUG("BL 'v'ersion 'i'nformation 'r'equest to XBD failed\n");
	return 1;
	
}/*}}}*/

/**
 * Returns bootloader/application status in p_answer
 * @param p_answer pointer to buffer to receive status
 */
static void XBH_HandleSTatusRequest(uint8_t* p_answer) {/*{{{*/
	memcpy(XBDCommandBuf, XBD_CMD[XBD_CMD_vir],XBD_COMMAND_LEN);
	xbdSend(XBD_COMMAND_LEN, XBDCommandBuf);
	xbdReceive(XBD_COMMAND_LEN, p_answer);
}/*}}}*/


/**
 * Sets reset pin depending on value of param
 * TODO Change to use c99 bool
 * TODO Actually implement
 * @return 0 if valid parameter, 1 if not
 */
uint8_t XBH_HandleResetControlRequest(uint8_t param) {/*{{{*/
	//Disable pull-up in input, set to low level if output
	PORTB &= ~(_BV(PB0));

	if(param == 'y' || param == 'Y')
	{
		//set DDR bit -> output
		DDRB |= _BV(PB0);
	
		return 0;
	}
	
	if(param == 'n' || param == 'N')
	{
		//clear DDR bit -> input
		DDRB &=  ~(_BV(PB0));

	
		return 0;
	}

	XBH_ERROR("Invalid Reset Reques\r\n");
	return 1;
}	/*}}}*/

/**
 * Retrieves stack usage information from XBD
 */
uint8_t XBH_HandleStackUsageRequest(uint8_t* p_answer) {/*{{{*/
    XBH_DEBUG("Sending 's'tack 'r'equest to XBD\n");

    memcpy(output, XBH_CMD[XBD_CMD_sur], XBH_COMMAND_LEN);
//	XBH_DEBUG("sur\n");

	memset(XBDResponseBuf,'-',XBD_COMMAND_LEN+NUMBSIZE);


	xbdSend(XBD_COMMAND_LEN, XBDCommandBuf);
	xbdReceive(XBD_COMMAND_LEN+NUMBSIZE, XBDResponseBuf);

//	XBH_DEBUG("Answer: [%s]",((uint8_t*)XBDResponseBuf));

	if(0 == memcmp_P(XBDResponseBuf,XBDsuo,XBD_COMMAND_LEN)) {	
        XBH_DEBUG("'s'tack 'u'sage 'o'kay received from XBD\n");
		//Copy to Stack Usage information for transmission to the XBS
		for(uint32_t i=0;i<4;++i) {
			p_answer[2*i] = ntoa(XBDResponseBuf[XBD_COMMAND_LEN+i]>>4);
			p_answer[2*i+1] = ntoa(XBDResponseBuf[XBD_COMMAND_LEN+i]&0xf);
		}
		return 0;
	} else {
        XBH_DEBUG("'s'tack 'u'sage 'o'kay not received from XBD\n");
		return 1;
	}
    return -1;
}/*}}}*/

//TODO: Redefine OCRVAL
//TODO: Counting cycles probably not accurate on ARM

void XBH_HandleRePorttimestampRequest(uint8_t* p_answer)	{/*{{{*/

	uint32_t tmpvar;
	int32_t rising_fract, falling_fract;
	

//#ifdef XBH_DEBUG_TIMING
//	XBH_DEBUG("\nRP, rising: %d s, %d IRQs, clocks:",
//						risingTimeStamp,
//						risingIntCtr);
//	XBD_debugOutHex32Bit(risingTime);
//
//	XBH_DEBUG("\nRP, falling: %d s, %d IRQs, clocks:",
//						fallingTimeStamp,
//						fallingIntCtr);
//	XBD_debugOutHex32Bit(fallingTime);
//#endif

	rising_fract=risingIntCtr*OCRVAL+risingTime;

//#ifdef XBH_DEBUG_TIMING
//	XBH_DEBUG("\nrising_fract:");
//	XBD_debugOutHex32Bit(rising_fract);
//#endif

	falling_fract=fallingIntCtr*OCRVAL+fallingTime;
//#ifdef XBH_DEBUG_TIMING
//	XBH_DEBUG("\nfalling_fract:");
//	XBD_debugOutHex32Bit(falling_fract);
//#endif

	if(rising_fract < falling_fract)
	{
		rising_fract+=F_CPU;
		--risingTimeStamp;
	}

//#ifdef XBH_DEBUG_TIMING
//	XBH_DEBUG("\nrising_fract:");
//	XBD_debugOutHex32Bit(rising_fract);
//
//	XBH_DEBUG("RP, rising: %d s, %d IRQs, %d clocks\r\n",
//						risingTimeStamp,
//						risingIntCtr,
//						risingTime);
//#endif


	tmpvar = (risingTimeStamp-fallingTimeStamp);
	for(uint32_t i=0;i<8;++i)
	{
		*p_answer++ = ntoa((tmpvar>>(28-(4*i)))&0xf);
	}

#ifdef XBH_DEBUG_TIMING
	XBH_DEBUG("\ntmpvar:");
	XBD_debugOutHex32Bit(tmpvar);
#endif
					
	tmpvar = (rising_fract - falling_fract);
	for(uint32_t i=0;i<8;++i)
	{
		*p_answer++ = ntoa((tmpvar>>(28-(4*i)))&0xf);
	}

#ifdef XBH_DEBUG_TIMING
	XBH_DEBUG("\ntmpvar:");
	XBD_debugOutHex32Bit(tmpvar);
#endif


	tmpvar = (F_CPU);
	for(i=0;i<8;++i)
	{
		*p_answer++ = ntoa((tmpvar>>(28-(4*i)))&0xf);
	}
	
#ifdef XBH_DEBUG_TIMING
	XBH_DEBUG("\ntmpvar:");
	XBD_debugOutHex32Bit(tmpvar);
#endif
}/*}}}*/
#endif/*}}}*/







/**
 * Decode commands
 * @param input buffer containing commands 
 * @param input_len length of buffer
 * @param reply Buffer containing output to return (MUST be greater or equal to XBH_COMMAND_LEN
 * @return Length of reply containing return data
 */
size_t XBH_handle(int sock, const uint8_t *input, size_t input_len, uint8_t *reply) {/*{{{*/
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
        XBH_DEBUG("Proper handle 'ex'ecution 'r'equest received\n");
		ret=XBH_HandleEXecutionRequest(sock);

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

#if 0
	if ( (0 == memcmp(XBH_CMD[XBH_CMD_rpr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 'r'e'p'ort timestamp 'r'equest received\n");

		resetTimer=resetTimerBase*1;
		XBH_HandleRePorttimestampRequest(&output[XBH_COMMAND_LEN]);
		resetTimer=0;
		
        XBH_DEBUG("'r'e'p'ort timestamp 'o'kay sent\n");
        memcpy(output, XBH_CMD[XBH_CMD_rpo], XBH_COMMAND_LEN);
		return (uint16_t) XBH_COMMAND_LEN+(3*TIMESIZE*2);
	}/*}}}*/

	if ( (0 == memcmp(XBH_CMD[XBH_CMD_tcr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 't'iming 'c'alibration 'r'equest received\n");
		ret=XBH_HandleTimingCalibrationRequest(&output[XBH_COMMAND_LEN]);
		
		if(0 == ret) {
            XBH_DEBUG("'t'iming 'c'alibration 'o'kay sent\n");
            memcpy(output, XBH_CMD[XBH_CMD_tco], XBH_COMMAND_LEN);
			return (uint16_t) XBH_COMMAND_LEN+2*NUMBSIZE;
		} else {
            XBH_DEBUG("'t'iming 'c'alibration 'f'ail sent\n");
            memcpy(output, XBH_CMD[XBH_CMD_tcf], XBH_COMMAND_LEN);
			return (uint16_t) XBH_COMMAND_LEN;
		}
	}/*}}}*/

	if ( (0 == memcmp(XBH_CMD[XBH_CMD_trr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 't'arget 'r'evision 'r'equest received\n");
		//resetTimer=resetTimerBase*2;
		ret = XBH_HandleTargetRevisionRequest(&output[XBH_COMMAND_LEN]);
		//resetTimer=0;
		if(0 == ret) {
            XBH_DEBUG("'t'arget 'r'evision 'o'kay sent\n");
            memcpy(output, XBH_CMD[XBH_CMD_tro], XBH_COMMAND_LEN);
			return (uint16_t) XBH_COMMAND_LEN+REVISIZE;
		} else {
            XBH_DEBUG("'t'arget 'r'evision 'o'kay sent\n");
            memcpy(output, XBH_CMD[XBH_CMD_trf], XBH_COMMAND_LEN);
			return (uint16_t) XBH_COMMAND_LEN;
		}
	}/*}}}*/

	if ( (0 == memcmp(XBH_CMD[XBH_CMD_urr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 'u'pload 'r'esults 'r'equest received\n");

		resetTimer=resetTimerBase*1;
		
		ret=XBH_HandleUploadResultsRequest(&output[XBH_COMMAND_LEN]);

		resetTimer=0;		

		if(0 == ret) {
            XBH_DEBUG("'u'pload 'r'esults 'o'kay sent\n");
			memcpy(output, XBH_CMD[XBH_CMD_uro], XBH_COMMAND_LEN);
			return (uint16_t)XBH_COMMAND_LEN+xbd_recmp_datanext*2;
		} else {
            XBH_DEBUG("'u'pload 'r'esults 'f'ail sent\n");
			memcpy(output, XBH_CMD[XBH_CMD_urf], XBH_COMMAND_LEN);
            // Append return value of XBH_HandleUploadResultsRequest 
            
            // TODO Find what return value actually means
			output[XBH_COMMAND_LEN]= ntoa(ret>>4);
			output[XBH_COMMAND_LEN+1]= ntoa(ret&0xf);
			return (uint16_t) XBH_COMMAND_LEN+2;
		}
	}/*}}}*/

	if ( (0 == memcmp(XBH_CMD[XBH_CMD_cdr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 'c'ode 'd'ownload 'r'equest received\n");

		resetTimer=resetTimerBase*1;

		ret=XBH_HandleCodeDownloadRequest(&input[XBH_COMMAND_LEN],(input_len-XBH_COMMAND_LEN));

		resetTimer=0;		

		if(0 == ret)
		{
            XBH_DEBUG("'c'ode 'd'ownload 'o'kay sent\n");
			memcpy(output, XBH_CMD[XBH_CMD_cdo], XBH_COMMAND_LEN);
			return (uint16_t) XBH_COMMAND_LEN;
		} else {
            XBH_DEBUG("'c'ode 'd'ownload 'f'ail sent\n");
			return (uint16_t) XBH_COMMAND_LEN;
		}	
	}/*}}}*/

	if ( (0 == memcmp(XBH_CMD[XBH_CMD_dpr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 'd'ownload 'p'arameters 'r'equest received\n");

		resetTimer=resetTimerBase*1;

		//prepare TWI transmission to XBD here
        //TODO Find parameter requirements 
		ret=XBH_HandleDownloadParametersRequest(&input[XBH_COMMAND_LEN],(input_len-XBH_COMMAND_LEN)/2);

		resetTimer=0;		

		if(0 == ret) {
            XBH_DEBUG("'d'ownload 'p'arameters 'o'kay sent\n");
			memcpy(output, XBH_CMD[XBH_CMD_dpo], XBH_COMMAND_LEN);
			return (uint16_t) XBH_COMMAND_LEN;
		} else {
            XBH_DEBUG("'d'ownload 'p'arameters 'f'ail sent\n");
			memcpy(output, XBH_CMD[XBH_CMD_dpf], XBH_COMMAND_LEN);
			return (uint16_t) XBH_COMMAND_LEN;
		}
	}/*}}}*/

	if ( (0 == memcmp(XBH_CMD[XBH_CMD_scr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 's'et 'c'ommunication 'r'equest received\n");

		resetTimer=resetTimerBase*1;
		ret=XBH_HandleSetCommunicationRequest(input[XBH_COMMAND_LEN]);
		resetTimer=0;
		
		if(ret==0)	{
            XBH_DEBUG("'s'et 'c'ommunication 'o'kay sent\n");
			memcpy(output, XBH_CMD[XBH_CMD_sco], XBH_COMMAND_LEN);
        }else{
            XBH_DEBUG("'s'et 'c'ommunication 'f'ail sent\n");
			memcpy(output, XBH_CMD[XBH_CMD_scf], XBH_COMMAND_LEN);
        }
		return (uint16_t) XBH_COMMAND_LEN;
	}	/*}}}*/
	
	if ( (0 == memcmp(XBH_CMD[XBH_CMD_str],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 'st'atus 'r'equest received\n");
		resetTimer=resetTimerBase*1;
		XBH_HandleSTatusRequest(&output[XBH_COMMAND_LEN]);
		resetTimer=0;
		
        memcpy(output, XBH_CMD[XBH_CMD_sto], XBH_COMMAND_LEN);
        XBH_DEBUG("'st'atus 'o'kay sent\n");
		return (uint16_t) 2*XBH_COMMAND_LEN;
	}/*}}}*/

	if ( (0 == memcmp(XBH_CMD[XBH_CMD_rcr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 'r'eset 'c'ontrol 'r'equest received\n");
		resetTimer=resetTimerBase*1;
		ret=XBH_HandleResetControlRequest(input[XBH_COMMAND_LEN]);
		resetTimer=0;		
		
		if(0 == ret) {
            XBH_DEBUG("'r'eset 'c'ontrol 'o'kay sent\n");
            memcpy(output, XBH_CMD[XBH_CMD_rco], XBH_COMMAND_LEN);
			return (uint16_t) XBH_COMMAND_LEN;
		} else {
            XBH_DEBUG("'r'eset 'c'ontrol 'f'ail sent\n");
            memcpy(output, XBH_CMD[XBH_CMD_rcf], XBH_COMMAND_LEN);
			return (uint16_t) XBH_COMMAND_LEN;
		}
	}/*}}}*/

	if ( (0 == memcmp(XBH_CMD[XBH_CMD_sar],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 's'tart 'a'pplication 'r'equest received\n");
		resetTimer=resetTimerBase*2;		
		ret=XBH_HandleStartApplicationRequest();
		resetTimer=0;
		
		if(0 == ret)
		{
            XBH_DEBUG("'s'tart 'a'pplication 'o'kay sent\n");
            memcpy(output, XBH_CMD[XBH_CMD_sao], XBH_COMMAND_LEN);
			return (uint16_t) XBH_COMMAND_LEN;
		} else {
            XBH_DEBUG("'s'tart 'a'pplication 'f'ail sent\n");
            memcpy(output, XBH_CMD[XBH_CMD_saf], XBH_COMMAND_LEN);
			return (uint16_t) XBH_COMMAND_LEN;
		}
	}/*}}}*/

	if ( (0 == memcmp(XBH_CMD[XBH_CMD_sur],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 's'tack 'u'sage 'r'equest received\n");
		resetTimer=resetTimerBase*1;
		ret=XBH_HandleStackUsageRequest(&output[XBH_COMMAND_LEN]);
		resetTimer=0;
		
		if(0 == ret) {
            XBH_DEBUG("'s'tack 'u'sage 'o'kay sent\n");
            memcpy(output, XBH_CMD[XBH_CMD_suo], XBH_COMMAND_LEN);
			return (uint16_t) XBH_COMMAND_LEN+2*NUMBSIZE;
		} else {
            XBH_DEBUG("'s'tack 'u'sage 'f'ail sent\n");
            memcpy(output, XBH_CMD[XBH_CMD_suf], XBH_COMMAND_LEN);
			return (uint16_t) XBH_COMMAND_LEN;
		}
	}/*}}}*/

	if ( (0 == memcmp(XBH_CMD[XBH_CMD_sbr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 's'tart 'b'ootloader 'r'equest received\n");
		resetTimer=resetTimerBase*2;	
		ret=XBH_HandleStartBootloaderRequest();
		resetTimer=0;
		
		if(0 == ret) {
            XBH_DEBUG("'s'tart 'b'ootloader 'o'kay sent\n");
            memcpy(output, XBH_CMD[XBH_CMD_sbo], XBH_COMMAND_LEN);
			return (uint16_t) XBH_COMMAND_LEN;
		} else {
            XBH_DEBUG("'s'tart 'b'ootloader 'f'ail sent\n");
            memcpy(output, XBH_CMD[XBH_CMD_sbf], XBH_COMMAND_LEN);
			return (uint16_t) XBH_COMMAND_LEN;
		}
	}/*}}}*/


	if ( (0 == memcmp(XBH_CMD[XBH_CMD_ccr],input,XBH_COMMAND_LEN)) ) {/*{{{*/
        XBH_DEBUG("Proper 'c'hecksum 'c'alc 'r'equest received\n");
		resetTimer=resetTimerBase*60;
		ret=XBH_HandleChecksumCalcRequest();
		resetTimer=0;

		if(0 == ret) {
            XBH_DEBUG("Handle 'c'hecksum 'c'alc 'o'kay sent\n");
            memcpy(output, XBH_CMD[XBH_CMD_cco], XBH_COMMAND_LEN);
			return (uint16_t) XBH_COMMAND_LEN;
		} else {
            XBH_DEBUG("Handle 'c'hecksum 'c'alc 'f'ail sent\n");
            memcpy(output, XBH_CMD[XBH_CMD_ccf], XBH_COMMAND_LEN);
			return (uint16_t) XBH_COMMAND_LEN;
		}
	}/*}}}*/

#endif
    memcpy(reply, XBH_CMD[XBH_CMD_unk], XBH_COMMAND_LEN);
    XBH_DEBUG("'un'known command 'r'eceived (len: %d)\n", input_len);
    for(uint8_t u=0;u<input_len;++u) {
        if(u % 32 == 0){ 
            XBH_DEBUG("\n");
        }
        XBH_DEBUG("%x",input[u]);
    }
    XBH_DEBUG("\n");
	
	return (uint16_t) XBH_COMMAND_LEN;
}/*}}}*/
