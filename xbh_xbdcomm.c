#include <util/delay.h>

#include "xbh_xbdcomm.h"
#include "xbd_multipacket.h"
#include "i2c.h"
#include "usart.h"
#include "xbh_utils.h"
#include "config.h"
#include "stack.h"

#define I2C_BAUDRATE 400
//Address must be shifted in by 1
#define SLAVE_ADR (0x75<<1)

#define UART_COMM_BAUDRATE 115200
#define UART_OVERDRIVE_BAUDRATE 250000
#define UART_DEBUG_BAUDRATE 115200

#define UDP_CONN_WRITEOP	'W'
#define UDP_CONN_READOP		'R'
#define UDP_CONN_NOOP		'0'

//#define XBD_UDP_DEBUG


//const char XBHcrcFail[] PROGMEM = "XBHcrcFL";

u32 xbd_ip=DEFAULT_XBD_IP;
unsigned char * xbd_ip_bytes=(unsigned char *)&xbd_ip;
#define XBD_PORT 22596

unsigned char xbd_comm=COMM_I2C;


unsigned char ce_state=CE_IDLE;
unsigned char ce_seqnum=0;
unsigned char ce_last_ackd=0;
unsigned char ce_timeouts=0;
unsigned long ce_timeout_at=0;


volatile unsigned char udp_conn_lastop = UDP_CONN_NOOP;

extern uint8_t I2cReceiveData[I2C_RECEIVE_DATA_BUFFER_SIZE];

void inc_ce_seqnum()
{
	++ce_seqnum;
	if(0 == ce_seqnum) ce_seqnum=1;
}


uint8_t usart_read_char(uint8_t i)
{
  int receive_char;

  loop_until_bit_is_set(UCSR0A ,RXC0);
  receive_char = (UDR0);

  return receive_char;
}

