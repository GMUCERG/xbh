#ifndef _MEASURE_H_
#define _MEASURE_H_

#include <inttypes.h>
#include <stdbool.h>

#include "FreeRTOSConfig.h"
#include <FreeRTOS.h>
#include <queue.h>

/**
 * Contains power measurement sample
 * All values should be big endian when sent over network
 */
struct pwr_sample{
    uint64_t timestamp;
    uint16_t voltage;
    uint16_t current;
    uint16_t power;
};

extern QueueHandle_t pwr_sample_q_handle;

void measure_setup(void);
void measure_start(void);
bool measure_isrunning(void);
uint64_t measure_get_stop(void);
uint64_t measure_get_start(void);
void exec_timer_start(void);
void power_setup(void);
#endif
