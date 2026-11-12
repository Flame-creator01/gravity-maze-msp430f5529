#ifndef WHEEL_H
#define WHEEL_H
#include <stdint.h>
/* Analog thumb wheel on the TI/SEEED MSP-EXP430F5529 board:
   P8.0 high enables it, P6.5 is ADC12 input A5. */
void Wheel_Init(void);
/* Returns the number of adjustment ticks since the last poll (-6..+6, 0 = none).
   Call periodically (the app polls every 20 ms) and discard outside TUNE. */
int8_t Wheel_Poll(void);
#endif
