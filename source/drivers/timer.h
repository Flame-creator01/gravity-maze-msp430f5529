#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>
void Timer_Init(void);
uint32_t Timer_GetMilliseconds(void);
uint8_t Timer_Elapsed(uint32_t now, uint32_t *last, uint16_t period);
#endif
