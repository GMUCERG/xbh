#include "tcp_xbh.h"
#include "stack.h"
#include "xbh.h"
#include "xbh_utils.h"

#define XBX_TCPBUF_SIZE 600


// #define XBH_TCP_DEBUG

#ifdef XBH_TCP_DEBUG
  #define DEBUG_APPSTATUS XBH_DEBUG("APP STATUS IS %d\r\n",tcp_entry[index].app_status)
  #define DEBUG_TIME  xbh_debug_time()

#else
  #define DEBUG_APPSTATUS
  #define DEBUG_TIME
#endif


/** 0= no connection, 1=connection open*/
u08 tcp_xbh_state;
u08 tcp_conn_index;

/** 0=no ack expected, 1= waiting for ack */
u08 tcp_xbh_ackwait;

u08 xbh_tcp_databuf[XBX_TCPBUF_SIZE];
u16 xbh_tcp_lastlen=0;



//Initialisierung des telnetd Testservers
void tcp_xbh_init (void)
{
        //Serverport und Anwendung in TCP Anwendungsliste eintragen
        add_tcp_app (TCP_XBH_PORT, (void(*)(unsigned char))tcp_xbh_get);
}


void tcp_xbh_get (unsigned char index)
{
    static u32 seqn_number=0;
    static u32 ack_number=0;
    u16 tcp_data_len = (TCP_DATA_END_VAR)-(TCP_DATA_START_VAR);
    DEBUG_APPSTATUS;



/*
    XBH_DEBUG("*XBH TCP GET Bytes: %i [",tcp_data_len);
    for(ret=0;ret<tcp_data_len;++ret)
    {
            XBH_DEBUG("%c",eth_buffer[TCP_DATA_START_VAR+ret]);
    }
    XBH_DEBUG("]\r\n");
*/
    //Verbindung wurde abgebaut!
    if (tcp_entry[index].status & FIN_FLAG)
    {
        DEBUG_TIME;
	XBH_DEBUG("TCP close\r\n");
	if (index != tcp_conn_index ) {
	  XBH_WARN("Ignoring tcp close - not on main connectio\r\n");
	  return;
	}
	if ( tcp_xbh_state != 1) {
	  // this is common after an xbh reset
          XBH_WARN("Connection was never open\r\n");
          return;
	}
	
	
        tcp_xbh_state=0;
	tcp_xbh_ackwait=0;
        tcp_entry[index].time = TCP_TIME_OFF;
        seqn_number=0;
        ack_number=0;
        return;
    }

        // Verbindungsaufbau
        if (tcp_entry[index].app_status == 0 || tcp_entry[index].app_status == 1)
        {
                if ( 1 == tcp_xbh_state ) {
                  XBH_WARN("Second connection attempted, discardin\r\n");
                  tcp_index_del(index);
                  return;
                }
                tcp_xbh_state=1;
                DEBUG_TIME;
                XBH_DEBUG("TCP open\r\n");
                tcp_entry[index].app_status = 2;
                tcp_xbh_ackwait = 0;
                tcp_entry[index].time = TCP_TIME_OFF;
                tcp_conn_index = index;
                seqn_number=0;
                ack_number=0;
        }
        
        if (index != tcp_conn_index ) {
	  XBH_ERROR("TCP packet not on main connection (was %d, main %d)",index, tcp_conn_index);
	  return;
	}

        
        if ( tcp_xbh_state != 1) {
          XBH_ERROR("Connection established assertion failed: '%d\r\n",tcp_xbh_state);
          return;
        }

        //Ack erhalten vom gesendeten Packet
        if ((tcp_entry[index].app_status > 1)&&(tcp_entry[index].status&ACK_FLAG)&&(tcp_xbh_ackwait))
        {
               if(ack_number == tcp_entry[index].ack_counter)
                {
                        DEBUG_TIME;
                        XBH_WARN("DUPLICATE ACK: ");
                        XBD_debugOutHex32Bit(ack_number);
                        XBH_DEBUG("\r\n");
                        return;
                }
                ack_number=tcp_entry[index].ack_counter;
        
                tcp_entry[index].time = TCP_TIME_OFF;
                tcp_xbh_ackwait = 0;
                DEBUG_TIME;
#ifdef TCP_XBH_DEBUG
                XBH_DEBUG("ACK seen\r\n");
#endif
                tcp_entry[index].error_count=0;
                //return;
        }

        //Time out kein ack angekommen
        if (tcp_entry[index].status == 0)
        {
                if( 1 != tcp_xbh_ackwait ) {
                  XBH_ERROR("Timeout waiting for ack while not waiting for ack (%d\r\n",tcp_xbh_ackwait);
                  tcp_index_del(index);
                  return;
                }


						     //Daten nochmal senden
			       
								if(!IS_LARGE_BUFFER)
						    {
									XBH_DEBUG("###XBH TCP Re-Transmit with small ETH buffer!!! Switching...###\r\n");
									SWITCH_TO_LARGE_BUFFER;
     							memcpy(&eth_buffer[TCP_DATA_START],xbh_tcp_databuf,xbh_tcp_lastlen);
						      create_new_tcp_packet(xbh_tcp_lastlen,index);
									SWITCH_TO_SMALL_BUFFER;
								} else {
			            memcpy(&eth_buffer[TCP_DATA_START],xbh_tcp_databuf,xbh_tcp_lastlen);
	                create_new_tcp_packet(xbh_tcp_lastlen,index);
								}
                tcp_entry[index].time = 2;
                DEBUG_TIME;
                XBH_DEBUG("###XBH TCP Re-Transmit!###\r\n");
                return;
        }

        // received command
        if ((tcp_entry[index].app_status > 1) && 0==tcp_xbh_ackwait && tcp_data_len > 0) 
        {
                tcp_entry[index].app_status = 2;

                tcp_entry[index].status = ACK_FLAG;
                create_new_tcp_packet(0,index);

                if(seqn_number == tcp_entry[index].seq_counter)
                {
                        DEBUG_TIME;
                        XBH_DEBUG("DUPLICATE: ");
                        XBD_debugOutHex32Bit(seqn_number);
                        XBH_DEBUG("\r\n");
                        return;
                }
                seqn_number = tcp_entry[index].seq_counter;

                xbh_tcp_lastlen = XBH_handle(&eth_buffer[TCP_DATA_START_VAR], tcp_data_len, xbh_tcp_databuf);
                if (0 == xbh_tcp_lastlen) {
					XBH_ERROR("Unknown Request Rec'\r\n");
                }else if (XBX_TCPBUF_SIZE <= xbh_tcp_lastlen) {
					XBH_ERROR("XBX_TCPBUF_SIZE limit reached\r\n");	
				}
                memcpy(&eth_buffer[TCP_DATA_START],xbh_tcp_databuf,xbh_tcp_lastlen);
                create_new_tcp_packet(xbh_tcp_lastlen,index);
                tcp_xbh_ackwait=1;
                tcp_entry[index].time = 2;

#ifdef TCP_XBH_DEBUG
                XBH_DEBUG("Ende XBH TCP\r\n");
#endif
                return;
        }
}