void xbdCommUdpGet(unsigned char index)
{
	unsigned int ip_byte[4];
	struct UDP_Header *udp;
	struct IP_Header  *ip;
	unsigned int src_port;
	u16 *p_crc;

	u16 udp_data_len = (UDP_DATA_END_VAR)-(UDP_DATA_START);


	udp = (struct UDP_Header *)&eth_buffer[UDP_OFFSET];
	ip  = (struct IP_Header  *)&eth_buffer[IP_OFFSET];
	src_port = NTOHS((udp->udp_SrcPort));
	
	if((ip->IP_Srcaddr != xbd_ip) ||(src_port != XBD_PORT))
	{	
		ip_byte[0]=(ip->IP_Srcaddr)&0xff;
		ip_byte[1]=((ip->IP_Srcaddr)>>8)&0xff;
		ip_byte[2]=((ip->IP_Srcaddr)>>16)&0xff;
		ip_byte[3]=((ip->IP_Srcaddr)>>24)&0xff;	
		XBH_ERROR("xbdCommUdpGet srcPort: %i *\n",src_port);
		XBH_ERROR("xbdCommUdpGet SrcIP: %i.%i.%i.%i *\n",ip_byte[0],ip_byte[1],ip_byte[2],ip_byte[3]);	
		return;
	}



#ifdef XBD_UDP_DEBUG
	XBH_DEBUG("xbdCommUdpGet Bytes: %\n",udp_data_len);
	XBD_debugOutBuffer("xbdCommUdpGet data", &eth_buffer[UDP_DATA_START],udp_data_len);
#endif

	switch(udp_conn_lastop)
	{
		case UDP_CONN_WRITEOP:
			#ifdef XBD_UDP_DEBUG
					XBH_DEBUG("UDP_CONN_WRITEOP, answer len=%d\n",udp_data_len);
			#endif

			switch(ce_state)
			{
				case CE_ACK_WAIT:
				//XBH_DEBUG("WR len=\n");
				if(	//UDP ACK Rec'd
						(0xAC == eth_buffer[UDP_DATA_START]) &&
						(ce_seqnum == eth_buffer[UDP_DATA_START+1]) &&
						(2==udp_data_len)	
					)
				{
					//XBH_DEBUG("WR ce_state=CE_ANSWER_WAIT\n");
					inc_ce_seqnum();
					ce_timeouts=0;
					ce_timeout_at=time+CE_TIMEOUT_SECS;
					ce_state=CE_ANSWER_WAIT;
				}
				else
				{
					XBH_WARN("WR case CE_ACK_WAIT els\n");
					XBH_WARN("Bytes: %\n",udp_data_len);
					XBD_debugOutBuffer("WR case CE_ACK_WAIT else data", &eth_buffer[UDP_DATA_START],udp_data_len);
					//In case the XBD has not seen my last UDP ack:
					if(ce_last_ackd == eth_buffer[UDP_DATA_START+1])
					{
						XBH_WARN("WR AW ACK re-tx\n");
						// Send the UDP ACK 
						eth_buffer[UDP_DATA_START]=0xAC;
						create_new_udp_packet(	(unsigned int) 2,
																		(unsigned int)  XBD_PORT,
																		(unsigned int)  XBD_PORT,
																		(unsigned long) xbd_ip);					
					}
					else
					{
					        // this may happen after an reset if xbd did not reset and has data waiting
						XBH_WARN("WR AW ACK unknown seq nu\n");
						// Send the UDP ACK anyway
						eth_buffer[UDP_DATA_START]=0xAC;
						create_new_udp_packet(	(unsigned int) 2,
																		(unsigned int)  XBD_PORT,
																		(unsigned int)  XBD_PORT,
																		(unsigned long) xbd_ip);
					}
				}
				break;

				case CE_ANSWER_WAIT:
				if(	//StreamComm 'A'CK Rec'd
						('D' == eth_buffer[UDP_DATA_START]) &&
						('A' == eth_buffer[UDP_DATA_START+2]) &&
						(3==udp_data_len)
                                  )
				{							
                                        //XBH_DEBUG("WR AnswW 'A' ACKe\n");
                                        if((ce_last_ackd==eth_buffer[UDP_DATA_START+1]) && (0 !=ce_last_ackd) )
                                                XBH_WARN(" but it was already ACKe\n");
                                        // Send the UDP ACK and set CE state machine to idle
                                        eth_buffer[UDP_DATA_START]=0xAC;
                                        ce_last_ackd=eth_buffer[UDP_DATA_START+1];					
                                        create_new_udp_packet(	(unsigned int) 2,
                                                                                                        (unsigned int)  XBD_PORT,
                                                                                                        (unsigned int)  XBD_PORT,
                                                                                                        (unsigned long) xbd_ip);
                                        ce_state=CE_IDLE;
                                        udp_conn_lastop = UDP_CONN_NOOP;
                                }
                                else
                                {
                                        XBH_ERROR("WR case CE_ANSWER_WAIT els\n");
                                        XBH_ERROR("Bytes: %\n",udp_data_len);
                                        XBD_debugOutBuffer("WR case CE_ANSWER_WAIT else data", &eth_buffer[UDP_DATA_START],udp_data_len);
                                }
        			break;

				case CE_IDLE:
				#ifdef XBD_UDP_DEBUG
					XBH_DEBUG("WR case CE_IDLE: Bytes: %\n",udp_data_len);
					XBD_debugOutBuffer("xbdCommUdpGet data", &eth_buffer[UDP_DATA_START],udp_data_len);
				#endif
				
                                        // TODO: Remove this block, it may not be necessary anymore
                                        XBH_WARN("WR IDLE ACK re-tx\n");
                                        // Send the UDP ACK 
                                        eth_buffer[UDP_DATA_START]=0xAC;
                                        ce_last_ackd=eth_buffer[UDP_DATA_START+1];				
                                        create_new_udp_packet(  (unsigned int) 2,
                                                                (unsigned int)  XBD_PORT,
                                                                (unsigned int)  XBD_PORT,
                                                                (unsigned long) xbd_ip);
				break;

				default:
				XBH_ERROR("\nBad ce_state [%d] at xbdCommUdpGet WO!\n",ce_state);
				ce_state=CE_FAILURE;
			}//switch(ce_state)
		break;//case UDP_CONN_WRITEOP:
		
		case UDP_CONN_READOP:
		#ifdef XBD_UDP_DEBUG
				  XBH_DEBUG("UDP_CONN_READOP, len=%d\n",udp_data_len);
		#endif
		if((udp_data_len-2) > I2C_RECEIVE_DATA_BUFFER_SIZE) {
			XBH_ERROR("Received packet too large for buffer! (%d bytes\n",udp_data_len);
			XBD_debugOutBuffer("too large packet", &eth_buffer[UDP_DATA_START],udp_data_len);
		}
		else
		{		
			switch(ce_state)
			{
				case CE_ACK_WAIT:
				if(	//UDP ACK Rec'd
						(0xAC == eth_buffer[UDP_DATA_START]) &&
						(ce_seqnum == eth_buffer[UDP_DATA_START+1]) &&
						(2==udp_data_len)	
					)
				{
					inc_ce_seqnum();
					ce_timeouts=0;
					ce_timeout_at=time+CE_TIMEOUT_SECS;
					ce_state=CE_ANSWER_WAIT;
					//XBH_DEBUG("RE ce_state=CE_ANSWER_WAIT\n");
				}
				else
				{
					XBH_WARN("RE case CE_ACK_WAIT else ");
					XBH_WARN("Bytes: %\n",udp_data_len);
					XBD_debugOutBuffer("RE case CE_ACK_WAIT else data", &eth_buffer[UDP_DATA_START],udp_data_len);
					
					XBH_WARN("RE AW ACK re-tx\n");
					//In case the XBD has not seen my last UDP ack:
					if(ce_last_ackd == eth_buffer[UDP_DATA_START+1])
					{
					// Re-Send the UDP ACK 
					eth_buffer[UDP_DATA_START]=0xAC;
					create_new_udp_packet(	(unsigned int) 2,
																	(unsigned int)  XBD_PORT,
																	(unsigned int)  XBD_PORT,
																	(unsigned long) xbd_ip);					
					}
					else
					{
						XBH_ERROR("RD AW ACK Arsc\n");
						// Re-Send the UDP ACK anyway
						eth_buffer[UDP_DATA_START]=0xAC;
						create_new_udp_packet(	(unsigned int) 2,
																		(unsigned int)  XBD_PORT,
																		(unsigned int)  XBD_PORT,
																		(unsigned long) xbd_ip);
					}			
				}
				break;

				
				case CE_ANSWER_WAIT:				
				if(	//StreamComm 'A'CK Rec'd
						('D' == eth_buffer[UDP_DATA_START]) &&
						(3<=udp_data_len)
					)
				{

					memcpy(I2cReceiveData,&eth_buffer[UDP_DATA_START+2],udp_data_len-2);

					//Check CRC
					p_crc=(u16 *)&eth_buffer[UDP_DATA_START+udp_data_len-CRC16SIZE];
				  if( ! (crc16check(&eth_buffer[UDP_DATA_START+2], udp_data_len-2-CRC16SIZE, p_crc)) )
					{
						XBD_debugOutBuffer("RE CE_ANSWER_WAIT wrong CRC at ", &eth_buffer[UDP_DATA_START], udp_data_len);
						break;
					}

					// Send the UDP ACK and set CE state machine to idle
					eth_buffer[UDP_DATA_START]=0xAC;
					ce_last_ackd=eth_buffer[UDP_DATA_START+1];										
					create_new_udp_packet(	(unsigned int) 2,
													(unsigned int)  XBD_PORT,
													(unsigned int)  XBD_PORT,
													(unsigned long) xbd_ip);
					ce_state=CE_IDLE;
					udp_conn_lastop = UDP_CONN_NOOP;
					//XBH_DEBUG("RE ce_state=CE_IDLE\n");
				}
				else
				{
					XBH_WARN("RE case CE_ANSWER_WAIT els\n");
					XBH_WARN("Bytes: %\n",udp_data_len);
					XBD_debugOutBuffer("RE case CE_ANSWER_WAIT else data", &eth_buffer[UDP_DATA_START],udp_data_len);
					XBH_ERROR("RD AnswW ACK Arsc\n");
				}
				break;

				
				case CE_IDLE:
				#ifdef XBD_UDP_DEBUG
				XBH_DEBUG("RE case CE_IDLE: Bytes: %\n",udp_data_len);
				XBD_debugOutBuffer("xbdCommUdpGet data", &eth_buffer[UDP_DATA_START],udp_data_len);
				#endif
				XBH_WARN("WR IDLE ACK re-tx\n");
				// Send the UDP ACK 
				eth_buffer[UDP_DATA_START]=0xAC;				
				ce_last_ackd=eth_buffer[UDP_DATA_START+1];	
				create_new_udp_packet(  (unsigned int) 2,
												(unsigned int)  XBD_PORT,
												(unsigned int)  XBD_PORT,
												(unsigned long) xbd_ip);
				break;
			
			
				default:
				XBH_ERROR("\nBad ce_state [%d] at xbdCommUdpGet RO!\n",ce_state);
				ce_state=CE_FAILURE;
			}//switch(ce_state)
		
		}//else 		    
		break;//case UDP_CONN_READOP:

		case UDP_CONN_NOOP:
		  XBH_WARN("UDP_CONN_NOOP, len=%d\n",udp_data_len);
			//In case the XBD has not seen my last UDP ack:
			if(	(ce_last_ackd == eth_buffer[UDP_DATA_START+1]) &&
					('D' == eth_buffer[UDP_DATA_START]))
			{
			XBH_WARN("NOOP ACK re-tx\n");
			// Send the UDP ACK 
			eth_buffer[UDP_DATA_START]=0xAC;
			create_new_udp_packet(	(unsigned int) 2,
															(unsigned int)  XBD_PORT,
															(unsigned int)  XBD_PORT,
															(unsigned long) xbd_ip);
			}	
		break;
		
	}//switch(udp_conn_lastop)
}




