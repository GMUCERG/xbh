#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <unistd.h>

#include <lwip/sockets.h> 

#include "util.h"

//LTOA/*{{{*/
//TODO: Make version for large unsigned ints

/*
**  LTOA.C
**
**  Converts a long integer to a string.
**
**  Copyright 1988-90 by Robert B. Stout dba MicroFirm
**
**  Released to public domain, 1991
**
**  Parameters: 1 - number to be converted
**              2 - buffer in which to build the converted string
**              3 - number base to use for conversion
**              4 - case
**
**  Returns:  A character pointer to the converted string if
**            successful, a NULL pointer if the number base specified
**            is out of range.
*/


//Size of base2 (8 bits) * bytes in input number + null terminator
#define BUFSIZE (sizeof(int64_t) * 8 + 1)
/**
 * Converts long to string
 * @param n Number to be converteed
 * @param str Buffer to build string
 * @param base Base for conversion (2-36)
 * @param lowercase True if lowercase, false if upercase
 * @return str if success, else null
 */
static char *ltoa(int64_t n, char *str, int base, bool lowercase) {
    uint64_t arg;
    int64_t quot;
    int64_t rem;
    char *head = str;
    char *tail; 
    char a; 
    char tmp;
    if(lowercase){
        a='a';
    }else{
        a='A';
    }
    if (36 < base || 2 > base){
        return NULL;                    /* can only use 0-9, A-Z        */
    }
    if(n < 0 && base == 10){
        *head++ = '-';
        n = -n;
    }
    tail = head;

    arg = n;
    do{
        quot       = arg/base; 
        rem        = arg%base; 
        *tail++  = (char)(rem + ((9 < rem) ?
                    (a - 10) : '0'));
        arg    = quot;
    }while(quot != 0);

    *tail-- = '\0';


    //in-place reverse digits
    while(head < tail){
        tmp = *head;
        *head = *tail;
        *tail = tmp;
        head++;
        tail--;
    }

    return str;
}/*}}}*/

//Serial output code /*{{{*/

//
//TODO: Possibly buffer serial messages using FreeRTOS queue
//

//from usart.c in XBH, modified a bit
static void uart_vwriteP(const char *Buffer, va_list *list) {/*{{{*/
	int format_flag;
	char str_buffer[25];
	char str_null_buffer[25];
	char move = 0;
	int tmp = 0;
	char by;
	char *ptr;
    uint8_t base;
    bool is_signed;
    bool is_lower = true;
    uint8_t field_sz;
    
		
	//Ausgabe der Zeichen
	for(;;) {
		by = *(Buffer++);
		if(by==0) break; // end of format string
            
		if (by == '%') {
            bool zeropad = false;
			by = *(Buffer++);
            if(by == '0'){
                by = *(Buffer++);
                zeropad = true;
            }

			if (isdigit((uint8_t)by)>0){
				str_null_buffer[0] = by;
				str_null_buffer[1] = '\0';
				move = atoi(str_null_buffer);
				by = *(Buffer++);
			}

			switch (by) {
                case 'h':
                    by = *(Buffer++);
                    if(by == 'h') {
                        field_sz = 1;
                        by = *(Buffer++);
                    }else{
                        field_sz = 2;
                    }
                    break;
                case 'l':
                    by = *(Buffer++);
                    if(by == 'l') {
                        field_sz = 8;
                        by = *(Buffer++);
                    }else{
                        field_sz = 4;
                    }
                    break;
                case 'z':
                    by = *(Buffer++);
                    field_sz = 4;
                    break;
                default:
                    field_sz = 4;
            }

			switch (by) {
				case 's':
					ptr = va_arg(*list,char *);
					while(*ptr) { uart_write_char( *ptr++); }
					break;
				case 'c':
					//Int to char
					format_flag = va_arg(*list,int);
					uart_write_char(format_flag++);
					break;
				case 'b':
                    base = 2;
                    is_signed = false;
					goto ConversionLoop;
				case 'i':
				case 'd':
                    is_signed = true;
                    base = 10;
                    goto ConversionLoop;
				case 'u':
                    is_signed = false;
                    base = 10;
					goto ConversionLoop;
				case 'o':
                    is_signed = false;
                    base = 8;
					goto ConversionLoop;
                case 'X':
                    is_lower = false;
				case 'x':
				case 'p':
                    is_signed = false;
                    base = 16;
					//****************************
                ConversionLoop:
					//****************************
                    switch(field_sz){
                        case 1:
                        case 2:
                        case 4:
                        default:
                            if(is_signed){
                                int32_t var = (int32_t)va_arg(*list,int32_t);
                                ltoa(var, str_buffer, base, is_lower);
                            }else{
                                int32_t var = (uint32_t)va_arg(*list,uint32_t);
                                ltoa(var, str_buffer, base, is_lower);
                            }
                            break;
                        case 8:
                            if(is_signed){
                                int64_t var = (int64_t)va_arg(*list,int64_t);
                                ltoa(var, str_buffer, base, is_lower);
                            }else{
                                uint64_t var = (uint64_t)va_arg(*list,uint64_t);
                                ltoa(var, str_buffer, base, is_lower);
                            }
                            break;
                    }


					int b=0;
					while (str_buffer[b++] != 0){};
					b--;
					if (b<move)
					{
						move -=b;
						for (tmp = 0;tmp<move;tmp++)
						{
                            if(zeropad){
                                str_null_buffer[tmp] = '0';
                            }else{
                                str_null_buffer[tmp] = ' ';
                            }
						}
						//tmp ++;
						str_null_buffer[tmp] = '\0';
						strcat(str_null_buffer,str_buffer);
						strcpy(str_buffer,str_null_buffer);
					}
					uart_write_str (str_buffer);
					move =0;
					break;
			}
			
		} else if (by == '\n'){
			uart_write_char( '\r' );	
			uart_write_char( by );	
        }else {
			uart_write_char( by );	
		}
	}
}/*}}}*/

void uart_printf(const char *buffer,...) {
	va_list ap;
	va_start (ap, buffer);	
	uart_vwriteP(buffer, &ap);
	va_end(ap);
}
void __error__(char * filename, uint32_t line) { 
    uart_printf("ERROR:%s:%d\n", filename, line);
}
#pragma GCC diagnostic push  // require GCC 4.6
#pragma GCC diagnostic ignored "-Wcast-qual"
void assert_called( const char * const filename,uint32_t line ) { __error__((char*)filename,line); }
#pragma GCC diagnostic pop
/*}}}*/

/** Simulates recv() w/ MSG_WAITALL flag */
int recv_waitall(int s, void *mem, size_t len, int flags){/*{{{*/
    int recved = 0;
    while(recved < len){
        int retval = recv(s, (uint8_t *)mem+recved, len-recved, flags);
        if(retval <= 0){
            return retval;
        }else{
            recved += retval;
        }
    }
    return recved;
}/*}}}*/

//for c99 compliance
extern inline uint8_t htoi(char h);
extern inline char ntoa(uint8_t n);
