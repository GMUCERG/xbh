#ifndef __UTILS_H__
#define __UTILS_H__

#define xbh_printf uart_writeP
#ifdef DEBUG
#include "hal/hal.h"
#define debug_writeP(...) {uart_writeP("%s: %d: ",__FILE__,__LINE__); uart_writeP( __VA_ARGS__);}
#define DEBUG_OUT(...) debug_writeP(__VA_ARGS__)
//#define debug_writeP(msg, ...) uart_writeP("%s: %d: "msg,__FILE__,__LINE__, ##__VA_ARGS__ )
//#define DEBUG_OUT(msg, ...) debug_writeP(msg,  ##__VA_ARGS__)
#else
#define DEBUG_OUT(msg, ...)
#endif



/**
 * Loop while error condition is true
 * @param x true if error condition
 */
#define LOOP_ERR(x) while(x)


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

#endif