void xbdCommExit()
{
  switch(xbd_comm) {
  case COMM_I2C:
    i2cExit();
    break;
  case COMM_UART:
  case COMM_UART_OVERDRIVE:
	case COMM_ETHERNET:
	break;
  }

}

void xbdCommUdpInit()
{
	static uint8_t app_added=0;
	
	//XBH_DEBUG("\nce_state=CE_IDLE @ xbdCommUdpInit(\n");
	ce_state=CE_IDLE;
	if(!app_added)
	{
		app_added=1;
		add_udp_app (XBD_PORT, (void(*)(unsigned char))xbdCommUdpGet);
		(*((unsigned long*)&xbd_ip_bytes[0])) = get_eeprom_value(XBDIP_EEPROM_STORE,DEFAULT_XBD_IP);
	}
}




void xbdCommInit(uint8_t commMode)
{
  xbd_comm=commMode;
  switch(xbd_comm) {
  case COMM_I2C:
    i2cInit();
    i2cSetLocalDeviceAddr(SLAVE_ADR+1, 0);
    i2cSetBitrate(I2C_BAUDRATE);
    XBH_DEBUG("I2C");
    usart_init(UART_DEBUG_BAUDRATE);
    usart_status.usart_ignore = 0;
    _delay_ms(100);
    XBH_DEBUG("...I2\n");
    break;
  case COMM_UART:
    usart_init(UART_COMM_BAUDRATE);
    usart_status.usart_ignore = 1;
    break;
  case COMM_UART_OVERDRIVE:
    usart_init(UART_OVERDRIVE_BAUDRATE);
    usart_status.usart_ignore = 1;
    break;
  case COMM_ETHERNET:
    usart_init(UART_DEBUG_BAUDRATE);
    usart_status.usart_ignore = 0;
    XBH_DEBUG("\nEthernet XB\n");
    xbdCommUdpInit();
    break;

  default:
    XBH_ERROR("HURTZ!\n");
  }
}

