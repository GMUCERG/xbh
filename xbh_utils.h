#ifndef _XBH_UTILS_H
#define _XBH_UTILS_H

#include "global.h"
#include "stack.h"
#include <avr/pgmspace.h>


extern unsigned char xbh_eth_buffer[MTU_SIZE+1];
extern unsigned char xbd_eth_buffer[SMALL_ETH_BUFFER_SIZE+1];
extern unsigned int  eth_buffer_size;


#define SWITCH_TO_LARGE_BUFFER { eth_buffer=xbh_eth_buffer;	eth_buffer_size=ORIGINAL_ETH_BUFFER_SIZE; }
#define SWITCH_TO_SMALL_BUFFER { eth_buffer=xbd_eth_buffer;	eth_buffer_size=SMALL_ETH_BUFFER_SIZE; }
#define IS_LARGE_BUFFER (eth_buffer_size==ORIGINAL_ETH_BUFFER_SIZE)


#define XBH_DEBUG xbh_debug_time
#define XBH_WARN  xbh_warn_write
#define XBH_ERROR xbh_error_write

#define xbh_debug_write(format, args...) { xbh_debug_time(); LWIP_DEBUGF(LWIP_XBH_DEBUG|LWIP_DBG_TRACE, (format, ## args));}
#define xbh_warn_write(format, args...) { xbh_debug_time(); LWIP_DEBUGF(LWIP_DBG_ON|LWIP_DBG_TRACE, ("warning: "format, ## args));}
#define xbh_error_write(format, args...) { xbh_debug_time(); LWIP_DEBUGF(LWIP_DBG_ON|LWIP_DBG_TRACE, ("ERROR: "format, ## args));}

void xbh_debug_time();
void xbh_debug_writeP(char * format, ...);
void XBD_debugOutHex32Bit(u32 toOutput);
/** outputs a buffer as hex blocks to the debug interface of the device 
 * @param name The name of the buffer, printed before the actual buffer output
 * @param buf The buffer
 * @param len The length of the buffer
 */
void XBD_debugOutBuffer(char *name, uint8_t *buf, uint16_t len);



inline char *constStringToBuffer(char *dest, const char * src)
{
  return strcpy((char *) dest, src);
}

inline char *constStringToBufferN(char *dest, const car * src, size_t n)
{
  return strncpy((char *) dest, src, n);
}



//inline u08 htoi(char h)
//{
//	if(h >= '0' && h <= '9')
//		return (h-'0');
//
//	if(h>= 'a' && h <= 'f')
//		return (h-'a'+10);
//
//	if(h>= 'A' && h <= 'F')
//		return (h-'A'+10);
//
//	return 255;
//}
//
//inline char ntoa(u08 n)
//{
//	if(n<=9)
//		return n+'0';
//	else
//		return n+'A'-10;
//}


u16 crc16buffer(u08 *data, u16 len);
void crc16create(u08 *data, u16 len, u16 *dest);
u08  crc16check(u08 *data, u16 len, u16 *crc);

void emergencyXBHandXBDreset();


#endif /* _XBH_UTILS_H */
