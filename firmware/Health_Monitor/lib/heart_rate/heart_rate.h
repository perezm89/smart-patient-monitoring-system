
#ifndef HEART_RATE_H
#define HEART_RATE_H

#include <stdint.h>
#include <stdbool.h>

void heart_rate_init(void);
bool heart_rate_add_sample(uint32_t ir_value, float *bpm);

#endif
