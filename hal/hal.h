#ifndef _HAL_H
#define _HAL_H

#include <inttypes.h>

extern uint32_t g_syshz; 

void HAL_setup(void);
void uart_writeP (const char *buffer,...) ;
#endif