void xbdSend(uint8_t length, uint8_t *buf) {
  int i;
  unsigned char ch;
	u16 *p_crc;

	//generate CRC16
	p_crc=(u16 *)&buf[length];
  crc16create(buf, length, p_crc);
	length+=CRC16SIZE;

  switch(xbd_comm) {
  case COMM_I2C:
    i2cMasterSendNI(SLAVE_ADR, length, buf);
    break;
  case COMM_UART:
  case COMM_UART_OVERDRIVE:
    usart_write_char(0);
    usart_write_char(0);
    usart_write_char(0);
    usart_write_char(0);
    usart_write_char('W');
    usart_write_char(length);
    for(i=0;i<length;i++) usart_write_char(buf[i]);
    ch=usart_read_char(0);
    if('A' != ch) XBH_ERROR("ACK was not an 'A\n");
    break;
  case COMM_ETHERNET:
	
	//XBH_DEBUG("\nce_state=CE_ACK_WAI\n");
	ce_state=CE_ACK_WAIT;
	ce_timeouts=0;

  SWITCH_TO_SMALL_BUFFER;
	
	eth_buffer[UDP_DATA_START]='D';
	eth_buffer[UDP_DATA_START+1]=ce_seqnum;
	eth_buffer[UDP_DATA_START+2]='W';
	memcpy(&eth_buffer[UDP_DATA_START+3],buf,length);

#ifdef XBD_UDP_DEBUG 

	XBD_debugOutBuffer("udp_data", &eth_buffer[UDP_DATA_START],length+3);
	XBH_DEBUG("----------UDP send-----------\n");		
#endif

	create_new_udp_packet(  (unsigned int)  length+3,
	               		 	(unsigned int)  XBD_PORT,
	                		(unsigned int)  XBD_PORT,
		                	(unsigned long) xbd_ip);
	

	ce_timeout_at=time+CE_TIMEOUT_SECS;
	udp_conn_lastop = UDP_CONN_WRITEOP;
	do
	{
		eth_get_data();	//Hast Du keinen, pump Dir einen!

		switch(ce_state)
		{
			case CE_ACK_WAIT:
			if(ce_timeout_at<=time)
			{
				eth_buffer[UDP_DATA_START]='D';
				eth_buffer[UDP_DATA_START+1]=ce_seqnum;
				eth_buffer[UDP_DATA_START+2]='W';
				memcpy(&eth_buffer[UDP_DATA_START+3],buf,length);

				//XBH_DEBUG("\n*** ce_timeout **\n");
				#ifdef XBD_UDP_DEBUG

					XBD_debugOutBuffer("CE_ACK_WAIT udp_data", &eth_buffer[UDP_DATA_START],length+3);
					XBH_DEBUG("----------UDP retransmit-----------\n");		
				#endif				

				create_new_udp_packet(  (unsigned int)  length+3,
				               		 	(unsigned int)  XBD_PORT,
				                		(unsigned int)  XBD_PORT,
					                	(unsigned long) xbd_ip);
				++ce_timeouts;
				ce_timeout_at=time+CE_TIMEOUT_SECS;
			}
			if(ce_timeouts >= CE_MAX_TIMEOUTS)
			{
				XBH_WARN("ce_state=CE_FAILURE, timeout limi\n");
				ce_state=CE_FAILURE;
				udp_conn_lastop = UDP_CONN_NOOP;	
			}
			break;

			case CE_ANSWER_WAIT:
			break;

			case CE_IDLE:
			udp_conn_lastop = UDP_CONN_NOOP;
			break;

			default:
			XBH_ERROR("\nBad ce_state [%d] at xbdSend!\n",ce_state);
			ce_state=CE_FAILURE;
			udp_conn_lastop = UDP_CONN_NOOP;	
		
		}//end of switch(ce_state)
	
	}// end of do
	while(udp_conn_lastop != UDP_CONN_NOOP);
  
	SWITCH_TO_LARGE_BUFFER;

	break;
  }
}


