#ifndef _MEASURE_H_
#define _MEASURE_H_

#include <inttypes.h>
#include <stdbool.h>

#include "FreeRTOSConfig.h"
#include <FreeRTOS.h>
#include <queue.h>

extern QueueHandle_t pwr_sample_q_handle;

void measure_setup(void);
void measure_start(void);
bool measure_isrunning(void);
uint64_t measure_get_stop(void);
uint64_t measure_get_start(void);
void exec_timer_start(void);
uint32_t measure_get_avg(void);
uint32_t measure_get_max(void);
uint32_t measure_get_gain(void);
uint32_t measure_get_cntover(void);

#endif
