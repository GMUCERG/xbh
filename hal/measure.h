#ifndef _MEASURE_H_
#define _MEASURE_H_

#include <inttypes.h>
#include <stdbool.h>

#include "FreeRTOSConfig.h"
#include <FreeRTOS.h>
#include <queue.h>

/**
 * Contains power measurement sample
 * All values should be big endian
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
#endif
