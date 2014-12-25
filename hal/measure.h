#ifndef _MEASURE_H_
#define _MEASURE_H_

#include <inttypes.h>

struct pwr_measurement{
    uint64_t timestamp;
    uint16_t value;
}


void exec_start(void);

void exec_timer_setup(void);
void pwr_measure_setup(void);

void pwr_get_measurement;

#endif
