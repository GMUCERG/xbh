#include <util/crc16.h>
#include <util/delay.h>
#include "xbh_utils.h"
#include "usart.h"
#include "xbh_xbdcomm.h"
#include "avrlibtypes.h"

extern unsigned char xbd_comm;
extern volatile unsigned long time;

void xbh_debug_time()
{
    unsigned char hh = (time/3600)%24;
    unsigned char mm = (time/60)%60;
    unsigned char ss = time %60;
    xbh_debug_write("TIME: %2i:%2i:%2i ",hh,mm,ss);
}

void XBD_debugOutHex32Bit(u32 toOutput)
{
	char buf[9];
	ltoa(toOutput,buf,16);
	
	switch (xbd_comm)	
	{
		case COMM_I2C:
	  case COMM_ETHERNET:
			usart_write_str(buf);
		break;
	}
}

void xbh_debug_writeP(char *format, ...)
{
	va_list args;
	//LED_RED(1);

	switch (xbd_comm)	{
		case COMM_I2C:
	    case COMM_ETHERNET:
			va_start( args, format );
			//usart_write_P("\r");
			usart_vwrite_P(format, &args );
			va_end(args);
	 	break;
	 	case COMM_UART_OVERDRIVE:
		case COMM_UART:
			/* use lcd */
/*			va_start(args,format);
			if(lcd_row==3) {
				lcd_clear();
				lcd_row=0;
			}
			lcd_vprint_P(lcd_row,0,format, &args);
			lcd_row++;
			va_end(args);
*/		break;
		default:
		break;
	}
    //LED_RED(0);
}


u16 crc16buffer(u08 *data, u16 len)
{
	u16 crc = 0xffff, ctr;

	for (ctr = 0; ctr < len; ++ctr) {
		crc = _crc16_update(crc, data[ctr]);
	}
	return crc;
}

void crc16create(u08 *data, u16 len, u16 *p_dest) {
    *p_dest = HTONS16(crc16buffer(data, len));
}

u08 crc16check(u08 *data, u16 len, u16 *p_crc) {
    u16 crc = crc16buffer(data, len);
    if (crc != NTOHS16(*p_crc)) {
            XBH_WARN("->CRC error! Rec=%x Calc=%x\n", NTOHS16(*p_crc), crc);
            return 0;
    }
    return 1;
}

//TODO Implement
void emergencyXBHandXBDreset()
{
	XBH_WARN("\nXBD and XBH RESET!!!");
	//Disable pull-up in input, set to low level if output
	PORTB &= ~(_BV(PB0));
	//set DDR bit -> output
	DDRB |= _BV(PB0);
	_delay_ms(500);
	//clear DDR bit -> input
	DDRB &=  ~(_BV(PB0));
	//reset XBH
	wdt_enable(WDTO_250MS);
	while(1);
}

void XBD_debugOutBuffer(char *name, uint8_t *buf, uint32_t len)
{
    uint16_t ctr;
	char obuf[3];
	obuf[2]=0;

        XBH_DEBUG("\n--- ");
        XBH_DEBUG("%s",name);
        XBH_DEBUG(" ---:");
	

        for (ctr = 0; ctr < len; ++ctr) {
                if (0 == ctr % 16)
                        XBH_DEBUG("\n");
		obuf[0]=ntoa(buf[ctr]/16);
		obuf[1]=ntoa(buf[ctr]%16);
		XBH_DEBUG("%s",obuf);
        }

	for (ctr = 0; ctr < len; ++ctr) {
  	if (0 == ctr % 16)
    	XBH_DEBUG("\n");
		
		if( (32 <= buf[ctr]) && (127 >= buf[ctr]) )
			XBH_DEBUG("%c ",buf[ctr]);
		else
			XBH_DEBUG("? ",buf[ctr]);
  }

    XBH_DEBUG("\n--------");
}
