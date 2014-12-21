#ifndef __UTILS_H__
#define __UTILS_H__


#include "hal/hal.h"
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
 * Loop while error condition is true
 * @param x true if error condition
 */
#define LOOP_ERR(x) while(x)

/**
 * Loop while error condition is true
 * @param x true if error condition
 * @param ... Input to printf
 */
#define LOOP_ERRMSG(x,...) if(x){uart_printf(__VA_ARGS__); while(1);}

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




char *ltoa(long val, char *str, int base);
void uart_printf(const char *buffer,...) ;
int recv_bytes(int s, void *mem, size_t len, int flags);

#ifdef DEBUG
//void dbg_maskisr(void) __attribute__((used));
//void dbg_unmaskisr(void) __attribute__((used));
void dbg_maskisr(void);
void dbg_unmaskisr(void);
#endif

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

#endif
