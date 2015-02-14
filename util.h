#ifndef __UTILS_H__
#define __UTILS_H__

/** @file Contains stuff formerly in xbh_utils.h */

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include "hal/hal.h"

// For htonl/htons
#include <lwip/def.h>



/**
 * Wrapper to printf to uart
 * @param ... Same as printf
 */
#ifdef DEBUG
#define DEBUG_OUT(...) {uart_printf("%s: %d: ",__FILE__,__LINE__); uart_printf( __VA_ARGS__);}
#else
#define DEBUG_OUT(...)
#endif



/**
 * Loop if error condition is true to preserve state for debugger, after
 * outputting message
 * @param x true if error condition
 * @param ... Message to printf
 */
#define LOOP_ERRMSG(x,...) if(x){uart_printf(__VA_ARGS__); while(1);}

/**
 * Outputs message if error condition happens
 * @param x true if error condition
 * @param ... Message to printf
 */
#define COND_ERRMSG(x,...) if(x){uart_printf(__VA_ARGS__);}


#ifdef DEBUG
/**
 * Outputs message of error condition happens; only enabled if debug is on
 * @param x true if error condition
 * @param ... Message to printf
 */
#define ASSERT_MSG(x,...) if(x){DEBUG_OUT(__VA_ARGS);}
#else
#define ASSERT_MSG(x,...)
#endif

/* 
 * http://stackoverflow.com/questions/9907160/how-to-convert-enum-names-to-string-in-c 
 */
#define GEN_ENUM(ENUM) ENUM,
#define GEN_STR(STRING) #STRING,

/*
 * #define FOREACH_FRUIT(FRUIT) \
 *      FRUIT(apple)   \
 *      FRUIT(orange)  \
 *      FRUIT(grape)   \
 *      FRUIT(banana)  
 */
/*
 * enum FRUIT_ENUM {
 *      FOREACH_FRUIT(GEN_ENUM)
 * };
 */
/*
 * static const char *FRUIT_STRING[] = {
 *      FOREACH_FRUIT(GEN_STR)
 * };
 */


// Byte swap macros 

#define htonll(x) __builtin_bswap64(x)
#define ntohll(x) __builtin_bswap64(x)

char *ltoa(long val, char *str, int base, bool lowercase);
void uart_printf(const char *buffer,...) ;
int recv_waitall(int s, void *mem, size_t len, int flags);

/**
 * Converts hex digit in ascii to numerical value
 * From XBH
 */
inline uint8_t htoi(char h) {
	if(h >= '0' && h <= '9')
		return (h-'0');

	if(h>= 'a' && h <= 'f')
		return (h-'a'+10);

	if(h>= 'A' && h <= 'F')
		return (h-'A'+10);

	return 255;
}

/**
 * Converts nibble to hex
 */
inline char ntoa(uint8_t n){
	if(n<=9)
		return n+'0';
	else
		return n+'A'-10;
}

#endif
