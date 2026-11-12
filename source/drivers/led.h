#ifndef LED_H
#define LED_H
#include <stdint.h>
void Led_Init(void);
void Led_Set(uint8_t on);
void Led_Toggle(void);
#endif