void xbdReceive(uint8_t length, uint8_t *buf) {
  int i;
	u16 *p_crc;
  //XBH_DEBUG("XBD: Waiting for %d byte\n",length);

	length+=CRC16SIZE;

  switch(xbd_comm) {
  case COMM_I2C:
    i=i2cMasterReceiveNI(SLAVE_ADR, length, buf);
    if( i != 0 ) {
      XBH_ERROR("I2C Receive error: %d\n",i);
    }
    break;
    case COMM_UART:
    case COMM_UART_OVERDRIVE:
    usart_write_char(0);
    usart_write_char(0);
    usart_write_char(0);
    usart_write_char(0);
    usart_write_char('R');
    usart_write_char(length);
    for(i=0;i<length;i++) {
      buf[i]=usart_read_char(i);
    }
    break;

	case COMM_ETHERNET:

	//XBH_DEBUG("\nce_state=CE_ACK_WAI\n");
	ce_state=CE_ACK_WAIT;
	ce_timeouts=0;

	SWITCH_TO_SMALL_BUFFER;

	eth_buffer[UDP_DATA_START]='D';
	eth_buffer[UDP_DATA_START+1]=ce_seqnum;	  
	eth_buffer[UDP_DATA_START+2]='R';
	eth_buffer[UDP_DATA_START+3]=length;

#ifdef XBD_UDP_DEBUG
	XBD_debugOutBuffer("COMM_ETHERNET udp_data", &eth_buffer[UDP_DATA_START],4);
	XBH_DEBUG("----------UDP receive-----------\n");		
#endif

	create_new_udp_packet(  (unsigned int)  4,
	               		 	(unsigned int)  XBD_PORT,
	                		(unsigned int)  XBD_PORT,
		                	(unsigned long) xbd_ip);
	
/*
	udp_conn_lastop = UDP_CONN_READOP;
	do
	{

		eth_get_data();	//Hast Du keinen, pump Dir einen!

	}
	while(udp_conn_lastop != UDP_CONN_NOOP);
*/

	ce_timeout_at=time+CE_TIMEOUT_SECS;
	udp_conn_lastop = UDP_CONN_READOP;
	do
	{
		eth_get_data();	//Hast Du keinen, pump Dir einen!

		switch(ce_state)
		{
			case CE_ACK_WAIT:
			if(ce_timeout_at<=time)
			{
				eth_buffer[UDP_DATA_START]='D';
				eth_buffer[UDP_DATA_START+1]=ce_seqnum;	  
				eth_buffer[UDP_DATA_START+2]='R';
				eth_buffer[UDP_DATA_START+3]=length;

				//XBH_DEBUG("\n*** ce_timeout **\n");
				#ifdef XBD_UDP_DEBUG	
					#endif

					XBD_debugOutBuffer("CE_ACK_WAIT udp_data", &eth_buffer[UDP_DATA_START],4);
					XBH_DEBUG("----------UDP retransmit-----------\n");		
				

				create_new_udp_packet(  (unsigned int)  4,
				               		 	(unsigned int)  XBD_PORT,
				                		(unsigned int)  XBD_PORT,
					                	(unsigned long) xbd_ip);
				++ce_timeouts;
				ce_timeout_at=time+CE_TIMEOUT_SECS;
			}
			if(ce_timeouts >= CE_MAX_TIMEOUTS)
			{
				XBH_ERROR("\nce_state=CE_FAILURE, timeout limi\n");
				ce_state=CE_FAILURE;
				udp_conn_lastop = UDP_CONN_NOOP;	
			}
			break;

			case CE_ANSWER_WAIT:
			break;
			
			case CE_IDLE:
			break;

			default:
			XBH_ERROR("\nBad ce_state [%d] at xbdSend!\n",ce_state);
			ce_state=CE_FAILURE;
			udp_conn_lastop = UDP_CONN_NOOP;	
		
		}//end of switch(ce_state)
	
	}// end of do
	while(udp_conn_lastop != UDP_CONN_NOOP);
 


#ifdef XBD_UDP_DEBUG
	XBD_debugOutBuffer("udp_data from XBD", I2cReceiveData, length);	  
#endif
	memcpy(buf,I2cReceiveData,length);

	SWITCH_TO_LARGE_BUFFER;
  break;
  }

	//Check CRC
	p_crc=(u16 *)&buf[length-CRC16SIZE];

    if( ! (crc16check(buf, length-CRC16SIZE, p_crc)) ) {
		XBD_debugOutBuffer("wrong CRC", buf, length);
		memcpy(buf, XBD_CMD[XBD_CMD_ccf], XBD_COMMAND_LEN);
		return;
	}

}
